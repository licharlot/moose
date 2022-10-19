---
title: 'The MOOSE Thermal Hydraulics Module'
tags:
  - C++
  - multiphysics
authors:
  - name: David Andrs
    orcid: 0000-0002-8913-902X
    affiliation: 1
  - name: Joshua Hansel
    orcid: 0000-0001-6782-5275
    affiliation: 1
  - name: Lise Charlot
    orcid: 0000-0002-6343-6990
    affiliation: 1
affiliations:
  - name: Idaho National Laboratory
    index: 1
date: 4 October 2022
bibliography: joss_paper.bib
---

# Introduction

The Multiphysics Object-Oriented Simulation Environment (MOOSE) is an object-oriented
finite element framework written in C++ [@lindsay2022moose]. The Thermal Hydraulics
Module (THM) is an optional MOOSE module that provides capabilities designed for
thermal hydraulic systems. Its core capability lies in assembling a network of
coupled components, for instance, pipes, junctions, valves, etc.

THM is used as the foundation for the applications RELAP-7 [@relap7theory] and
Sockeye [@hansel2021sockeye], both developed at Idaho National Laboratory, which
respectively simulate two-phase flow components and heat pipes.

# Capabilities

## Components System

The `Components` system allows users to add "component"s to their
simulation, which in general may perform several tasks:

- Build meshes (1D, 2D, or 3D)
- Add MOOSE variables
- Add MOOSE objects

Usually components represent some physical component in a system
such as a 1D pipe, a 2D wall/structure, or a 0D junction; however, they can also be abstract,
for example, coupling other components together or providing some
source or boundary conditions.

### Heat Conduction Components



### Single-Phase Flow Components

THM's current library is geared toward single-phase, variable-area,
compressible flows.

## Closures System

The `Closures` system allows users to create MOOSE objects (usually `Material`s)
that specify closures for their component models. For example, a flow channel
may require definition of quantities such as friction factors and heat transfer
coefficients. While these definitions could be made inside a component, it is
advantageous to have closure definitions outside. There may be a large number of
closures choices, each with their own user parameters. In large systems of
components, `Closures` objects can be re-used when the closures apply to many
objects.

## Control Logic System

The `ControlLogic` system is an extension of MOOSE's `Controls`
system, which is used to control input parameters to various objects during
a simulation. Unlike standard controls objects, control logic objects
may declare new control data that is not associated with input parameters and
may retrieve control data declared in other control logic objects, allowing
control operations to be chained together, which is not possible in the standard
`Controls` system. This is necessary to mirror real control systems in thermal
hydraulic systems, which may have many controls in series.

Examples of controls in THM's library include a function control, PID control,
delay control, trip control, and terminate control.

Also, THM automatically brings all `Postprocessor`s into the `ControlLogic`
system.

## Integrity Checking

### Error Logging

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

# Demonstration

# Conclusions

(paragraph about THM providing thermal hydraulics capabilities)

THM also provides many useful capabilities to the MOOSE framework that extend beyond
the field of thermal hydraulics. The `Components` system provides an ideal structure
for setting up large systems of connected components in MOOSE, significantly
reducing the user input burden, since the higher level components syntax hides
the multitude of lower level MOOSE objects. The `ControlLogic` system extends
the usability of MOOSE's `Controls` system, allowing control units to be
chained together.

Currently, when components are used, it is not possible to create meshes outside
of the component system, which prevents having THM and non-THM physics in the
same simulation. Usually THM-based applications are loosely or tightly coupled
to other applications through MOOSE's multi-app system, but there are cases
where a full coupling is advantageous and/or more convenient. Future work to THM
is planned to address this inability by building component meshes via MOOSE's
"mesh generators", which provide the ability to chain together multiple
mesh operations to compose a mesh. Additional future work to THM may include
improvement of existing components, as well as additional components related to
single-phase flow and heat conduction. Depending on future needs, additional
flow models may be added as well.

# Acknowledgements


# References
