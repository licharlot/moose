//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "BoundaryFlux3EqnGhostWall.h"
#include "THMIndices3Eqn.h"

registerMooseObject("ThermalHydraulicsApp", BoundaryFlux3EqnGhostWall);

InputParameters
BoundaryFlux3EqnGhostWall::validParams()
{
  InputParameters params = BoundaryFlux3EqnGhostBase::validParams();

  params.addClassDescription(
      "Wall boundary conditions for the 1-D, 1-phase, variable-area Euler equations");

  return params;
}

BoundaryFlux3EqnGhostWall::BoundaryFlux3EqnGhostWall(const InputParameters & parameters)
  : BoundaryFlux3EqnGhostBase(parameters)
{
}

std::vector<Real>
BoundaryFlux3EqnGhostWall::getGhostCellSolution(const std::vector<Real> & U1) const
{
  std::vector<Real> U_ghost(THM3Eqn::N_CONS_VAR);
  U_ghost[THM3Eqn::CONS_VAR_RHOA] = U1[THM3Eqn::CONS_VAR_RHOA];
  U_ghost[THM3Eqn::CONS_VAR_RHOUA] = -U1[THM3Eqn::CONS_VAR_RHOUA];
  U_ghost[THM3Eqn::CONS_VAR_RHOEA] = U1[THM3Eqn::CONS_VAR_RHOEA];
  U_ghost[THM3Eqn::CONS_VAR_AREA] = U1[THM3Eqn::CONS_VAR_AREA];

  return U_ghost;
}

DenseMatrix<Real>
BoundaryFlux3EqnGhostWall::getGhostCellSolutionJacobian(const std::vector<Real> & /*U1*/) const
{
  DenseMatrix<Real> J(THM3Eqn::N_EQ, THM3Eqn::N_EQ);
  J(THM3Eqn::EQ_MASS, THM3Eqn::EQ_MASS) = 1.0;
  J(THM3Eqn::EQ_MOMENTUM, THM3Eqn::EQ_MOMENTUM) = -1.0;
  J(THM3Eqn::EQ_ENERGY, THM3Eqn::EQ_ENERGY) = 1.0;

  return J;
}
