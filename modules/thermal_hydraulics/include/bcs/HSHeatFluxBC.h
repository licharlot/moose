//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "FunctionNeumannBC.h"

/**
 * Applies a specified heat flux to the side of a plate heat structure
 */
class HSHeatFluxBC : public FunctionNeumannBC
{
public:
  HSHeatFluxBC(const InputParameters & parameters);

  virtual Real computeQpResidual() override;

  /// Post-processor by which to scale boundary condition
  const PostprocessorValue & _scale_pp;

public:
  static InputParameters validParams();
};
