//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "GateValve1PhaseBC.h"
#include "GateValve1PhaseUserObject.h"
#include "THMIndices3Eqn.h"
#include "Assembly.h"

registerMooseObject("ThermalHydraulicsApp", GateValve1PhaseBC);

InputParameters
GateValve1PhaseBC::validParams()
{
  InputParameters params = OneDIntegratedBC::validParams();

  params.addRequiredParam<unsigned int>("connection_index", "Index of the connected flow channel");
  params.addRequiredParam<UserObjectName>("gate_valve_uo", "1-phase gate valve user object");

  params.addRequiredCoupledVar("A_elem", "Cross-sectional area, elemental");
  params.addRequiredCoupledVar("A_linear", "Cross-sectional area, linear");

  params.addRequiredCoupledVar("rhoA", "Flow channel variable: rho*A");
  params.addRequiredCoupledVar("rhouA", "Flow channel variable: rho*u*A");
  params.addRequiredCoupledVar("rhoEA", "Flow channel variable: rho*E*A");

  params.addClassDescription(
      "Adds boundary fluxes for flow channels connected to a 1-phase gate valve");

  return params;
}

GateValve1PhaseBC::GateValve1PhaseBC(const InputParameters & params)
  : OneDIntegratedBC(params),

    _connection_index(getParam<unsigned int>("connection_index")),
    _gate_valve_uo(getUserObject<GateValve1PhaseUserObject>("gate_valve_uo")),

    _A_elem(coupledValue("A_elem")),
    _A_linear(coupledValue("A_linear")),

    _rhoA_jvar(coupled("rhoA")),
    _rhouA_jvar(coupled("rhouA")),
    _rhoEA_jvar(coupled("rhoEA")),

    _jvar_map(getIndexMapping()),
    _equation_index(_jvar_map.at(_var.number()))
{
}

Real
GateValve1PhaseBC::computeQpResidual()
{
  const auto & flux = _gate_valve_uo.getFlux(_connection_index);

  // Note that the ratio A_linear / A_elem is necessary because A_elem is passed
  // to the flux function, but A_linear is to be used on the boundary.
  return flux[_equation_index] * _A_linear[_qp] / _A_elem[_qp] * _normal * _test[_i][_qp];
}

Real
GateValve1PhaseBC::computeQpJacobian()
{
  mooseError("Not implemented.");
}

Real
GateValve1PhaseBC::computeQpOffDiagJacobian(unsigned int /*jvar*/)
{
  mooseError("Not implemented.");
}

void
GateValve1PhaseBC::computeJacobian()
{
  mooseError("Not implemented. Ensure you use preconditioner with type 'SMP'.");
}

void
GateValve1PhaseBC::computeOffDiagJacobian(const unsigned int jvar_num)
{
  DenseMatrix<Real> jacobian_block;
  std::vector<dof_id_type> dofs_j;
  std::map<unsigned int, unsigned int>::const_iterator it;
  if ((it = _jvar_map.find(jvar_num)) != _jvar_map.end())
  {
    const unsigned int j_equation_index = it->second;
    _gate_valve_uo.getJacobianEntries(
        _connection_index, _equation_index, j_equation_index, jacobian_block, dofs_j);

    const auto & jvar = getVariable(jvar_num);

    // It is assumed here that _phi[_j][_qp] will give the correct value for DoFs
    // on the other subdomain. For 1-D, there should be only one basis function
    // on the boundary, and its value should be equal to 1, so this assumption
    // should remain valid.
    Real mult_factor = 0;
    for (_qp = 0; _qp < _qrule->n_points(); _qp++)
      for (_i = 0; _i < _test.size(); _i++)
        for (_j = 0; _j < jvar.phiFaceSize(); _j++)
          mult_factor += _A_linear[_qp] / _A_elem[_qp] * _normal * _test[_i][_qp] * _phi[_j][_qp];
    jacobian_block *= mult_factor;

    auto && dofs_i = _var.dofIndices();
    mooseAssert(dofs_i.size() == 1, "There should be only one DoF index.");

    _assembly.cacheJacobianBlock(jacobian_block, dofs_i, dofs_j, _var.scalingFactor());
  }
}

std::map<unsigned int, unsigned int>
GateValve1PhaseBC::getIndexMapping() const
{
  std::map<unsigned int, unsigned int> jvar_map;
  jvar_map.insert(std::pair<unsigned int, unsigned int>(_rhoA_jvar, THM3Eqn::EQ_MASS));
  jvar_map.insert(std::pair<unsigned int, unsigned int>(_rhouA_jvar, THM3Eqn::EQ_MOMENTUM));
  jvar_map.insert(std::pair<unsigned int, unsigned int>(_rhoEA_jvar, THM3Eqn::EQ_ENERGY));

  return jvar_map;
}
