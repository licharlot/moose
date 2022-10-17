//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "Material.h"

/**
 * Computes wall heat transfer coefficient for gas at high temperatures using McEligot et al. (1966)
 * correlation
 *
 * See McEligot D.M. et al., "Investigation of Fundamental Thermal-Hydraulic Phenomena in Advanced
 * Gas-Cooled Reactors," INL/EXT-06-11801, September 2006, equation (2-4)
 *
 * Valid for x/D > 30, 4000 < Re < 15000, 0 < q* < 0.004, where q* is "nondimensional heat flux",
 * defined as q" / (G h), where G is mass flux [kg/(m2 -s)] and h is specific enthalpy.
 */

class ADWallHeatTransferCoefficient3EqnModifiedDittusBoelterMaterial : public Material
{
public:
  ADWallHeatTransferCoefficient3EqnModifiedDittusBoelterMaterial(
      const InputParameters & parameters);

protected:
  virtual void computeQpProperties();

  /// Wall heat transfer coefficient
  ADMaterialProperty<Real> & _Hw;
  /// Density
  const ADMaterialProperty<Real> & _rho;
  /// Velocity
  const ADMaterialProperty<Real> & _vel;
  /// Hydraulic diameter
  const ADMaterialProperty<Real> & _D_h;
  /// Heat conduction
  const ADMaterialProperty<Real> & _k;
  /// Dynamic viscosity
  const ADMaterialProperty<Real> & _mu;
  /// Dynamic viscosity
  const ADMaterialProperty<Real> & _cp;
  /// Fluid temperature
  const ADMaterialProperty<Real> & _T;
  /// Wall temperature
  const ADMaterialProperty<Real> & _T_wall;

public:
  static InputParameters validParams();
};
