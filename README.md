SPDX-License-Identifier: MIT

# keff

*pronounced like "Jeff", but with a K*

keff is a small C program that computes the effective neutron multiplication
factor k_eff by solving the two-group neutron diffusion eigenvalue problem on
a 1D slab. Materials are assigned per region, so fuel/reflector stacks and
axial layouts work. Power iteration, finite differences, vacuum or reflective
boundaries.

## Build

    gcc -Wall -Wextra keff.c -o keff -lm

## Run

    ./keff -i input.txt [-o output.txt]

Output goes to stdout if -o is omitted.

## Input

key = value lines, # comments, sections in brackets. Units: cm, cm^-1.

    [geometry]
    width = 100.0
    mesh = 200
    boundary = vacuum        # or reflective
    flux = yes               # optional, dumps flux profile

    [material fuel]
    d1 = 1.0                 # fast diffusion coefficient
    sa1 = 0.004              # fast absorption
    s12 = 0.04               # downscatter
    nusf1 = 0.002            # nu * fast fission xs
    d2 = 0.9
    sa2 = 0.08
    nusf2 = 0.115            # nu * thermal fission xs

    [material water]
    ...

    [layout]
    regions = water:20,fuel:60,water:20

chi1/chi2 (fission spectrum) optional, defaults to all-fast.

Verified against analytic k_inf (reflective) and bare-slab buckling cases,
agrees to ~1e-6. See the input examples in the repo.

License: MIT, see LICENSE.

Onyx Nemo, 08/NOV/2026
