//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "Test3EqnRDGObjectBase.h"
#include "THMIndices3Eqn.h"

std::vector<Real>
Test3EqnRDGObjectBase::computeConservativeSolution(const std::vector<Real> & W,
                                                   const Real & A) const
{
  const Real & p = W[0];
  const Real & T = W[1];
  const Real & vel = W[2];

  const Real rho = _fp.rho_from_p_T(p, T);
  const Real e = _fp.e_from_p_rho(p, rho);
  const Real E = e + 0.5 * vel * vel;

  std::vector<Real> U(THM3Eqn::N_CONS_VAR, 0.0);
  U[THM3Eqn::CONS_VAR_RHOA] = rho * A;
  U[THM3Eqn::CONS_VAR_RHOUA] = rho * vel * A;
  U[THM3Eqn::CONS_VAR_RHOEA] = rho * E * A;
  U[THM3Eqn::CONS_VAR_AREA] = A;

  return U;
}
