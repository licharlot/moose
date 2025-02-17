//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "OneDEnergyStaticPressureSupersonicBC.h"
#include "SinglePhaseFluidProperties.h"
#include "Numerics.h"

registerMooseObject("ThermalHydraulicsApp", OneDEnergyStaticPressureSupersonicBC);

InputParameters
OneDEnergyStaticPressureSupersonicBC::validParams()
{
  InputParameters params = OneDEnergyFreeBC::validParams();
  params.addParam<bool>("reversible", false, "If the BC is reversible.");
  params.addRequiredCoupledVar("v", "Specific volume");
  params.addRequiredCoupledVar("e", "Specific internal energy");
  params.addRequiredParam<MaterialPropertyName>("c", "Sound speed material property");
  params.addRequiredParam<UserObjectName>("fp", "The name of the fluid property user object");
  return params;
}

OneDEnergyStaticPressureSupersonicBC::OneDEnergyStaticPressureSupersonicBC(
    const InputParameters & parameters)
  : OneDEnergyFreeBC(parameters),
    _reversible(getParam<bool>("reversible")),
    _vel_old(coupledValueOld("vel")),
    _v_old(coupledValueOld("v")),
    _e_old(coupledValueOld("e")),
    _c(getMaterialProperty<Real>("c")),
    _fp(getUserObject<SinglePhaseFluidProperties>("fp"))
{
}

bool
OneDEnergyStaticPressureSupersonicBC::shouldApply()
{
  if (!_reversible && THM::isOutlet(_vel_old[0], _normal))
  {
    // use old value to prevent BC from oscillating around M = 1 and failing to solve
    Real M = _vel_old[0] / _fp.c_from_v_e(_v_old[0], _e_old[0]);
    return (M > 1);
  }
  else
    return false;
}
