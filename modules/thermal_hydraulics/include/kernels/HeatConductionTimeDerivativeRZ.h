//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "HeatConductionTimeDerivative.h"
#include "RZSymmetry.h"

/**
 * Time derivative kernel used by heat conduction equation in arbitrary RZ symmetry
 */
class HeatConductionTimeDerivativeRZ : public HeatConductionTimeDerivative, public RZSymmetry
{
public:
  HeatConductionTimeDerivativeRZ(const InputParameters & parameters);

protected:
  virtual Real computeQpResidual();
  virtual Real computeQpJacobian();

public:
  static InputParameters validParams();
};
