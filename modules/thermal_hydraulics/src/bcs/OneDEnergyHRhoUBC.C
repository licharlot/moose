//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "OneDEnergyHRhoUBC.h"

registerMooseObject("ThermalHydraulicsApp", OneDEnergyHRhoUBC);

InputParameters
OneDEnergyHRhoUBC::validParams()
{
  InputParameters params = OneDIntegratedBC::validParams();
  params.addRequiredParam<Real>("H", "Specified enthalpy");
  params.addRequiredParam<Real>("rhou", "Specified momentum");
  params.addRequiredCoupledVar("A", "Area");

  params.declareControllable("H rhou");

  return params;
}

OneDEnergyHRhoUBC::OneDEnergyHRhoUBC(const InputParameters & parameters)
  : DerivativeMaterialInterfaceTHM<OneDIntegratedBC>(parameters),
    _H(getParam<Real>("H")),
    _rhou(getParam<Real>("rhou")),
    _area(coupledValue("A"))
{
}

Real
OneDEnergyHRhoUBC::computeQpResidual()
{
  return _rhou * _H * _area[_qp] * _normal * _test[_i][_qp];
}

Real
OneDEnergyHRhoUBC::computeQpJacobian()
{
  return 0;
}

Real
OneDEnergyHRhoUBC::computeQpOffDiagJacobian(unsigned int)
{
  return 0;
}
