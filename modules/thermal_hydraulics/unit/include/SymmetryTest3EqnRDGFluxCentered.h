//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "SymmetryTest3EqnRDGFluxBase.h"
#include "NumericalFlux3EqnCentered.h"

/**
 * Tests symmetry of the centered numerical flux for the 3-equation model.
 */
class SymmetryTest3EqnRDGFluxCentered : public SymmetryTest3EqnRDGFluxBase
{
protected:
  virtual const NumericalFlux3EqnBase * createFluxObject() override
  {
    const std::string class_name = "NumericalFlux3EqnCentered";
    InputParameters params = _factory.getValidParams(class_name);
    params.set<UserObjectName>("fluid_properties") = _fp_name;
    _fe_problem->addUserObject(class_name, class_name, params);
    return static_cast<const NumericalFlux3EqnBase *>(
        &_fe_problem->getUserObject<NumericalFlux3EqnCentered>(class_name));
  }

  virtual std::vector<std::pair<std::vector<Real>, std::vector<Real>>>
  getPrimitiveSolutionPairs() const override;
};
