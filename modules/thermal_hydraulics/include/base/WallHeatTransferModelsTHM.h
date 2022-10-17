//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "libmesh/libmesh_common.h"
#include "ADReal.h"

using namespace libMesh;

namespace WallHeatTransferTHM
{

/**
 * Computes Nusselt number using Dittus-Boelter correlation
 *
 * @param[in] Re Reynolds number
 * @param[in] Pr Prandtl number
 * @param[in] T  Fluid bulk temperature
 * @param[in] T_wall  Wall temperature
 */
ADReal NusseltDittusBoelter(ADReal Re, ADReal Pr, ADReal T, ADReal T_wall);

/**
 * Computes Nusselt number using a modified Dittus-Boelter correlation
 * See McEligot D.M. et al., "Investigation of Fundamental Thermal-Hydraulic Phenomena in Advanced
 * Gas-Cooled Reactors," INL/EXT-06-11801, September 2006, equation (2-4)
 *
 * Valid for x/D > 30, 4000 < Re < 15000, 0 < q* < 0.004, where q* is "nondimensional heat flux",
 * defined as q" / (G h), where G is mass flux [kg/(m2 -s)] and h is specific enthalpy.
 *
 * @param[in] Re Reynolds number
 * @param[in] Pr Prandtl number
 * @param[in] T  Fluid bulk temperature
 * @param[in] T_wall  Wall temperature
 */
ADReal NusseltModifiedDittusBoelter(ADReal Re, ADReal Pr, ADReal T, ADReal T_wall);
}
