# K_EFF
version 0.0.1
## K_EFF, pronounced like Jeff but with a K, is a tool for calculating k_eff. 
What's k_eff? k_eff, pronounced “k-effective,” is the effective neutron multiplication factor: a dimensionless measure of whether a fission chain reaction shrinks, sustains itself, or grows in a real, finite reactor, accounting for neutron absorption and leakage. So in short, trying to balance a nuclear reactor!
The core formula:
## k_eff = k_inf * P_F * P_T = eta * epsilon * p * f * P_F * P_T
Where:

```k_eff = effective neutron multiplication factor
P_F = fast non-leakage probability
P_T = thermal non-leakage probability
eta = reproduction factor
epsilon = fast fission factor
p = resonance escape probability
f = thermal utilization 
```

## Usage
Compile with ```gcc -Wall -Wextra -o keff```.\n
Run with ```./keff -i input.txt [-o output.txt]```\n
Input file is mandatory, output file may be left out. In that case results will go to stdout.

Use: This project is under the MIT license.\n
Author: Onyx Nemo\n
Date last updated: 02/NOV/2026\n

