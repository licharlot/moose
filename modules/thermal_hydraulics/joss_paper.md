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
    affiliation: 1
  - name: Lise Charlot
    orcid: 0000-0002-6343-6990
    affiliation: 1
  - name: Guillaume Giudicelli
    orcid: 0000-0001-9714-4382
    affiliation: 1
affiliations:
  - name: Idaho National Laboratory
    index: 1
date: 11 October 2023
bibliography: joss_paper.bib
---

# Summary

The Multiphysics Object-Oriented Simulation Environment (MOOSE) is an open-source object-oriented
finite element framework written in C++ [@lindsay2022moose]. The Thermal Hydraulics
Module (THM) is an optional MOOSE physics module that provides capabilities for
studying thermal hydraulic systems. Its core capability lies in assembling a network of
coupled components, for instance, pipes, junctions, valves, etc.

THM provides several new systems to MOOSE to enable and facilitate thermal
hydraulic simulations, most notably the `Components` system, which allows provides
a higher-level syntax to MOOSE's lower-level objects. This system is extensible
by the user, but the current library primarily includes components based on a 1D, single-phase,
variable-area, compressible flow model, as well as heat conduction.

# Statement of need

Numerous engineering applications employ fluid flow, and in particular, use systems
of connected components to transfer heat. Power generation applications must
provide a medium for converting power from a source to useful energy, and thermal
hydraulic systems are well-suited for the task, due to their ability to move
energy efficiently over long distances. These systems vary widely in their size
and complexity and may feature a large number of components coupled together.

A notable example of the application of thermal hydraulic systems analysis is for
nuclear reactor systems. These systems typically involve a large network of
components to facilitate the conversion of the nuclear power to electrical power,
as well as provide a number of safety systems. For example, there may be a
primary flow "loop" of pipes that extract heat from the fuel, pumps to force
circulation, one or more heat exchangers exchanging heat between this primary
loop and a secondary loop, turbomachinery components like turbines and generators,
etc. Accurate analyses of these systems require models that capture the coupling
between all of these components.

A wide variety of applications have been built using the MOOSE framework. A
suite of applications supporting various domains of analysis of nuclear reactor systems is under
active development at Idaho National Laboratory. THM provides a foundation for
thermal hydraulic applications in this area, including RELAP-7 [@relap7theory],
which models two-phase flow in light water nuclear reactors and coolant flow in
gas-cooled reactors, and Sockeye [@hansel2021sockeye], which models heat pipes
used in nuclear microreactors.

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

The `Components` system is abstract and provides several base classes that could
be utilized by a variety of physical applications. All components share the
base class `Component`; some intermediate base classes are the following:

- `Component1D`: generates a 1D mesh in 3D space, defined by a starting point,
  direction, length, and discretization.
- `Component2D`: generates a 2D mesh in 3D space, defined by the same axial
  parameters as `Component1D`, plus transverse direction, length, and discretization.
  This may represent either a Cartesian or axisymmetric coordinate system.
- `FileMeshComponent`: generates a mesh copied from a file (generally 3D).
- `Component1DBoundary`: used for applying boundary conditions to a `Component1D` component.
- `Component1DJunction`: used for applying coupled boundary conditions between `Component1D` components.

The following sub-sections describe some of the currently available components.

### Flow components

THM has numerous components that support a 1D, variable-area, single-phase
compressible flow model [@relap7theory].
Components related to this flow model use the suffix `1Phase`, the main component
being the straight-channel component called `FlowChannel1Phase`. Other components
related to this flow model are summarized as follows:

- boundary conditions, such as inlets/outlets (with various formulations) and walls.
- junctions, which allow flow to occurs between multiple flow channels by providing
  coupled boundary conditions to each connected channel. These also include valves,
  which can partially or completely close flow paths.
- volumetric heat sources, such as heat from a provided function, a convection
  condition, or a coupled heat flux.
- volumetric form loss sources, such as those arising from flow blockages.
- turbomachinery components, such as pumps, compressors, and turbines,
  which are particular types of junction that add source terms to the momentum
  and energy equations to simulate turbomachinery.

In addition to these components, there is also `FlowComponentNS`, which leverages
a selection of flow formulations from MOOSE's Navier-Stokes module [@lindsay2023moose],
with a mesh provided by a file.

### Heat conduction components

Thermal hydraulic systems feature not only flow components but also solid bodies
that transfer heat with the flow, such as the walls of a heat exchanger. In THM,
these bodies are 2D or 3D components referred to as "heat structures", which
solve the transient heat conduction equation:

- `HeatStructurePlate`: based from `Component2D` and using a Cartesian coordinate system,
  representing a "plate" geometry.
- `HeatStructureCylindrical`: based from `Component2D` and using an axisymmetric coordinate system,
  representing a "cylinder" or "shell" geometry.
- `HeatStructureFromFile3D`: based from `FileMeshComponent`, representing a general 3D geometry.

In addition to the heat structure components themselves, there are components that
interact with them:

- boundary conditions, such as Dirichlet, provided heat flux function, convection,
  and radiation.
- volumetric heat sources.
- interface conditions, which couple heat structures to other heat structures or
  to flow channels.

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

THM's documentation is extensive and follows the same structure as the code base. Each
object is accompanied by a documentation page that:

- describes it, including its equations or a figure as relevant
- lists its parameters along with metadata about the parameters
- lists all the input files in the repository that use this object
- lists all the classes deriving from this object.

Major groups of objects, usually derived from a single base class, are documented through the syntax documentation,
which describes how these objects are instantiated and used in a simulation.
For example, `Components` and `Closures` are examples of syntax unique to THM that also correspond to
base classes of groups of THM objects.

This documentation page is hosted on the module website [@thm_website]. The website also notably hosts the software
quality assurance (SQA) records, such as the testing requirement matrix or the failure analysis reports for example.
The interested reader is referred to the MOOSE SQA plan [@sqa] for more information.

# Testing

The module relies on CIVET [@slaughter2021continuous] for continuous integration.
Every pull request to the module runs the entire test suite for MOOSE, including the tests for THM.
The test suite is comprehensive and aims to cover every feature available in the module. It
consists of unit and regression tests, described below. The entire test suite is run with a wide variety of
configurations, from compiling with the oldest and newest supported compilers, to running with several
shared memory threads and distributed memory processes, on a variety of operating systems. The test suite
is also run encapsulated in Valgrind to detect memory leaks. Every pull request to the module
must include tests to cover any new features. This is ensured by the reviewer. The review process is detailed
in MOOSE's SQA plan [@sqa].

Unit tests in THM are targeted at specific routines that can be accessed by creating the relevant object with
example parameters. For example, creating a `Flux` object, we can check that the formulation of the numerical
flux, whether with a centered scheme or with HLLC, are both consistent and symmetric.

Regression tests are essentially created for each object to ensure that their output does not vary on a
relevant test case. For example, each function or each auxiliary kernel is tested on a simple Cartesian mesh to make sure
the field it produces is consistently the same. Components are often tested in the minimal configuration sufficient
to satisfy the test requirement, for example, to prove conservation of mass and energy on a flow channel.

# Demonstration
THM can be used as a foundation for other MOOSE-based applications or as a stand-alone software to model a variety of systems, including nuclear systems, power conversion cycles, geothermal piping networks.
The flexibility of THM is demonstrated with a two loop system that is typical of a nuclear reactor system. This model is the final step of the single-phase flow THM tutorial available on the THM website [@thm_website].

![System diagram (left) and temperature distribution (right) \label{fig:demo}](demo_dia_T.png)

The system is shown in \autoref{fig:demo}. Helium circulates in the primary loop and extracts heat from a rod. Heat is transferred to a secondary system through a heat exchanger. The cold helium then enters a pump, before flowing in the heated section again.
Each part of the systen shown in \autoref{fig:demo} is defined using the appropriate `Component` object.
The `ControlLogic` system is used to set the pump head to match a target mass flow rate in the primary loop. The secondary side is a flow channel with water.
The MOOSE Fluid Property Module is used to define the fluid properties in each loop.
This example features 2 set of `Closures` objects. The first set is of type `Closures1PhaseTHM` and uses classic correlations for the heat transfer coefficient and friction factor; the second set is of type `Closures1PhaseNone` and allows the user to define custom relations for the closure coefficient.
In this example, custom closures are used for the primary side of the heat exchanger, this is useful for complex geometries where experimental data can be used to set these closure coefficients. THM then calculates quantities of interest such as pressure, temperature, or mass flow rate.


# Conclusions

THM provides a flexible framework for thermal hydraulic systems simulations
performed using the MOOSE framework.
THM also provides many useful capabilities to the MOOSE framework that extend beyond
the field of thermal hydraulics. The `Components` system provides an ideal structure
for setting up large systems of connected components in MOOSE, significantly
reducing the user input burden, since the higher level components syntax hides
the multitude of lower level MOOSE objects. The `ControlLogic` system extends
the usability of MOOSE's `Controls` system, allowing control units to be
chained together.

The development of the module is driven by user application needs.
Future work to THM may include
improvement of existing components, as well as additional components related to
single-phase flow and heat conduction. Depending on future needs, additional
flow models may be added as well. Further abstraction of the way the discretization
of an equation is created on a component is underway, and should allow for the definition
of a general multiphysics component, able to instantiate any equation discretized in MOOSE.

External contributions to the module are encouraged, and
support will be provided to comply with MOOSE's SQA standard.

# Acknowledgements

We would like to acknowledge the time and effort of THM's many contributors,
most notably Jack Cavaluzzi, Thomas Freyman, Luiz Aldeia, and Rachel Beall.
We would also like to thank Richard Martineau for his strong support of the project.

This work was funded by the Department of Energy Nuclear Energy Advanced Modeling and Simulation (NEAMS) program.
This manuscript has been authored by Battelle Energy Alliance, LLC under Contract No. DE-AC07-05ID14517 with the US Department of Energy. The United States Government retains and the publisher, by accepting the article for publication, acknowledges that the United States Government retains a nonexclusive, paid-up, irrevocable, worldwide license to publish or reproduce the published form of this manuscript, or allow others to do so, for United States Government purposes.

# References
