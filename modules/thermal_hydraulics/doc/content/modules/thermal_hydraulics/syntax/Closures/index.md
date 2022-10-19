# Closures System

The `Closures` system allows users to create MOOSE objects (usually [Materials](syntax/Materials/index.md))
that specify closures for their [Components](syntax/Components/index.md). For example, a flow channel
may require definition of quantities such as friction factors and heat transfer
coefficients. While these definitions could be made inside a component, it is
advantageous to have closure definitions outside. There may be a large number of
closures choices, each with their own user parameters. In large systems of
components, `Closures` objects can be re-used when the closures apply to many
objects.

The base class `ClosuresBase` declares the methods `addMooseObjectsFlowChannel()`
and `addMooseObjectsHeatTransfer()`, which are intended to be called in the
`addMooseObjects()` method of the flow channel and heat transfer components,
respectively. Similarly, the methods `checkFlowChannel()` and
`checkHeatTransfer()` are intended to be called in the `check()` method.
The flow channel methods `addMooseObjectsFlowChannel()` and `checkFlowChannel()`
take the flow channel component as an argument and thus may call any of its
public methods, which are often needed to construct lower-level closures objects.
The heat transfer methods `addMooseObjectsHeatTransfer()` and `checkHeatTransfer()`
take the heat transfer component and its associated flow channel component as
arguments, since both of these components are often needed to construct
lower-level closures objects.

## Objects and Associated Actions

!syntax list /Components objects=True actions=False subsystems=False

!syntax list /Components objects=False actions=False subsystems=True

!syntax list /Components objects=False actions=True subsystems=False
