#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define VERSION ("0.0.1")

/*K_EFF is a utility for calculation of the effective neutron multiplication factor for a finite nuclear reactor. 
 * It is free for use, modification, and distribution under the MIT license
 * Author: Onyx Nemo; Date: 00/NOV/2026 */

// VERSION 0.0.1 is very early, doesn't yet have user modifiable factors and only outputs k_eff, state and rho.
// Next versions will go more in depth about actual calculations plus will include user interface.

typedef struct {
	char title[128];
	int has_title;
	double eta; // reproduction factor
	double eps; // fast fission factor
	double res_escape;  // resonance escape probability
	double f;   // thermal utilization
	double pf;  // fast non-leakage probability
	double pt;  // thermal non-leakage probability
} Input;

typedef struct {
    const char *name;
    double *dst;
} Field;

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

static int parse_num(const char *s, double *out) {
    char *end;
    *out = strtod(s, &end);
    return end != s && *end == 0;
}

static int parse_file(const char *path, Input *in) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "error: cannot open %s\n", path);
        return -1;
    }

    Field fields[] = {
        {"eta",        &in->eta},
        {"eps",        &in->eps},
        {"res_escape", &in->res_escape},
        {"f",          &in->f},
        {"pf",         &in->pf},
        {"pt",         &in->pt},
    };
    int nfields = sizeof fields / sizeof fields[0];
    int seen[6] = {0};

    char line[512];
    int ln = 0;
    while (fgets(line, sizeof line, fp)) {
        ln++;
        char *s = trim(line);
        if (*s == 0 || *s == '#')
            continue;
        char *eq = strchr(s, '=');
        if (!eq) {
            fprintf(stderr, "error: %s:%d: expected key = value\n", path, ln);
            fclose(fp);
            return -1;
        }
        *eq = 0;
        char *key = trim(s);
        char *val = trim(eq + 1);

        if (!strcmp(key, "title")) {
            snprintf(in->title, sizeof in->title, "%s", val);
            in->has_title = 1;
            continue;
        }

        int i, hit = -1;
        for (i = 0; i < nfields; i++)
            if (!strcmp(key, fields[i].name))
                hit = i;
        if (hit < 0) {
            fprintf(stderr, "error: %s:%d: unknown key '%s'\n", path, ln, key);
            fclose(fp);
            return -1;
        }
        if (!parse_num(val, fields[hit].dst)) {
            fprintf(stderr, "error: %s:%d: bad value for %s\n", path, ln, key);
            fclose(fp);
            return -1;
        }
        seen[hit] = 1;
    }
    fclose(fp);

    int missing = 0;
    for (int i = 0; i < nfields; i++) {
        if (!seen[i]) {
            fprintf(stderr, "error: missing key '%s'\n", fields[i].name);
            missing = 1;
        }
    }
    return missing ? -1 : 0;
}

static double k_eff(const Input *in) {
    return in->eta * in->eps * in->res_escape * in->f * in->pf * in->pt;
}

static const char *state(double k) {
    return k < 1.0 ? "subcritical" : k > 1.0 ? "supercritical" : "critical";
}

static int run(const char *in_path, FILE *out) {
    Input in = {0};
    in.eta = in.eps = in.res_escape = in.f = in.pf = in.pt = NAN;

    if (parse_file(in_path, &in))
        return 1;

    double k = k_eff(&in);
    double rho = (k - 1.0) / k * 1e5;

    if (in.has_title)
        fprintf(out, "title = %s\n", in.title);
    fprintf(out, "k_eff = %.6f\n", k);
    fprintf(out, "state = %s\n", state(k));
    fprintf(out, "rho_pcm = %+.2f\n", rho);
    printf("OK\n");
    return 0;
}

int main(int argc, char **argv) {
	printf(" --- K_EFF version ");
	printf(VERSION);
	printf(" ---\n");
	
    const char *in_path = NULL, *out_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc)
            in_path = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc)
            out_path = argv[++i];
        else {
            fprintf(stderr, "usage: %s -i input [-o output]\n", argv[0]);
            return 1;
        }
    }
    if (!in_path) {
        fprintf(stderr, "usage: %s -i input [-o output]\n", argv[0]);
        return 1;
    }

    if (!out_path)
        return run(in_path, stdout);

    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "error: cannot open %s for writing\n", out_path);
        return 1;
    }
    int rc = run(in_path, out);
    fclose(out);
    return rc;
}
