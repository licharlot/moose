//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "BoundaryFlux3EqnGhostStagnationPressureTemperature.h"
#include "SinglePhaseFluidProperties.h"
#include "THMIndices3Eqn.h"
#include "Numerics.h"

registerMooseObject("ThermalHydraulicsApp", BoundaryFlux3EqnGhostStagnationPressureTemperature);

InputParameters
BoundaryFlux3EqnGhostStagnationPressureTemperature::validParams()
{
  InputParameters params = BoundaryFlux3EqnGhostBase::validParams();

  params.addClassDescription("Computes boundary flux from a specified stagnation pressure and "
                             "temperature for the 1-D, 1-phase, variable-area Euler equations");

  params.addRequiredParam<Real>("p0", "Stagnation pressure");
  params.addRequiredParam<Real>("T0", "Stagnation temperature");
  params.addParam<bool>("reversible", true, "True for reversible, false for pure inlet");

  params.addRequiredParam<UserObjectName>("fluid_properties",
                                          "Name of fluid properties user object");

  params.declareControllable("p0 T0");

  return params;
}

BoundaryFlux3EqnGhostStagnationPressureTemperature::
    BoundaryFlux3EqnGhostStagnationPressureTemperature(const InputParameters & parameters)
  : BoundaryFlux3EqnGhostBase(parameters),

    _p0(getParam<Real>("p0")),
    _T0(getParam<Real>("T0")),
    _reversible(getParam<bool>("reversible")),
    _fp(getUserObject<SinglePhaseFluidProperties>("fluid_properties"))
{
}

std::vector<Real>
BoundaryFlux3EqnGhostStagnationPressureTemperature::getGhostCellSolution(
    const std::vector<Real> & U) const
{
  const Real rhoA = U[THM3Eqn::CONS_VAR_RHOA];
  const Real rhouA = U[THM3Eqn::CONS_VAR_RHOUA];
  const Real A = U[THM3Eqn::CONS_VAR_AREA];

  const Real vel = rhouA / rhoA;

  std::vector<Real> U_ghost(THM3Eqn::N_CONS_VAR);
  if (!_reversible || THM::isInlet(vel, _normal))
  {
    // compute stagnation quantities
    const Real rho0 = _fp.rho_from_p_T(_p0, _T0);
    const Real e0 = _fp.e_from_p_rho(_p0, rho0);
    const Real v0 = 1.0 / rho0;
    const Real h0 = _fp.h_from_p_T(_p0, _T0);
    const Real s0 = _fp.s_from_v_e(v0, e0);

    // compute static quantities
    const Real h = h0 - 0.5 * vel * vel;
    const Real s = s0;
    const Real p = _fp.p_from_h_s(h, s);
    const Real rho = _fp.rho_from_p_s(p, s);
    const Real e = _fp.e_from_p_rho(p, rho);
    const Real E = e + 0.5 * vel * vel;

    U_ghost[THM3Eqn::CONS_VAR_RHOA] = rho * A;
    U_ghost[THM3Eqn::CONS_VAR_RHOUA] = rho * vel * A;
    U_ghost[THM3Eqn::CONS_VAR_RHOEA] = rho * E * A;
    U_ghost[THM3Eqn::CONS_VAR_AREA] = A;
  }
  else
  {
    const Real rho = rhoA / A;
    const Real E = _fp.e_from_p_rho(_p0, rho) + 0.5 * vel * vel;

    U_ghost[THM3Eqn::CONS_VAR_RHOA] = rhoA;
    U_ghost[THM3Eqn::CONS_VAR_RHOUA] = rhouA;
    U_ghost[THM3Eqn::CONS_VAR_RHOEA] = rhoA * E;
    U_ghost[THM3Eqn::CONS_VAR_AREA] = A;
  }

  return U_ghost;
}

DenseMatrix<Real>
BoundaryFlux3EqnGhostStagnationPressureTemperature::getGhostCellSolutionJacobian(
    const std::vector<Real> & U) const
{
  const Real rhoA = U[THM3Eqn::CONS_VAR_RHOA];
  const Real rhouA = U[THM3Eqn::CONS_VAR_RHOUA];
  const Real A = U[THM3Eqn::CONS_VAR_AREA];

  const Real vel = rhouA / rhoA;

  DenseMatrix<Real> J(THM3Eqn::N_EQ, THM3Eqn::N_EQ);
  if (!_reversible || THM::isInlet(vel, _normal))
  {
    Real vel, dvel_drhoA, dvel_drhouA;
    THM::vel_from_arhoA_arhouA(rhoA, rhouA, vel, dvel_drhoA, dvel_drhouA);

    // compute stagnation quantities
    const Real rho0 = _fp.rho_from_p_T(_p0, _T0);
    const Real e0 = _fp.e_from_p_rho(_p0, rho0);
    const Real v0 = 1.0 / rho0;
    const Real h0 = _fp.h_from_p_T(_p0, _T0);
    const Real s0 = _fp.s_from_v_e(v0, e0);

    // compute static quantities

    const Real h = h0 - 0.5 * vel * vel;
    const Real dh_drhoA = -vel * dvel_drhoA;
    const Real dh_drhouA = -vel * dvel_drhouA;

    const Real s = s0;

    Real p, dp_dh, dp_ds;
    _fp.p_from_h_s(h, s, p, dp_dh, dp_ds);
    const Real dp_drhoA = dp_dh * dh_drhoA;
    const Real dp_drhouA = dp_dh * dh_drhouA;

    Real rho, drho_dp, drho_ds;
    _fp.rho_from_p_s(p, s, rho, drho_dp, drho_ds);
    const Real drho_drhoA = drho_dp * dp_drhoA;
    const Real drho_drhouA = drho_dp * dp_drhouA;

    Real e, de_dp, de_drho;
    _fp.e_from_p_rho(p, rho, e, de_dp, de_drho);
    const Real de_drhoA = de_dp * dp_drhoA + de_drho * drho_drhoA;
    const Real de_drhouA = de_dp * dp_drhouA + de_drho * drho_drhouA;

    const Real E = e + 0.5 * vel * vel;
    const Real dE_drhoA = de_drhoA + vel * dvel_drhoA;
    const Real dE_drhouA = de_drhouA + vel * dvel_drhouA;

    J(THM3Eqn::EQ_MASS, THM3Eqn::EQ_MASS) = drho_drhoA * A;
    J(THM3Eqn::EQ_MASS, THM3Eqn::EQ_MOMENTUM) = drho_drhouA * A;

    J(THM3Eqn::EQ_MOMENTUM, THM3Eqn::EQ_MASS) = (drho_drhoA * vel + rho * dvel_drhoA) * A;
    J(THM3Eqn::EQ_MOMENTUM, THM3Eqn::EQ_MOMENTUM) = (drho_drhouA * vel + rho * dvel_drhouA) * A;

    J(THM3Eqn::EQ_ENERGY, THM3Eqn::EQ_MASS) = (drho_drhoA * E + rho * dE_drhoA) * A;
    J(THM3Eqn::EQ_ENERGY, THM3Eqn::EQ_MOMENTUM) = (drho_drhouA * E + rho * dE_drhouA) * A;
  }
  else
  {
    const Real rho = rhoA / A;
    const Real drho_drhoA = 1.0 / A;

    Real vel, dvel_drhoA, dvel_drhouA;
    THM::vel_from_arhoA_arhouA(rhoA, rhouA, vel, dvel_drhoA, dvel_drhouA);

    Real e, de_dp, de_drho;
    _fp.e_from_p_rho(_p0, rho, e, de_dp, de_drho);
    const Real de_drhoA = de_drho * drho_drhoA;

    const Real E = _fp.e_from_p_rho(_p0, rho) + 0.5 * vel * vel;
    const Real dE_dvel = vel;
    const Real dE_drhoA = de_drhoA + dE_dvel * dvel_drhoA;
    const Real dE_drhouA = dE_dvel * dvel_drhouA;

    J(THM3Eqn::EQ_MASS, THM3Eqn::EQ_MASS) = 1.0;

    J(THM3Eqn::EQ_MOMENTUM, THM3Eqn::EQ_MOMENTUM) = 1.0;

    J(THM3Eqn::EQ_ENERGY, THM3Eqn::EQ_MASS) = E + rhoA * dE_drhoA;
    J(THM3Eqn::EQ_ENERGY, THM3Eqn::EQ_MOMENTUM) = rhoA * dE_drhouA;
  }

  return J;
}
