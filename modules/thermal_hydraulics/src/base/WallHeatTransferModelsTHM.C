//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "WallHeatTransferModelsTHM.h"
#include "Numerics.h"
#include "ADReal.h"

namespace WallHeatTransferTHM
{

ADReal
NusseltDittusBoelter(ADReal Re, ADReal Pr, ADReal T, ADReal T_wall)
{
  ADReal n = (T < T_wall) ? 0.4 : 0.3;
  return 0.023 * std::pow(Re, 4. / 5.) * std::pow(Pr, n);
}

ADReal
NusseltModifiedDittusBoelter(ADReal Re, ADReal Pr, ADReal T, ADReal T_wall)
{
  return 0.021 * std::pow(Re, 0.8) * std::pow(Pr, 0.4) * std::pow(T_wall / T, -0.5);
}
}
