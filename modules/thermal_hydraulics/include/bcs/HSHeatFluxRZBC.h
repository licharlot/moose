//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "HSHeatFluxBC.h"
#include "RZSymmetry.h"

/**
 * Applies a specified heat flux to the side of a cylindrical heat structure
 */
class HSHeatFluxRZBC : public HSHeatFluxBC, public RZSymmetry
{
public:
  HSHeatFluxRZBC(const InputParameters & parameters);

protected:
  virtual Real computeQpResidual() override;

public:
  static InputParameters validParams();
};
