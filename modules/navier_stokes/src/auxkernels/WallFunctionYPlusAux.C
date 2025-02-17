//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "WallFunctionYPlusAux.h"
#include "INSFVMethods.h"

registerMooseObject("NavierStokesApp", WallFunctionYPlusAux);

InputParameters
WallFunctionYPlusAux::validParams()
{
  InputParameters params = AuxKernel::validParams();
  params.addClassDescription(
      "Calculates y+ value according to the algebraic velocity standard wall function.");
  params.addRequiredCoupledVar("u", "The velocity in the x direction.");
  params.addCoupledVar("v", "The velocity in the y direction.");
  params.addCoupledVar("w", "The velocity in the z direction.");
  params.addParam<Real>("rho", "fluid density");
  params.addRequiredParam<MaterialPropertyName>("mu", "Dynamic viscosity");
  params.addParam<std::vector<BoundaryName>>("walls", "Boundaries that correspond to solid walls");
  return params;
}

WallFunctionYPlusAux::WallFunctionYPlusAux(const InputParameters & params)
  : AuxKernel(params),
    _dim(_subproblem.mesh().dimension()),
    _u_var(dynamic_cast<const INSFVVelocityVariable *>(getFieldVar("u", 0))),
    _v_var(params.isParamValid("v")
               ? dynamic_cast<const INSFVVelocityVariable *>(getFieldVar("v", 0))
               : nullptr),
    _w_var(params.isParamValid("w")
               ? dynamic_cast<const INSFVVelocityVariable *>(getFieldVar("w", 0))
               : nullptr),
    _rho(getParam<Real>("rho")),
    _mu(getADMaterialProperty<Real>("mu")),
    _wall_boundary_names(getParam<std::vector<BoundaryName>>("walls"))
{
}

Real
WallFunctionYPlusAux::computeValue()
{
  const Elem & elem = *_current_elem;

  bool wall_bounded = false;
  Real min_wall_dist = 1e10;
  Point wall_vec;
  Point normal;
  for (unsigned int i_side = 0; i_side < elem.n_sides(); ++i_side)
  {
    const std::vector<BoundaryID> side_bnds =
        _subproblem.mesh().getBoundaryIDs(_current_elem, i_side);
    for (const BoundaryName & name : _wall_boundary_names)
    {
      BoundaryID wall_id = _subproblem.mesh().getBoundaryID(name);
      for (BoundaryID side_id : side_bnds)
      {
        if (side_id == wall_id)
        {
          const auto side_elem_ptr = elem.side_ptr(i_side);
          const FaceInfo * const fi = _mesh.faceInfo(&elem, i_side);
          const Point & this_normal = fi->normal();
          Point this_wall_vec = (elem.vertex_average() - side_elem_ptr->vertex_average());
          Real dist = std::abs(this_wall_vec * normal);
          if (dist < min_wall_dist)
          {
            min_wall_dist = dist;
            wall_vec = this_wall_vec;
            normal = this_normal;
          }
          wall_bounded = true;
        }
      }
    }
  }

  if (!wall_bounded)
    return 0;

  // Get the velocity vector
  ADRealVectorValue velocity(_u_var->getElemValue(&elem));
  if (_v_var)
    velocity(1) = _v_var->getElemValue(&elem);
  if (_w_var)
    velocity(2) = _w_var->getElemValue(&elem);

  // Compute the velocity and direction of the velocity component that is parallel to the wall
  Real dist = std::abs(wall_vec * normal);
  ADReal perpendicular_speed = velocity * normal;
  ADRealVectorValue parallel_velocity = velocity - perpendicular_speed * normal;
  ADReal parallel_speed = parallel_velocity.norm();
  ADRealVectorValue parallel_dir = parallel_velocity / parallel_speed;

  if (parallel_speed.value() < 1e-6)
    return 0;

  if (!std::isfinite(parallel_speed.value()))
    return parallel_speed.value();

  // Compute the friction velocity and the wall shear stress
  ADReal u_star = findUStar(_mu[_qp].value(), _rho, parallel_speed, dist);
  ADReal tau = u_star * u_star * _rho;

  return (dist * u_star * _rho / _mu[_qp]).value();
}
