//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "OneDIntegratedBC.h"

class VolumeJunction1PhaseUserObject;

/**
 * Adds boundary fluxes for flow channels connected to a 1-phase volume junction
 */
class VolumeJunction1PhaseBC : public OneDIntegratedBC
{
public:
  VolumeJunction1PhaseBC(const InputParameters & params);

protected:
  virtual Real computeQpResidual() override;
  virtual Real computeQpJacobian() override;
  virtual Real computeQpOffDiagJacobian(unsigned int jvar) override;

  /**
   * Creates the mapping of coupled variable index to local equation system
   * index for flow channel variables
   */
  std::map<unsigned int, unsigned int> getFlowChannelIndexMapping() const;

  /**
   * Creates the mapping of coupled variable index to local equation system
   * index for junction variables
   */
  std::map<unsigned int, unsigned int> getJunctionIndexMapping() const;

  /// Index of the connected flow channel
  const unsigned int _connection_index;

  /// 1-phase volume junction user object
  const VolumeJunction1PhaseUserObject & _volume_junction_uo;

  /// Cross-sectional area, elemental
  const VariableValue & _A_elem;
  /// Cross-sectional area, linear
  const VariableValue & _A_linear;

  /// Flow channel rho*A coupled variable index
  const unsigned int _rhoA_jvar;
  /// Flow channel rho*u*A coupled variable index
  const unsigned int _rhouA_jvar;
  /// Flow channel rho*E*A coupled variable index
  const unsigned int _rhoEA_jvar;

  /// Junction rho*V coupled variable index
  const unsigned int _rhoV_jvar;
  /// Junction rho*u*V coupled variable index
  const unsigned int _rhouV_jvar;
  /// Junction rho*v*V coupled variable index
  const unsigned int _rhovV_jvar;
  /// Junction rho*w*V coupled variable index
  const unsigned int _rhowV_jvar;
  /// Junction rho*E*V coupled variable index
  const unsigned int _rhoEV_jvar;

  /// Map of coupled variable index to local equation system index for flow channel variables
  const std::map<unsigned int, unsigned int> _flow_channel_jvar_map;
  /// Map of coupled variable index to local equation system index for junction variables
  const std::map<unsigned int, unsigned int> _junction_jvar_map;
  /// Index within local system of the equation upon which this object acts
  const unsigned int _equation_index;

public:
  static InputParameters validParams();
};
