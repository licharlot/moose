!syntax description /UserObjects/BoundaryFlux3EqnGhostPressure

!include euler_1d_var_area_boundary_flux_ghost.md

This computes the boundary flux from a specified pressure $p_b$, with density
$\rho_i$ and velocity $u_i$ coming from the solution:
\begin{equation}
  \mathbf{U}_b = \mathbf{U}(\rho_i, u_i, p_b) \,,
\end{equation}
\begin{equation}
  \mathbf{F}_b = \mathbf{F}(\mathbf{U}_b) \,.
\end{equation}

!syntax parameters /UserObjects/BoundaryFlux3EqnGhostPressure

!syntax inputs /UserObjects/BoundaryFlux3EqnGhostPressure

!syntax children /UserObjects/BoundaryFlux3EqnGhostPressure
