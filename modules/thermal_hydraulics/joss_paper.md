---
title: 'The MOOSE Thermal Hydraulics Module'
tags:
  - C++
  - multiphysics
  - system analysis
  - thermal hydraulics
authors:
  - name: Joshua Hansel
    orcid: 0000-0001-6782-5275
    affiliation: 1
  - name: David Andrs
    orcid: 0000-0002-8913-902X
    affiliation: 2
  - name: Lise Charlot
    orcid: 0000-0002-6343-6990
    affiliation: 1
  - name: Guillaume Giudicelli
    orcid: 0000-0001-9714-4382
    affiliation: 1
affiliations:
  - name: Idaho National Laboratory
    index: 1
  - name: Sawtooth Simulation
    index: 2
date: 11 October 2023
bibliography: joss_paper.bib
---

# Summary

The Multiphysics Object-Oriented Simulation Environment (MOOSE) is an open-source object-oriented
finite element framework written in C++ [@lindsay2022moose]. The Thermal Hydraulics
Module (THM) is an optional MOOSE physics module that provides capabilities for
studying thermal hydraulic systems. Its core capability lies in assembling a network of
coupled components, for instance, pipes, junctions, valves, etc. It can then discretize
single-phase compressible flow equations to model these components.

(Josh: expand)

# Statement of need

THM is used as the foundation for several applications, including RELAP-7 [@relap7theory] and
Sockeye [@hansel2021sockeye], both developed at Idaho National Laboratory. RELAP-7 serves to model
two-phase flow in light water nuclear reactors, coolant flow in gas-cooled reactors. Sockeye
is used to model heat pipes, mainly for cooling purposes of nuclear micro-reactors. The components
developed in those applications are compatible with the components in THM, and vice-versa.

This paper briefly describes the software design principles guiding the development of the module,
some of THM's systems, including its core components and functionalities, and gives a basic
demonstration of its capabilities.

# Core capabilities

## `Components` system

The `Components` system allows users to add "components", which are very flexible
in their use, but in general represent "pieces" of a simulation, which may couple
together. Common uses for components include adding meshes (1D, 2D, or 3D), variables,
equations, and output. Components provide a higher level syntax that
hides lower level MOOSE objects such as `Kernel`s, `BoundaryCondition`s, etc. While
`Action`s can also be used to create a higher level syntax, components provide much more
convenience, particularly when multiple components interact.

Usually components represent some physical component in a system
such as a 1D pipe, a 2D wall/structure, or a 0D junction; however, they can also be abstract,
for example, coupling other components together, providing some
source or boundary conditions, or just adding any other MOOSE objects
in a convenient manner.

While the `Components` system itself is abstract, the library of existing
components in THM is geared toward thermal hydraulics, and specifically
contains 1D and 0D components using a single-phase, compressible flow model.
Additionally, there are components for modeling heat conduction in 2D and
3D, as well as some miscellaneous 0D components.

All components derive from the class `Component`

- `BoundaryBase`

Components can be broadly organized into the following categories:

-
- 2D (Cartesian or cylindrical) and 3D *heat structures*, which are volumes
  which apply the transient heat conduction equation,

  \begin{equation}
    \rho c_p \frac{\partial T}{\partial t} - \nabla \cdot (k \nabla T) = q''' \,,
  \end{equation}

  where $\rho$ is density, $c_p$ is the specific heat capacity, $T$ is temperature,
  $k$ is thermal conductivity, and $q'''$ is a heat source term.
- heat structure boundary conditions like convection, radiation, and a provided heat flux function,
-

Heat conduction is an important physical domain that is relevant to numerous
applications. The transient heat conduction equation is given as

(Josh: finish this section)

## `Closures` system

The `Closures` system allows users to create MOOSE objects (usually `Material`s)
that specify closures for their component models. For example, a flow channel
may require definition of quantities such as friction factors and heat transfer
coefficients. While these definitions could be made inside a component, it is
advantageous to have closure definitions outside. There may be a large number of
closures choices, each with their own user parameters. In large systems of
components, `Closures` objects can be re-used when the closures apply to many
objects.

## `ControlLogic` system

The `ControlLogic` system is an extension of MOOSE's `Controls`
system, which is used to control input parameters to various objects during
a simulation. Unlike standard controls objects, control logic objects
may declare new control data that is not associated with input parameters and
may retrieve control data declared in other control logic objects, allowing
control operations to be chained together, which is not possible in the standard
`Controls` system. This is necessary to mirror real control systems in thermal
hydraulic systems, which may feature various controllers in series.
Examples of controls in THM's library include the following:
a transient function control,
a proportional-integral-derivative (PID) control,
a delay control, a trip control, and a terminate control.

## Integrity-checking

Systems simulations can have a very large number of components, and creation of
such large models is potentially complex and error-prone, perhaps involving
tens or hundreds of input errors in a user's first attempt. To enable a practical
input file-writing workflow, it is important that errors can be reported in batch,
instead of stopping execution at the first error encountered, which is the usual
behavior in MOOSE. To address this need, THM provides a logging capability.
An application creates a single "logger" object, and
then various objects (such as components and closures objects) can log errors
and warnings. Execution continues until the application chooses to print
out all of the errors and warnings, in a very palatable, condensed format.
Then the user can view and address these errors all at once, significantly
decreasing the number of input file iterations.

# Documentation

(Guillaume: refer to website)

# Testing

(Guillaume)

# Demonstration

(Lise: tutorial demonstration here, also give some example usage)

# Conclusions

(Josh: paragraph about THM providing thermal hydraulics capabilities)

THM also provides many useful capabilities to the MOOSE framework that extend beyond
the field of thermal hydraulics. The `Components` system provides an ideal structure
for setting up large systems of connected components in MOOSE, significantly
reducing the user input burden, since the higher level components syntax hides
the multitude of lower level MOOSE objects. The `ControlLogic` system extends
the usability of MOOSE's `Controls` system, allowing control units to be
chained together.

Future work to THM may include
improvement of existing components, as well as additional components related to
single-phase flow and heat conduction. Depending on future needs, additional
flow models may be added as well. Additionally, work is planned to integrate
THM with MOOSE's Navier-Stokes (NS) module, bringing existing flow models and methods
from THM into the NS module, and allowing components to use alternative
flow models and methods implemented in the NS module.

(Guillaume: general plans for new syntax(es), capabilities if they are pretty certain)

# Acknowledgements

We would like to acknowledge the time and effort of THM's many contributors,
most notably Jack Cavaluzzi, Thomas Freyman, Luiz Aldeia, and Rachel Beall.
We would also like to thank Richard Martineau for his strong support of the project.

This work was funded by the Department of Energy Nuclear Energy Advanced Modeling and Simulation (NEAMS) program.
This manuscript has been authored by Battelle Energy Alliance, LLC under Contract No. DE-AC07-05ID14517 with the US Department of Energy. The United States Government retains and the publisher, by accepting the article for publication, acknowledges that the United States Government retains a nonexclusive, paid-up, irrevocable, worldwide license to publish or reproduce the published form of this manuscript, or allow others to do so, for United States Government purposes.

# References
