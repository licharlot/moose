//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "OneDIntegratedBC.h"
#include "DerivativeMaterialInterfaceTHM.h"

/**
 *
 */
class OneDMomentumStaticPressureBC : public DerivativeMaterialInterfaceTHM<OneDIntegratedBC>
{
public:
  OneDMomentumStaticPressureBC(const InputParameters & parameters);

protected:
  virtual bool shouldApply();
  virtual Real computeQpResidual();
  virtual Real computeQpJacobian();
  virtual Real computeQpOffDiagJacobian(unsigned int jvar);

  bool _reversible;
  unsigned int _rhoA_var_number;
  unsigned int _rhoEA_var_number;

  // Coupled variables
  const VariableValue & _area;
  const VariableValue & _rhoA;
  const VariableValue & _vel_old;
  // Required parameters
  const Real & _p_in;

public:
  static InputParameters validParams();
};
