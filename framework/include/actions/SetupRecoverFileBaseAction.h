//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

// MOOSE includes
#include "Action.h"

// Forward declerations
class SetupRecoverFileBaseAction;

template <>
InputParameters validParams<SetupRecoverFileBaseAction>();

/**
 *
 */
class SetupRecoverFileBaseAction : public Action
{
public:
  /**
   * Class constructor
   * @param params Input parameters for this action
   */
  static InputParameters validParams();

  SetupRecoverFileBaseAction(InputParameters params);

  virtual void act() override;
};
