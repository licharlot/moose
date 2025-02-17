//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "INSFVMomentumBoussinesq.h"
#include "NS.h"

registerMooseObject("NavierStokesApp", INSFVMomentumBoussinesq);

InputParameters
INSFVMomentumBoussinesq::validParams()
{
  InputParameters params = FVElementalKernel::validParams();
  params.addClassDescription("Computes a body force for natural convection buoyancy.");
  params.addRequiredParam<MooseFunctorName>(NS::T_fluid, "the fluid temperature");
  params.addRequiredParam<RealVectorValue>("gravity", "Direction of the gravity vector");
  params.addParam<MooseFunctorName>("alpha_name",
                                    NS::alpha_boussinesq,
                                    "The name of the thermal expansion coefficient"
                                    "this is of the form rho = rho*(1-alpha (T-T_ref))");
  params.addRequiredParam<Real>("ref_temperature", "The value for the reference temperature.");
  MooseEnum momentum_component("x=0 y=1 z=2");
  params.addRequiredParam<MooseEnum>(
      "momentum_component",
      momentum_component,
      "The component of the momentum equation that this kernel applies to.");
  params.addRequiredParam<Real>("rho", "The value for the density");
  params.declareControllable("rho");
  return params;
}

INSFVMomentumBoussinesq::INSFVMomentumBoussinesq(const InputParameters & params)
  : FVElementalKernel(params),
    _temperature(getFunctor<ADReal>(NS::T_fluid)),
    _gravity(getParam<RealVectorValue>("gravity")),
    _alpha(getFunctor<ADReal>("alpha_name")),
    _ref_temperature(getParam<Real>("ref_temperature")),
    _rho(getParam<Real>(NS::density)),
    _index(getParam<MooseEnum>("momentum_component"))
{
#ifndef MOOSE_GLOBAL_AD_INDEXING
  mooseError("INSFV is not supported by local AD indexing. In order to use INSFV, please run the "
             "configure script in the root MOOSE directory with the configure option "
             "'--with-ad-indexing-type=global'");
#endif
}

ADReal
INSFVMomentumBoussinesq::computeQpResidual()
{
  return _alpha(_current_elem) * _gravity(_index) * _rho *
         (_temperature(_current_elem) - _ref_temperature);
}
