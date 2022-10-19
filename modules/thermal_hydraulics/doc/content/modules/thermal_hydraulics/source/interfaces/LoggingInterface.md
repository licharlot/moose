# LoggingInterface

Systems simulations can have a very large number of components, and creation of
such large models is potentially complex and error-prone, perhaps involving
tens or hundreds of input errors in a user's first attempt. To enable a practical
input file-writing workflow, it is important that errors can be reported in batch,
instead of stopping execution at the first error encountered, which is the usual
behavior in MOOSE due to the use of `mooseError()`.
To address this need, the `Logger` and the associated
`LoggingInterface` are provided. An application creates a single `Logger`, and
then various objects can inherit from `LoggingInterface` (providing the
interface with the single `Logger` so that local references may be made).
[Components](syntax/Components/index.md) and [Closures](syntax/Closures/index.md)
both inherit from `LoggingInterface`, for example. `LoggingInterface` provides
methods to conveniently log errors and warnings by interacting directly with the
`Logger`. These log events add messages to error and warning lists, but
execution continues until the application chooses to print out all of the errors
and warnings, which the `Logger` assembles into a very palatable, condensed
format. Then the user can view and address these errors all at once,
significantly decreasing the number of input file iterations.
