#include <stdio.h>
#include <math.h>
#define VERSION ("0.0.1")

/*K_EFF is a utility for calculation of the effective neutron multiplication factor for a finite nuclear reactor. 
 * It is free for use, modification, and distribution under the MIT license
 * Author: Onyx Nemo; Date: 00/NOV/2026 */

// VERSION 0.0.1 is very early, doesn't yet have user modifiable factors and only outputs k_eff, state and rho.
// Next versions will go more in depth about actual calculations plus will include user interface.

typedef struct {
	double eta; // reproduction factor
	double eps; // fast fission factor
	double p;  // resonance escape probability
	double f;   // thermal utilization
	double pf;  // fast non-leakage probability
	double pt;  // thermal non-leakage probability
} Factors;

double k_eff(const Factors *s) {
	return s->eta * s->eps * s->p * s->f * s->pf * s->pt;
}

void print_state(double k) {
	if (k < 1.0)
		printf("subcritical\n");
	else if (k > 1.0)
		printf("supercritical\n");
	else
		printf("critical\n");
}

int main(void) {
	printf(" --- K_EFF version ");
	printf(VERSION);
	printf(" ---\n");

	Factors pwr = { .eta = 1.65, .eps = 1.03, .p = 0.75, .f = 0.90, .pf = 0.98, .pt = 0.96 };
	Factors unreflected_bare = { .eta = 1.65, .eps = 1.03, .p = 0.75, .f = 0.90, .pf = 0.60, .pt = 0.55 };

	double k = k_eff(&pwr);
	double k0 = k_eff(&unreflected_bare);

	printf("k_eff (leakage-controlled) = %.4f, ", k);
	print_state(k);
	printf("k_eff (bare core) = %.4f, ", k0);
	print_state(k0);

	// reactivity, under rho = (k - 1)/k
	printf("rho = %+.2f pcm\n", (k - 1.0) / k * 1e5);

	return 0;
}
