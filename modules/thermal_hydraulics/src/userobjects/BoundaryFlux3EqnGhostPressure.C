//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "BoundaryFlux3EqnGhostPressure.h"
#include "SinglePhaseFluidProperties.h"
#include "THMIndices3Eqn.h"
#include "Numerics.h"

registerMooseObject("ThermalHydraulicsApp", BoundaryFlux3EqnGhostPressure);

InputParameters
BoundaryFlux3EqnGhostPressure::validParams()
{
  InputParameters params = BoundaryFlux3EqnGhostBase::validParams();

  params.addClassDescription("Computes boundary flux from a specified pressure for the 1-D, "
                             "1-phase, variable-area Euler equations");

  params.addRequiredParam<Real>("p", "Pressure");

  params.addRequiredParam<UserObjectName>("fluid_properties",
                                          "Name of fluid properties user object");

  params.declareControllable("p");
  return params;
}

BoundaryFlux3EqnGhostPressure::BoundaryFlux3EqnGhostPressure(const InputParameters & parameters)
  : BoundaryFlux3EqnGhostBase(parameters),

    _p(getParam<Real>("p")),
    _fp(getUserObject<SinglePhaseFluidProperties>("fluid_properties"))
{
}

std::vector<Real>
BoundaryFlux3EqnGhostPressure::getGhostCellSolution(const std::vector<Real> & U) const
{
  const Real rhoA = U[THM3Eqn::CONS_VAR_RHOA];
  const Real rhouA = U[THM3Eqn::CONS_VAR_RHOUA];
  const Real A = U[THM3Eqn::CONS_VAR_AREA];

  const Real rho = rhoA / A;
  const Real vel = rhouA / rhoA;
  const Real E = _fp.e_from_p_rho(_p, rho) + 0.5 * vel * vel;

  std::vector<Real> U_ghost(THM3Eqn::N_CONS_VAR);
  U_ghost[THM3Eqn::CONS_VAR_RHOA] = rhoA;
  U_ghost[THM3Eqn::CONS_VAR_RHOUA] = rhouA;
  U_ghost[THM3Eqn::CONS_VAR_RHOEA] = rhoA * E;
  U_ghost[THM3Eqn::CONS_VAR_AREA] = A;

  return U_ghost;
}

DenseMatrix<Real>
BoundaryFlux3EqnGhostPressure::getGhostCellSolutionJacobian(const std::vector<Real> & U) const
{
  const Real rhoA = U[THM3Eqn::CONS_VAR_RHOA];
  const Real rhouA = U[THM3Eqn::CONS_VAR_RHOUA];
  const Real A = U[THM3Eqn::CONS_VAR_AREA];

  const Real rho = rhoA / A;
  const Real drho_drhoA = 1.0 / A;

  Real vel, dvel_drhoA, dvel_drhouA;
  THM::vel_from_arhoA_arhouA(rhoA, rhouA, vel, dvel_drhoA, dvel_drhouA);

  Real e, de_dp, de_drho;
  _fp.e_from_p_rho(_p, rho, e, de_dp, de_drho);
  const Real de_drhoA = de_drho * drho_drhoA;

  const Real E = _fp.e_from_p_rho(_p, rho) + 0.5 * vel * vel;
  const Real dE_dvel = vel;
  const Real dE_drhoA = de_drhoA + dE_dvel * dvel_drhoA;
  const Real dE_drhouA = dE_dvel * dvel_drhouA;

  DenseMatrix<Real> J(THM3Eqn::N_EQ, THM3Eqn::N_EQ);

  J(THM3Eqn::EQ_MASS, THM3Eqn::EQ_MASS) = 1.0;

  J(THM3Eqn::EQ_MOMENTUM, THM3Eqn::EQ_MOMENTUM) = 1.0;

  J(THM3Eqn::EQ_ENERGY, THM3Eqn::EQ_MASS) = E + rhoA * dE_drhoA;
  J(THM3Eqn::EQ_ENERGY, THM3Eqn::EQ_MOMENTUM) = rhoA * dE_drhouA;

  return J;
}
