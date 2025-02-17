//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "GeneralPostprocessor.h"

class NumElems;

template <>
InputParameters validParams<NumElems>();

class NumElems : public GeneralPostprocessor
{
public:
  static InputParameters validParams();

  NumElems(const InputParameters & parameters);

  virtual void initialize() override {}
  virtual void execute() override {}

  virtual Real getValue() override;

private:
  enum class ElemFilter
  {
    ACTIVE,
    TOTAL,
  };

  const ElemFilter _filt;

  const MeshBase & _mesh;
};
