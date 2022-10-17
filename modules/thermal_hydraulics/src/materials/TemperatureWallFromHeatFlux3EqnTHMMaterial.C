//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "TemperatureWallFromHeatFlux3EqnTHMMaterial.h"
#include "Numerics.h"
registerMooseObject("ThermalHydraulicsApp", TemperatureWallFromHeatFlux3EqnTHMMaterial);

InputParameters
TemperatureWallFromHeatFlux3EqnTHMMaterial::validParams()
{
  InputParameters params = Material::validParams();
  params.addRequiredParam<MaterialPropertyName>("T_wall", "Wall temperature");
  params.addRequiredParam<MaterialPropertyName>("q_wall", "Wall heat flux");
  params.addRequiredParam<MaterialPropertyName>("rho", "Density of the liquid");
  params.addRequiredParam<MaterialPropertyName>("vel", "x-component of the liquid velocity");
  params.addRequiredParam<MaterialPropertyName>("D_h", "Hydraulic diameter");
  params.addRequiredParam<MaterialPropertyName>("cp", "Specific heat of the fluid");
  params.addRequiredParam<MaterialPropertyName>("mu", "Dynamic viscosity of the fluid");
  params.addRequiredParam<MaterialPropertyName>("k", "Heat conductivity of the fluid");
  params.addRequiredParam<MaterialPropertyName>("T", "Fluid temperature");

  return params;
}

TemperatureWallFromHeatFlux3EqnTHMMaterial::TemperatureWallFromHeatFlux3EqnTHMMaterial(
    const InputParameters & parameters)
  : Material(parameters),
    _T_wall(declareADProperty<Real>("T_wall")),
    _rho(getADMaterialProperty<Real>("rho")),
    _vel(getADMaterialProperty<Real>("vel")),
    _D_h(getADMaterialProperty<Real>("D_h")),
    _k(getADMaterialProperty<Real>("k")),
    _mu(getADMaterialProperty<Real>("mu")),
    _cp(getADMaterialProperty<Real>("cp")),
    _q_wall(getADMaterialProperty<Real>("q_wall")),
    _T(getADMaterialProperty<Real>("T"))
{
}

void
TemperatureWallFromHeatFlux3EqnTHMMaterial::computeQpProperties()
{
  // initial guess
  ADReal T_wall = _T[_qp];
  ADReal Pr = THM::Prandtl(_cp[_qp], _mu[_qp], _k[_qp]);
  ADReal Re = std::max(1.0, THM::Reynolds(1., _rho[_qp], _vel[_qp], _D_h[_qp], _mu[_qp]));

  ADReal T_wall_prev;
  ADReal htc_wall;
  for (unsigned int iter = 1; iter < 25; iter++)
  {
    T_wall_prev = T_wall;

    ADReal Nu = WallHeatTransferTHM::NusseltModifiedDittusBoelter(Re, Pr, _T[_qp], _T_wall[_qp]);
    htc_wall = THM::wallHeatTransferCoefficient(Nu, _k[_qp], _D_h[_qp]);

    mooseAssert(std::abs(htc_wall) > 1e-14,
                "Wall HTC is approximately zero; this is typically caused by a zero velocity.");

    T_wall = _q_wall[_qp] / htc_wall + _T[_qp];

    if (std::abs(T_wall - T_wall_prev) < 0.01)
    {
      _T_wall[_qp] = T_wall;
      return;
    }
  }

  mooseError(name(), ": Wall temperature iteration did not converge.");
}
