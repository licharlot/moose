//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "BoundaryFlux3EqnGhostBase.h"
#include "NumericalFlux3EqnBase.h"

InputParameters
BoundaryFlux3EqnGhostBase::validParams()
{
  InputParameters params = BoundaryFluxBase::validParams();

  params.addClassDescription("Computes boundary fluxes for the 1-D, variable-area Euler equations "
                             "using a numerical flux user object and a ghost cell solution");

  params.addRequiredParam<UserObjectName>("numerical_flux", "Name of numerical flux user object");
  params.addRequiredParam<Real>("normal", "Outward normal");

  return params;
}

BoundaryFlux3EqnGhostBase::BoundaryFlux3EqnGhostBase(const InputParameters & parameters)
  : BoundaryFluxBase(parameters),
    _numerical_flux(getUserObject<NumericalFlux3EqnBase>("numerical_flux")),
    _normal(getParam<Real>("normal"))
{
}

void
BoundaryFlux3EqnGhostBase::calcFlux(unsigned int iside,
                                    dof_id_type ielem,
                                    const std::vector<Real> & U1,
                                    const RealVectorValue & normal,
                                    std::vector<Real> & flux) const
{
  const std::vector<Real> U2 = getGhostCellSolution(U1);

  flux = _numerical_flux.getFlux(iside, ielem, true, U1, U2, normal(0));
}

void
BoundaryFlux3EqnGhostBase::calcJacobian(unsigned int iside,
                                        dof_id_type ielem,
                                        const std::vector<Real> & U1,
                                        const RealVectorValue & normal,
                                        DenseMatrix<Real> & J) const
{
  const std::vector<Real> U2 = getGhostCellSolution(U1);

  const auto & pdF_pdU1 = _numerical_flux.getJacobian(true, true, iside, ielem, U1, U2, normal(0));
  const auto & dF_dU2 = _numerical_flux.getJacobian(true, false, iside, ielem, U1, U2, normal(0));

  // compute dF/dU1 = pdF/pdU1 + dF/dU2 * dU2/dU1
  J = getGhostCellSolutionJacobian(U1);
  J.left_multiply(dF_dU2);
  J += pdF_pdU1;
}
