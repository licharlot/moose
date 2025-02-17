//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "Action.h"

class EmptyAction;

template <>
InputParameters validParams<EmptyAction>();

/**
 * Do nothing action.
 */
class EmptyAction : public Action
{
public:
  static InputParameters validParams();

  EmptyAction(InputParameters params);

  virtual void act() override;
};
