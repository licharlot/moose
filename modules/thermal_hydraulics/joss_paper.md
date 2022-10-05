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
finite element framework written in C++.

# Design

## Components System

The `Components` system allows users to add `Component`s to their
simulation, which in general may perform several tasks:

- Build meshes (1D, 2D, or 3D)
- Add MOOSE variables
- Add MOOSE objects

Usually `Component`s represent some physical component in a system
such as a 1D pipe, a 2D wall/structure, or a 0D junction; however, they can also be abstract,
for example, coupling other `Component`s together or providing some
source or boundary conditions.

THM's current library is geared toward single-phase, variable-area,
compressible flows.

## Closures System

The `Closures` system allows users to create objects that specify
closures for their component models.

## Control Logic System

The `ControlLogic` system is an extension of MOOSE's `Controls`
system, which is used to control input parameters to various objects during
a simulation. The base class of the `ControlLogic` system, `THMControl`,
adds methods to declare new control data and methods to bring in control data
declared by other `THMControl`s. This allows these objects to be chained
together, which is not possible in MOOSE's `Controls` system, allowing modeling
of control setups in physical systems.

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
behavior in MOOSE due to the use of `mooseError()`.
To address this need, THM provides the `Logger` and the associated
`LoggingInterface`. An application creates a single `Logger`, and
then various objects can inherit from `LoggingInterface` (providing the
interface with the single `Logger` so that local references may be made).
`Component` and `ClosuresBase` both inherit from `LoggingInterface`,
for example. `LoggingInterface` provides methods to conveniently
log errors and warnings by interacting directly with the `Logger`.
These log events add messages to error and warning lists, but execution continues
until the application chooses to print out all of the errors and warnings,
which the `Logger` assembles into a very palatable, condensed format.
Then the user can view and address these errors all at once, significantly
decreasing the number of input file iterations.

# Demonstration

# Conclusions

THM provides many useful capabilities to the MOOSE framework that extend beyond
the field of thermal hydraulics.

Future work to THM will include...

# Acknowledgements


# References
