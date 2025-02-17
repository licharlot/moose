//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PINSFVMomentumAdvection.h"
#include "INSFVPressureVariable.h"
#include "PINSFVSuperficialVelocityVariable.h"
#include "FVUtils.h"
#include "NS.h"

registerMooseObject("NavierStokesApp", PINSFVMomentumAdvection);

InputParameters
PINSFVMomentumAdvection::validParams()
{
  auto params = INSFVMomentumAdvection::validParams();
  params.addClassDescription("Object for advecting superficial momentum, e.g. rho*u_d, "
                             "in the porous media momentum equation");
  params.addRequiredCoupledVar(NS::porosity, "Porosity auxiliary variable");
  params.addParam<bool>(
      "smooth_porosity", false, "Whether the porosity field is smooth or has discontinuities");

  return params;
}

PINSFVMomentumAdvection::PINSFVMomentumAdvection(const InputParameters & params)
  : INSFVMomentumAdvection(params),
    _eps_var(dynamic_cast<const MooseVariableFV<Real> *>(getFieldVar(NS::porosity, 0))),
    _eps(getFunctor<ADReal>(NS::porosity)),
    _smooth_porosity(getParam<bool>("smooth_porosity"))
{
  if (!dynamic_cast<const PINSFVSuperficialVelocityVariable *>(_u_var))
    mooseError("PINSFVMomentumAdvection may only be used with a superficial advective velocity, "
               "of variable type PINSFVSuperficialVelocityVariable.");
}

#ifdef MOOSE_GLOBAL_AD_INDEXING
VectorValue<ADReal>
PINSFVMomentumAdvection::coeffCalculator(const Elem & elem) const
{
  // these coefficients arise from simple control volume balances of advection and diffusion. These
  // coefficients are the linear coefficients associated with the centroid of the control volume.
  // Note that diffusion coefficients should always be positive, e.g. elliptic operators always
  // yield positive definite matrices
  //
  // Example 1D discretization of diffusion, e.g. the sum of the fluxes around a control volume:
  //
  // \sum_f -D \nabla \phi * \hat{n} =
  //   -D_e * (phi_E - \phi_C) / d_{CE} * 1 - D_w * (\phi_C - \phi_W) / d_{WC} * -1 =
  //   D_e / d_{CE} * (\phi_C - \phi_E) + D_w / d_{WC} * (\phi_C - \phi_W)
  //
  // Note the positive coefficients for \phi_C !!
  //
  // Now an example 1D discretization for advection using central differences, e.g. an average
  // interpolation
  //
  // \sum_f \vec{u} \phi \hat{n} =
  //   u_w * (\phi_W + \phi_C) / 2 * -1 + u_e * (\phi_C + \phi_E) / 2 * 1 =
  //   -u_w / 2 * \phi_W + u_e / 2 * \phi_E + (u_e - u_w) / 2 * \phi_C
  //
  // Note that the coefficient for \phi_C may or may not be positive depending on the values of u_e
  // and u_w

  VectorValue<ADReal> coeff = 0;

  ADRealVectorValue elem_velocity(_u_var->getElemValue(&elem));

  if (_v_var)
    elem_velocity(1) = _v_var->getElemValue(&elem);
  if (_w_var)
    elem_velocity(2) = _w_var->getElemValue(&elem);

  auto action_functor = [&coeff,
                         &elem_velocity,
#ifndef NDEBUG
                         &elem,
#endif
                         this](const Elem & libmesh_dbg_var(functor_elem),
                               const Elem * const neighbor,
                               const FaceInfo * const fi,
                               const Point & surface_vector,
                               Real libmesh_dbg_var(coord),
                               const bool elem_has_info)
  {
    mooseAssert(fi, "We need a non-null FaceInfo");
    mooseAssert(&elem == &functor_elem, "Elems don't match");
    mooseAssert((&elem == &fi->elem()) || (&elem == fi->neighborPtr()),
                "Surely the element has to match one of the face information's elements right?");

    const Point normal = elem_has_info ? fi->normal() : Point(-fi->normal());
    const Point & rc_centroid = elem_has_info ? fi->elemCentroid() : fi->neighborCentroid();
#ifndef NDEBUG
    for (const auto i : make_range(unsigned(LIBMESH_DIM)))
      mooseAssert(
          coord == 0
              ? true
              : MooseUtils::absoluteFuzzyEqual(
                    normal(i), (surface_vector / (fi->faceArea() * coord))(i), libMesh::TOLERANCE),
          "Let's make sure our normal is what we think it is");
#endif

    // Unless specified otherwise, "elem" here refers to the element we're computing the
    // Rhie-Chow coefficient for. "neighbor" is the element across the current FaceInfo (fi)
    // face from the Rhie-Chow element

    const auto face_mu = _mu(std::make_tuple(
        fi, Moose::FV::LimiterType::CentralDifference, true, faceArgSubdomains(fi)));
    const auto face_rho = _rho(std::make_tuple(
        fi, Moose::FV::LimiterType::CentralDifference, true, faceArgSubdomains(fi)));
    const auto face_eps = _eps(std::make_tuple(
        fi, Moose::FV::LimiterType::CentralDifference, true, faceArgSubdomains(fi)));

    if (onBoundary(*fi))
    {
      // Find the boundary id that has an associated INSFV boundary condition
      // if a face has more than one bc_id
      for (const auto bc_id : fi->boundaryIDs())
      {
        if (_no_slip_wall_boundaries.find(bc_id) != _no_slip_wall_boundaries.end())
        {
          // Need to account for viscous shear stress from wall
          for (const auto i : make_range(_dim))
            coeff(i) += face_mu / face_eps * surface_vector.norm() /
                        std::abs((fi->faceCentroid() - rc_centroid) * normal) *
                        (1 - normal(i) * normal(i));

          // No flow normal to wall, so no contribution to coefficient from the advection term
          return;
        }

        if (_slip_wall_boundaries.find(bc_id) != _slip_wall_boundaries.end())
          // In the case of a slip wall we neither have viscous shear stress from the wall nor
          // normal outflow, so our contribution to the coefficient is zero
          return;

        if (_flow_boundaries.find(bc_id) != _flow_boundaries.end())
        {
          ADRealVectorValue face_velocity(_u_var->getBoundaryFaceValue(*fi));
          if (_v_var)
            face_velocity(1) = _v_var->getBoundaryFaceValue(*fi);
          if (_w_var)
            face_velocity(2) = _w_var->getBoundaryFaceValue(*fi);

          const auto advection_coeffs =
              Moose::FV::interpCoeffs(_advected_interp_method, *fi, elem_has_info, face_velocity);
          ADReal temp_coeff =
              face_rho * face_velocity / face_eps * surface_vector * advection_coeffs.first;

          if (_fully_developed_flow_boundaries.find(bc_id) ==
              _fully_developed_flow_boundaries.end())
            // We are not on a fully developed flow boundary, so we have a viscous term
            // contribution. This term is slightly modified relative to the internal face term.
            // Instead of the distance between elem and neighbor centroid, we just have the distance
            // between the elem and face centroid. Specifically, the term below is the result of
            // Moukalled 8.80, 8.82, and the orthogonal correction approach equation for E_f,
            // equation 8.89. So relative to the internal face viscous term, we have substituted
            // eqn. 8.82 for 8.78
            // Note: If mu is an effective diffusivity, this should not be divided by face_eps
            temp_coeff += face_mu / face_eps * surface_vector.norm() /
                          (fi->faceCentroid() - rc_centroid).norm();

          // For flow boundaries, the coefficient addition is the same for every velocity component
          for (const auto i : make_range(_dim))
            coeff(i) += temp_coeff;

          return;
        }

        if (_symmetry_boundaries.find(bc_id) != _symmetry_boundaries.end())
        {
          // Moukalled eqns. 15.154 - 15.156, adapted for porosity
          for (const auto i : make_range(_dim))
            coeff(i) += 2. * face_mu / face_eps * surface_vector.norm() /
                        std::abs((fi->faceCentroid() - rc_centroid) * normal) * normal(i) *
                        normal(i);

          return;
        }
      }

      const auto bc_id = *fi->boundaryIDs().begin();
      mooseError("The INSFVMomentumAdvection object ",
                 this->name(),
                 " is not completely bounded by INSFVBCs. Please examine surface ",
                 bc_id,
                 " and your FVBCs blocks.");
    }

    // Else we are on an internal face

    mooseAssert((neighbor == &fi->elem()) || (neighbor == fi->neighborPtr()),
                "Surely the neighbor has to match one of the face information's elements, right?");

    ADRealVectorValue neighbor_velocity(_u_var->getNeighborValue(neighbor, *fi, elem_velocity(0)));
    if (_v_var)
      neighbor_velocity(1) = _v_var->getNeighborValue(neighbor, *fi, elem_velocity(1));
    if (_w_var)
      neighbor_velocity(2) = _w_var->getNeighborValue(neighbor, *fi, elem_velocity(2));

    ADRealVectorValue interp_v;
    Moose::FV::interpolate(Moose::FV::InterpMethod::Average,
                           interp_v,
                           elem_velocity,
                           neighbor_velocity,
                           *fi,
                           elem_has_info);

    const auto advection_coeffs =
        Moose::FV::interpCoeffs(_advected_interp_method, *fi, elem_has_info, interp_v);
    ADReal temp_coeff = face_rho * interp_v / face_eps * surface_vector * advection_coeffs.first;

    // Now add the viscous flux. Note that this includes only the orthogonal component! See
    // Moukalled equations 8.80, 8.78, and the orthogonal correction approach equation for
    // E_f, equation 8.69
    temp_coeff += face_mu / face_eps * surface_vector.norm() /
                  (fi->neighborCentroid() - fi->elemCentroid()).norm();

    // For internal faces the coefficient is the same for every velocity component.
    for (const auto i : make_range(_dim))
      coeff(i) += temp_coeff;
  };

  Moose::FV::loopOverElemFaceInfo(elem, _subproblem.mesh(), _subproblem, action_functor);

  return coeff;
}

void
PINSFVMomentumAdvection::interpolate(Moose::FV::InterpMethod m, ADRealVectorValue & v)
{
  const Elem * const elem = &_face_info->elem();
  const Elem * const neighbor = _face_info->neighborPtr();

  if (onBoundary(*_face_info))
  {
#ifndef NDEBUG
    bool flow_boundary_found = false;
    for (const auto b_id : _face_info->boundaryIDs())
      if (_flow_boundaries.find(b_id) != _flow_boundaries.end())
      {
        flow_boundary_found = true;
        break;
      }

    mooseAssert(flow_boundary_found,
                "INSFV*Advection flux kernel objects should only execute on flow boundaries.");
#endif

    v(0) = _u_var->getBoundaryFaceValue(*_face_info);
    if (_v_var)
      v(1) = _v_var->getBoundaryFaceValue(*_face_info);
    if (_w_var)
      v(2) = _w_var->getBoundaryFaceValue(*_face_info);

    return;
  }

  const auto elem_face = elemFromFace();
  const auto neighbor_face = neighborFromFace();

  Moose::FV::interpolate(
      Moose::FV::InterpMethod::Average, v, _vel(elem_face), _vel(neighbor_face), *_face_info, true);

  if (m == Moose::FV::InterpMethod::Average)
    return;

  // Avoid computing a pressure gradient near porosity jumps
  if (!_smooth_porosity)
    if (MetaPhysicL::raw_value(_eps_var->adGradSln(elem)).norm() > 1e-12 ||
        MetaPhysicL::raw_value(_eps_var->adGradSln(neighbor)).norm() > 1e-12)
      return;

  mooseAssert((neighbor && this->hasBlocks(neighbor->subdomain_id())),
              "We should be on an internal face...");

  // Get pressure gradient. This is the uncorrected gradient plus a correction from cell centroid
  // values on either side of the face
  const VectorValue<ADReal> & grad_p = _p_var->adGradSln(*_face_info);

  // Get uncorrected pressure gradient. This will use the element centroid gradient if we are
  // along a boundary face
  const VectorValue<ADReal> & unc_grad_p = _p_var->uncorrectedAdGradSln(*_face_info);

  const Point & elem_centroid = _face_info->elemCentroid();
  const Point & neighbor_centroid = _face_info->neighborCentroid();
  Real elem_volume = _face_info->elemVolume();
  Real neighbor_volume = _face_info->neighborVolume();

  // Now we need to perform the computations of D
  const VectorValue<ADReal> & elem_a = rcCoeff(*elem);

  mooseAssert(_subproblem.getCoordSystem(elem->subdomain_id()) ==
                  _subproblem.getCoordSystem(neighbor->subdomain_id()),
              "Coordinate systems must be the same between the two elements");

  Real coord;
  coordTransformFactor(_subproblem, elem->subdomain_id(), elem_centroid, coord);

  elem_volume *= coord;

  VectorValue<ADReal> elem_D = 0;
  for (const auto i : make_range(_dim))
  {
    mooseAssert(elem_a(i).value() != 0, "We should not be dividing by zero");
    elem_D(i) = elem_volume / elem_a(i);
  }

  VectorValue<ADReal> face_D;

  const VectorValue<ADReal> & neighbor_a = rcCoeff(*neighbor);

  coordTransformFactor(_subproblem, neighbor->subdomain_id(), neighbor_centroid, coord);
  neighbor_volume *= coord;

  VectorValue<ADReal> neighbor_D = 0;
  for (const auto i : make_range(_dim))
  {
    mooseAssert(neighbor_a(i).value() != 0, "We should not be dividing by zero");
    neighbor_D(i) = neighbor_volume / neighbor_a(i);
  }
  Moose::FV::interpolate(
      Moose::FV::InterpMethod::Average, face_D, elem_D, neighbor_D, *_face_info, true);

  // evaluate face porosity, see (18) in Hanimann 2021 or (11) in Nordlund 2016
  const auto face_eps = _eps(std::make_tuple(
      _face_info, Moose::FV::LimiterType::CentralDifference, true, faceArgSubdomains()));

  // perform the pressure correction
  for (const auto i : make_range(_dim))
    v(i) -= face_D(i) * face_eps * (grad_p(i) - unc_grad_p(i));
}
#else

VectorValue<ADReal>
PINSFVMomentumAdvection::coeffCalculator(const Elem &) const
{
  mooseError("PINSFVMomentumAdvection only works with global AD indexing");
}

void
PINSFVMomentumAdvection::interpolate(Moose::FV::InterpMethod, ADRealVectorValue &)
{
  mooseError("PINSFVMomentumAdvection only works with global AD indexing");
}
#endif

ADReal
PINSFVMomentumAdvection::computeQpResidual()
{
  ADRealVectorValue v;
  ADReal adv_quant_interface;

  const auto elem_face = elemFromFace();
  const auto neighbor_face = neighborFromFace();

  // Superficial velocity interpolation
  this->interpolate(_velocity_interp_method, v);

  // Interpolation of the interstitial momentum
  Moose::FV::interpolate(_advected_interp_method,
                         adv_quant_interface,
                         _adv_quant(elem_face) / _eps(elem_face),
                         _adv_quant(neighbor_face) / _eps(neighbor_face),
                         v,
                         *_face_info,
                         true);

  return _normal * v * adv_quant_interface;
}
