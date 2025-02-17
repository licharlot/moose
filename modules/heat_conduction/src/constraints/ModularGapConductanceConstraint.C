//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ModularGapConductanceConstraint.h"
#include "GapFluxModelBase.h"
#include "MooseError.h"

registerMooseObject("HeatConductionApp", ModularGapConductanceConstraint);

InputParameters
ModularGapConductanceConstraint::validParams()
{
  InputParameters params = ADMortarConstraint::validParams();
  params.addClassDescription(
      "Computes the residual and Jacobian contributions for the 'Lagrange Multiplier' "
      "implementation of the thermal contact problem. For more information, see the "
      "detailed description here: http://tinyurl.com/gmmhbe9");
  params.addParam<std::vector<UserObjectName>>("gap_flux_models",
                                               "List of GapFluxModel user objects");
  params.addCoupledVar("displacements", "Displacement variables");

  MooseEnum gap_geometry_type("PLATE CYLINDER SPHERE", "PLATE");
  params.addParam<MooseEnum>("gap_geometry_type",
                             gap_geometry_type,
                             "Gap calculation type. The geometry type is used to compute "
                             "gap distances and scale fluxes to ensure energy balance.");
  params.addRangeCheckedParam<Real>("max_gap", 1e6, "max_gap>=0", "A maximum gap size");
  params.addParam<RealVectorValue>("cylinder_axis_point_1",
                                   "Start point for line defining cylindrical axis");
  params.addParam<RealVectorValue>("cylinder_axis_point_2",
                                   "End point for line defining cylindrical axis");
  params.addParam<RealVectorValue>("sphere_origin", "Origin for sphere geometry");

  return params;
}

ModularGapConductanceConstraint::ModularGapConductanceConstraint(const InputParameters & parameters)
  : ADMortarConstraint(parameters),
    _gap_flux_model_names(getParam<std::vector<UserObjectName>>("gap_flux_models")),
    _disp_name(parameters.getVecMooseType("displacements")),
    _n_disp(_disp_name.size()),
    _disp_secondary(_n_disp),
    _disp_primary(_n_disp),
    _gap_width(0.0),
    _gap_geometry_type(getParam<MooseEnum>("gap_geometry_type").getEnum<GapGeometry>()),
    _surface_integration_factor(1.0),
    _p1(declareRestartableData<Point>("cylinder_axis_point_1", Point(0, 1, 0))),
    _p2(declareRestartableData<Point>("cylinder_axis_point_2", Point(0, 0, 0))),
    _r1(0),
    _r2(0),
    _max_gap(getParam<Real>("max_gap")),
    _adjusted_length(0.0)

{
  if (_n_disp && !getParam<bool>("use_displaced_mesh"))
    paramWarning("displacements",
                 "You are coupling displacement variables but are evaluating the gap width on the "
                 "undisplaced mesh. This is probably a mistake.");

  for (unsigned int i = 0; i < _n_disp; ++i)
  {
    auto & disp_var = _subproblem.getStandardVariable(_tid, _disp_name[i]);
    _disp_secondary[i] = &disp_var.adSln();
    _disp_primary[i] = &disp_var.adSlnNeighbor();
  }

  for (const auto & name : _gap_flux_model_names)
  {
    const auto & gap_model = getUserObjectByName<GapFluxModelBase>(name);

    // This constraint explicitly calls the gap flux model user objects to
    // obtain contributions to its residuals. It therefore depends on all
    // variables and material properties, that these gap flux models use, to be
    // computed and up to date. To ensure that we collect all variable and
    // material property dependencies of these models and declare them as
    // dependencies of this constraint object. This turns an implicit, hidden
    // dependency into an explicit dependency that MOOSE will automatically fulfill.

    // pass variable dependencies through
    const auto & var_dependencies = gap_model.getMooseVariableDependencies();
    for (const auto & var : var_dependencies)
      addMooseVariableDependency(var);

    // pass material property dependencies through
    const auto & mat_dependencies = gap_model.getMatPropDependencies();
    _material_property_dependencies.insert(mat_dependencies.begin(), mat_dependencies.end());

    // add gap model to list
    _gap_flux_models.push_back(&gap_model);
  }
}

void
ModularGapConductanceConstraint::initialSetup()
{
  ///set generated from the passed in vector of subdomain names
  const auto & subdomainIDs = _mesh.meshSubdomains();

  // make sure all subdomains are using the same coordinate system
  Moose::CoordinateSystemType coord_system = feProblem().getCoordSystem(*subdomainIDs.begin());
  for (auto subdomain : subdomainIDs)
    if (feProblem().getCoordSystem(subdomain) != coord_system)
      mooseError("ModularGapConductanceConstraint requires all subdomains to have the same "
                 "coordinate system.");

  // Select proper coordinate system and geometry (plate, cylinder, sphere)
  setGapGeometryParameters(
      _pars, coord_system, feProblem().getAxisymmetricRadialCoord(), _gap_geometry_type, _p1, _p2);
}

void
ModularGapConductanceConstraint::setGapGeometryParameters(
    const InputParameters & params,
    const Moose::CoordinateSystemType coord_sys,
    unsigned int axisymmetric_radial_coord,
    GapGeometry & gap_geometry_type,
    Point & p1,
    Point & p2)
{

  // Determine what type of gap geometry we are dealing with
  // Either user input or from system's coordinate systems
  if (params.isParamSetByUser("gap_geometry_type"))
    gap_geometry_type = getParam<MooseEnum>("gap_geometry_type").getEnum<GapGeometry>();
  else
  {
    if (coord_sys == Moose::COORD_XYZ)
      gap_geometry_type = GapGeometry::PLATE;
    else if (coord_sys == Moose::COORD_RZ)
      gap_geometry_type = GapGeometry::CYLINDER;
    else if (coord_sys == Moose::COORD_RSPHERICAL)
      gap_geometry_type = GapGeometry::SPHERE;
  }

  // Check consistency of geometry information
  // Inform the user of needed input according to gap geometry (if not PLATE)
  if (gap_geometry_type == GapGeometry::PLATE)
  {
    if (coord_sys == Moose::COORD_RSPHERICAL)
      paramError("gap_geometry_type",
                 "'gap_geometry_type = PLATE' cannot be used with models having a spherical "
                 "coordinate system.");
  }
  else if (gap_geometry_type == GapGeometry::CYLINDER)
  {
    if (coord_sys == Moose::COORD_XYZ)
    {
      if (!params.isParamValid("cylinder_axis_point_1") ||
          !params.isParamValid("cylinder_axis_point_2"))
        paramError("gap_geometry_type",
                   "For 'gap_geometry_type = CYLINDER' to be used with a Cartesian model, "
                   "'cylinder_axis_point_1' and 'cylinder_axis_point_2' must be specified.");
      p1 = params.get<RealVectorValue>("cylinder_axis_point_1");
      p2 = params.get<RealVectorValue>("cylinder_axis_point_2");
    }
    else if (coord_sys == Moose::COORD_RZ)
    {
      if (params.isParamValid("cylinder_axis_point_1") ||
          params.isParamValid("cylinder_axis_point_2"))
        paramError("cylinder_axis_point_1",
                   "The 'cylinder_axis_point_1' and 'cylinder_axis_point_2' cannot be specified "
                   "with axisymmetric models.  The y-axis is used as the cylindrical axis of "
                   "symmetry.");

      if (axisymmetric_radial_coord == 0) // R-Z problem
      {
        p1 = Point(0, 0, 0);
        p2 = Point(0, 1, 0);
      }
      else // Z-R problem
      {
        p1 = Point(0, 0, 0);
        p2 = Point(1, 0, 0);
      }
    }
    else if (coord_sys == Moose::COORD_RSPHERICAL)
      paramError("gap_geometry_type",
                 "'gap_geometry_type = CYLINDER' cannot be used with models having a spherical "
                 "coordinate system.");
  }
  else if (gap_geometry_type == GapGeometry::SPHERE)
  {
    if (coord_sys == Moose::COORD_XYZ || coord_sys == Moose::COORD_RZ)
    {
      if (!params.isParamValid("sphere_origin"))
        paramError("gap_geometry_type",
                   "For 'gap_geometry_type = SPHERE' to be used with a Cartesian or axisymmetric "
                   "model, 'sphere_origin' must be specified.");
      p1 = params.get<RealVectorValue>("sphere_origin");
    }
    else if (coord_sys == Moose::COORD_RSPHERICAL)
    {
      if (params.isParamValid("sphere_origin"))
        paramError("sphere_origin",
                   "The 'sphere_origin' cannot be specified with spherical models.  x=0 is used "
                   "as the spherical origin.");
      p1 = Point(0, 0, 0);
    }
  }
}

ADReal
ModularGapConductanceConstraint::computeSurfaceIntegrationFactor() const
{

  ADReal surface_integration_factor = 1.0;

  if (_gap_geometry_type == GapGeometry::CYLINDER)
    surface_integration_factor = 0.5 * (_r1 + _r2) / _radius;
  else if (_gap_geometry_type == GapGeometry::SPHERE)
    surface_integration_factor = 0.25 * (_r1 + _r2) * (_r1 + _r2) / (_radius * _radius);

  return surface_integration_factor;
}

ADReal
ModularGapConductanceConstraint::computeGapLength() const
{

  if (_gap_geometry_type == GapGeometry::CYLINDER)
  {
    const auto denominator = _radius * std::log(_r2 / _r1);
    return std::min(denominator, _max_gap);
  }
  else if (_gap_geometry_type == GapGeometry::SPHERE)
  {
    const auto denominator = _radius * _radius * ((1.0 / _r1) - (1.0 / _r2));
    return std::min(denominator, _max_gap);
  }
  else
    return std::min(_r2 - _r1, _max_gap);
}

void
ModularGapConductanceConstraint::computeGapRadii(const ADReal & gap_length)
{
  const Point & current_point = _q_point[_qp];

  if (_gap_geometry_type == GapGeometry::CYLINDER)
  {
    // The vector _p1 + t*(_p2-_p1) defines the cylindrical axis.  The point along this
    // axis closest to current_point is found by the following for t:
    const Point p2p1(_p2 - _p1);
    const Point p1pc(_p1 - current_point);
    const Real t = -(p1pc * p2p1) / p2p1.norm_sq();

    // The nearest point on the cylindrical axis to current_point is p.
    const Point p(_p1 + t * p2p1);
    Point rad_vec(current_point - p);
    Real rad = rad_vec.norm();
    rad_vec /= rad;
    Real rad_dot_norm = rad_vec * _normals[_qp];

    if (rad_dot_norm > 0)
    {
      _r1 = rad;
      _r2 = rad + gap_length;
      _radius = _r1;
    }
    else if (rad_dot_norm < 0)
    {
      _r1 = rad - gap_length;
      _r2 = rad;
      _radius = _r2;
    }
    else
      mooseError("Issue with cylindrical flux calculation normals.\n");
  }
  else if (_gap_geometry_type == GapGeometry::SPHERE)
  {
    const Point origin_to_curr_point(current_point - _p1);
    const Real normal_dot = origin_to_curr_point * _normals[_qp];
    const Real curr_point_radius = origin_to_curr_point.norm();
    if (normal_dot > 0) // on inside surface
    {
      _r1 = curr_point_radius;
      _r2 = curr_point_radius + gap_length;
      _radius = _r1;
    }
    else if (normal_dot < 0) // on outside surface
    {
      _r1 = curr_point_radius - gap_length;
      _r2 = curr_point_radius;
      _radius = _r2;
    }
    else
      mooseError("Issue with spherical flux calculation normals. \n");
  }
  else
  {
    _r2 = gap_length;
    _r1 = 0;
    _radius = 0;
  }
}

ADReal
ModularGapConductanceConstraint::computeQpResidual(Moose::MortarType mortar_type)
{
  switch (mortar_type)
  {
    case Moose::MortarType::Primary:
      return _lambda[_qp] * _test_primary[_i][_qp];
    case Moose::MortarType::Secondary:
      return -_lambda[_qp] * _test_secondary[_i][_qp];
    case Moose::MortarType::Lower:
    {
      // we are creating an AD version of phys points primary and secondary here...
      ADRealVectorValue ad_phys_points_primary = _phys_points_primary[_qp];
      ADRealVectorValue ad_phys_points_secondary = _phys_points_secondary[_qp];

      // ...which uses the derivative vector of the primary and secondary displacements as
      // an approximation of the true phys points derivatives when the mesh is displacing
      if (_displaced)
        for (unsigned int i = 0; i < _n_disp; ++i)
        {
          ad_phys_points_primary(i).derivatives() = (*_disp_primary[i])[_qp].derivatives();
          ad_phys_points_secondary(i).derivatives() = (*_disp_secondary[i])[_qp].derivatives();
        }

      // compute an ADReal gap width to pass to each gap flux model
      _gap_width = (ad_phys_points_primary - ad_phys_points_secondary) * _normals[_qp];

      // First, compute radii with _gap_width
      computeGapRadii(_gap_width);

      // With radii, compute adjusted gap length for conduction
      _adjusted_length = computeGapLength();

      // Ensure energy balance for non-flat (non-PLATE) general geometries when using radiation
      _surface_integration_factor = computeSurfaceIntegrationFactor();

      // Sum up all flux contributions from all supplied gap flux models
      ADReal flux = 0.0;
      for (const auto & model : _gap_flux_models)
        flux += model->computeFluxInternal(*this);

      // The Lagrange multiplier _is_ the gap flux
      return (_lambda[_qp] - flux) * _test[_i][_qp];
    }

    default:
      return 0;
  }
}
