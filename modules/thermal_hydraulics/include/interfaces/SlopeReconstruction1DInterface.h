//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "MooseObject.h"
#include "MooseEnum.h"
#include "MooseTypes.h"
#include "InputParameters.h"
#include "THMEnums.h"

#include "libmesh/elem.h"
#include "libmesh/vector_value.h"
#include "libmesh/point.h"

/**
 * Interface class for 1-D slope reconstruction
 *
 * This class provides interfaces for generating slopes for an arbitrary set
 * of variables. A number of reconstruction options are provided, including a
 * number of TVD slope limiters.
 */
template <bool is_ad>
class SlopeReconstruction1DInterface
{
public:
  SlopeReconstruction1DInterface(const MooseObject * moose_object);

  /// Slope reconstruction type
  enum ESlopeReconstructionType
  {
    None,    ///< No reconstruction; Godunov scheme
    Full,    ///< Full reconstruction; no limitation
    Minmod,  ///< Minmod slope limiter
    MC,      ///< Monotonized Central-Difference slope limiter
    Superbee ///< Superbee slope limiter
  };
  /// Map of slope reconstruction type string to enum
  static const std::map<std::string, ESlopeReconstructionType> _slope_reconstruction_type_to_enum;

  /**
   * Gets a MooseEnum for slope reconstruction type
   *
   * @param[in] name   default value
   * @returns MooseEnum for slope reconstruction type
   */
  static MooseEnum getSlopeReconstructionMooseEnum(const std::string & name = "");

protected:
  /**
   * Gets limited slopes for the primitive variables in the 1-D direction
   *
   * @param[in] elem   Element for which to get slopes
   *
   * @returns Vector of slopes for the element in the 1-D direction
   */
  std::vector<GenericReal<is_ad>> getElementSlopes(const Elem * elem) const;

  /**
   * Computes the cell-average primitive variable values for an element
   *
   * @param[in] elem   Element for which to get values
   *
   * @returns Vector of values on element
   */
  virtual std::vector<GenericReal<is_ad>>
  computeElementPrimitiveVariables(const Elem * elem) const = 0;

  /// Slope reconstruction scheme
  const ESlopeReconstructionType _scheme;

public:
  static InputParameters validParams();

protected:
  /// Number of sides
  static const unsigned int _n_side;
  /// Number of elemental values in stencil for computing slopes
  static const unsigned int _n_sten;
};

namespace THM
{
template <>
SlopeReconstruction1DInterface<true>::ESlopeReconstructionType
stringToEnum<SlopeReconstruction1DInterface<true>::ESlopeReconstructionType>(const std::string & s);

template <>
SlopeReconstruction1DInterface<false>::ESlopeReconstructionType
stringToEnum<SlopeReconstruction1DInterface<false>::ESlopeReconstructionType>(
    const std::string & s);
}

template <bool is_ad>
const std::map<std::string,
               typename SlopeReconstruction1DInterface<is_ad>::ESlopeReconstructionType>
    SlopeReconstruction1DInterface<is_ad>::_slope_reconstruction_type_to_enum{
        {"NONE", None}, {"FULL", Full}, {"MINMOD", Minmod}, {"MC", MC}, {"SUPERBEE", Superbee}};

template <bool is_ad>
MooseEnum
SlopeReconstruction1DInterface<is_ad>::getSlopeReconstructionMooseEnum(const std::string & name)
{
  return THM::getMooseEnum<SlopeReconstruction1DInterface<is_ad>::ESlopeReconstructionType>(
      name, _slope_reconstruction_type_to_enum);
}

template <bool is_ad>
const unsigned int SlopeReconstruction1DInterface<is_ad>::_n_side = 2;

template <bool is_ad>
const unsigned int SlopeReconstruction1DInterface<is_ad>::_n_sten = 3;

template <bool is_ad>
InputParameters
SlopeReconstruction1DInterface<is_ad>::validParams()
{
  InputParameters params = emptyInputParameters();

  params.addRequiredParam<MooseEnum>(
      "scheme",
      SlopeReconstruction1DInterface::getSlopeReconstructionMooseEnum("None"),
      "Slope reconstruction scheme");

  return params;
}

template <bool is_ad>
SlopeReconstruction1DInterface<is_ad>::SlopeReconstruction1DInterface(
    const MooseObject * moose_object)
  : _scheme(THM::stringToEnum<ESlopeReconstructionType>(
        moose_object->parameters().get<MooseEnum>("scheme")))
{
}

template <bool is_ad>
std::vector<GenericReal<is_ad>>
SlopeReconstruction1DInterface<is_ad>::getElementSlopes(const Elem * elem) const
{
  // Determine flow channel direction
  const RealVectorValue dir_unnormalized = elem->node_ref(1) - elem->node_ref(0);
  const RealVectorValue dir = dir_unnormalized / dir_unnormalized.norm();

  std::vector<const Elem *> neighbors(_n_side, nullptr);
  for (unsigned int i_side = 0; i_side < _n_side; i_side++)
    neighbors[i_side] = elem->neighbor_ptr(i_side);

  // get this element's position and solution
  const Point x_elem = elem->vertex_average();
  const auto W_elem = computeElementPrimitiveVariables(elem);

  // get the number of slopes to be stored
  const unsigned int n_slopes = W_elem.size();

  // neighbor positions and solutions, if any
  std::vector<Point> x_neighbor(_n_side);
  std::vector<std::vector<GenericReal<is_ad>>> W_neighbor(_n_side);

  // one-sided slopes on each side
  std::vector<std::vector<GenericReal<is_ad>>> slopes_one_sided(
      _n_side, std::vector<GenericReal<is_ad>>(n_slopes, 0.0));

  // flags indicating whether each side is a boundary
  std::vector<bool> side_is_boundary(_n_side, false);

  // loop over the sides to compute the one-sided slopes
  for (unsigned int i_side = 0; i_side < _n_side; i_side++)
  {
    if (neighbors[i_side] != nullptr)
    {
      x_neighbor[i_side] = neighbors[i_side]->vertex_average();
      W_neighbor[i_side] = computeElementPrimitiveVariables(neighbors[i_side]);

      // compute change in position along flow channel direction
      const Real dx = (x_elem - x_neighbor[i_side]) * dir;

      for (unsigned int m = 0; m < n_slopes; m++)
        slopes_one_sided[i_side][m] = (W_elem[m] - W_neighbor[i_side][m]) / dx;
    }
    else
      side_is_boundary[i_side] = true;
  }

  // determine is cell is an interior cell
  const bool is_interior_cell = (!side_is_boundary[0] && !side_is_boundary[1]);

  // vector for the (possibly limited) slopes
  std::vector<GenericReal<is_ad>> slopes_limited(n_slopes, 0.0);

  // limit the slopes
  switch (_scheme)
  {
    // first-order, zero slope
    case None:
      break;

    // full reconstruction; no limitation
    case Full:
    {
      for (unsigned int m = 0; m < n_slopes; m++)
      {
        // For 1-D, the current element falls into 1 of 4 categories:
        // 1) interior cell
        // 2) left boundary cell (side 0 is boundary)
        // 3) right boundary cell (side 1 is boundary)
        // 4) left AND right boundary cell (there is a single cell in mesh)
        if ((!side_is_boundary[0]) && (!side_is_boundary[1]))
        {
          // compute change in position along flow channel direction
          const Real dx = (x_neighbor[0] - x_neighbor[1]) * dir;

          // use central slope
          slopes_limited[m] = (W_neighbor[0][m] - W_neighbor[1][m]) / dx;
        }
        else if (side_is_boundary[0])
          // side 0 is boundary; use side 1 slope
          slopes_limited[m] = slopes_one_sided[1][m];
        else if (side_is_boundary[1])
          // side 1 is boundary; use side 0 slope
          slopes_limited[m] = slopes_one_sided[0][m];
        else
          // single cell in mesh; no slope
          slopes_limited[m] = 0.0;
      }
    }
    break;

    // minmod limiter
    case Minmod:

      if (is_interior_cell)
      {
        for (unsigned int m = 0; m < n_slopes; m++)
        {
          if ((slopes_one_sided[0][m] * slopes_one_sided[1][m]) > 0.0)
          {
            if (std::abs(slopes_one_sided[0][m]) < std::abs(slopes_one_sided[1][m]))
              slopes_limited[m] = slopes_one_sided[0][m];
            else
              slopes_limited[m] = slopes_one_sided[1][m];
          }
        }
      }
      break;

    // MC (monotonized central-difference) limiter
    case MC:

      if (is_interior_cell)
      {
        // compute change in position along flow channel direction
        const Real dx = (x_neighbor[0] - x_neighbor[1]) * dir;

        std::vector<GenericReal<is_ad>> slopes_central(n_slopes, 0.0);
        for (unsigned int m = 0; m < n_slopes; m++)
          slopes_central[m] = (W_neighbor[0][m] - W_neighbor[1][m]) / dx;

        for (unsigned int m = 0; m < n_slopes; m++)
        {
          if (slopes_central[m] > 0.0 && slopes_one_sided[0][m] > 0.0 &&
              slopes_one_sided[1][m] > 0.0)
            slopes_limited[m] = std::min(
                slopes_central[m], 2.0 * std::min(slopes_one_sided[0][m], slopes_one_sided[1][m]));
          else if (slopes_central[m] < 0.0 && slopes_one_sided[0][m] < 0.0 &&
                   slopes_one_sided[1][m] < 0.0)
            slopes_limited[m] = std::max(
                slopes_central[m], 2.0 * std::max(slopes_one_sided[0][m], slopes_one_sided[1][m]));
        }
      }
      break;

    // superbee limiter
    case Superbee:

      if (is_interior_cell)
      {
        for (unsigned int m = 0; m < n_slopes; m++)
        {
          GenericReal<is_ad> slope1 = 0.0;
          GenericReal<is_ad> slope2 = 0.0;

          // calculate slope1 with minmod
          if (slopes_one_sided[1][m] > 0.0 && slopes_one_sided[0][m] > 0.0)
            slope1 = std::min(slopes_one_sided[1][m], 2.0 * slopes_one_sided[0][m]);
          else if (slopes_one_sided[1][m] < 0.0 && slopes_one_sided[0][m] < 0.0)
            slope1 = std::max(slopes_one_sided[1][m], 2.0 * slopes_one_sided[0][m]);

          // calculate slope2 with minmod
          if (slopes_one_sided[1][m] > 0.0 && slopes_one_sided[0][m] > 0.0)
            slope2 = std::min(2.0 * slopes_one_sided[1][m], slopes_one_sided[0][m]);
          else if (slopes_one_sided[1][m] < 0.0 && slopes_one_sided[0][m] < 0.0)
            slope2 = std::max(2.0 * slopes_one_sided[1][m], slopes_one_sided[0][m]);

          // calculate slope with maxmod
          if (slope1 > 0.0 && slope2 > 0.0)
            slopes_limited[m] = std::max(slope1, slope2);
          else if (slope1 < 0.0 && slope2 < 0.0)
            slopes_limited[m] = std::min(slope1, slope2);
        }
      }
      break;

    default:
      mooseError("Unknown slope reconstruction scheme");
      break;
  }

  return slopes_limited;
}
