#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <math.h>

#define MAXMAT 32
#define MAXREG 32
#define REQ 0x7F

typedef struct {
    char name[32];
    double d[2], sa[2], nusf[2], chi[2], s12;
    unsigned got;
} Mat;

typedef struct {
    int mat;
    double width;
} Region;

typedef struct {
    char title[128];
    int has_title;
    double width;
    int mesh;
    int reflective;
    int flux_out;
    Mat mats[MAXMAT];
    int nmats;
    Region regs[MAXREG];
    int nregs;
} Input;

static const struct { const char *key; size_t off; } mfields[] = {
    {"d1",    offsetof(Mat, d[0])},
    {"d2",    offsetof(Mat, d[1])},
    {"sa1",   offsetof(Mat, sa[0])},
    {"sa2",   offsetof(Mat, sa[1])},
    {"nusf1", offsetof(Mat, nusf[0])},
    {"nusf2", offsetof(Mat, nusf[1])},
    {"s12",   offsetof(Mat, s12)},
    {"chi1",  offsetof(Mat, chi[0])},
    {"chi2",  offsetof(Mat, chi[1])},
};
#define NMFIELD (sizeof mfields / sizeof mfields[0])

static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

static int parse_num(const char *s, double *out)
{
    char *end;
    *out = strtod(s, &end);
    return end != s && *end == 0;
}

static Mat *find_mat(Input *in, const char *name)
{
    for (int i = 0; i < in->nmats; i++)
        if (!strcmp(in->mats[i].name, name))
            return &in->mats[i];
    return NULL;
}

static int parse_regions(Input *in, const char *val, const char *path, int ln)
{
    char buf[256];
    snprintf(buf, sizeof buf, "%s", val);

    for (char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
        char *colon = strchr(tok, ':');
        if (!colon) {
            fprintf(stderr, "error: %s:%d: region needs material:width\n", path, ln);
            return -1;
        }
        *colon = 0;
        char *name = trim(tok);
        char *wstr = trim(colon + 1);
        double w;
        if (!parse_num(wstr, &w)) {
            fprintf(stderr, "error: %s:%d: bad region width\n", path, ln);
            return -1;
        }
        Mat *m = find_mat(in, name);
        if (!m) {
            fprintf(stderr, "error: %s:%d: unknown material '%s'\n", path, ln, name);
            return -1;
        }
        if (in->nregs >= MAXREG) {
            fprintf(stderr, "error: too many regions\n");
            return -1;
        }
        in->regs[in->nregs].mat = (int)(m - in->mats);
        in->regs[in->nregs].width = w;
        in->nregs++;
    }
    return in->nregs ? 0 : -1;
}

static int parse_file(const char *path, Input *in)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "error: cannot open %s\n", path);
        return -1;
    }

    char section[32] = "";
    Mat *cur = NULL;
    unsigned gseen = 0;
    char line[512];
    int ln = 0;

    while (fgets(line, sizeof line, fp)) {
        ln++;
        char *s = trim(line);
        if (*s == 0 || *s == '#')
            continue;

        if (*s == '[') {
            char *close = strchr(s, ']');
            if (!close) {
                fprintf(stderr, "error: %s:%d: unterminated section\n", path, ln);
                fclose(fp);
                return -1;
            }
            *close = 0;
            char body[64];
            snprintf(body, sizeof body, "%s", s + 1);
            char *sp = strtok(body, " \t");
            char *arg = strtok(NULL, " \t");
            if (!sp) {
                fprintf(stderr, "error: %s:%d: empty section\n", path, ln);
                fclose(fp);
                return -1;
            }
            if (!strcmp(sp, "material")) {
                if (!arg) {
                    fprintf(stderr, "error: %s:%d: material needs a name\n", path, ln);
                    fclose(fp);
                    return -1;
                }
                if (find_mat(in, arg)) {
                    fprintf(stderr, "error: %s:%d: duplicate material '%s'\n", path, ln, arg);
                    fclose(fp);
                    return -1;
                }
                if (in->nmats >= MAXMAT) {
                    fprintf(stderr, "error: too many materials\n");
                    fclose(fp);
                    return -1;
                }
                cur = &in->mats[in->nmats++];
                snprintf(cur->name, sizeof cur->name, "%s", arg);
                cur->chi[0] = 1.0;  // chi defaults to all-fast
                strcpy(section, "material");
            } else if (!strcmp(sp, "geometry") || !strcmp(sp, "layout")) {
                cur = NULL;
                snprintf(section, sizeof section, "%s", sp);
            } else {
                fprintf(stderr, "error: %s:%d: unknown section '%s'\n", path, ln, sp);
                fclose(fp);
                return -1;
            }
            continue;
        }

        char *eq = strchr(s, '=');
        if (!eq) {
            fprintf(stderr, "error: %s:%d: expected key = value\n", path, ln);
            fclose(fp);
            return -1;
        }
        *eq = 0;
        char *key = trim(s);
        char *val = trim(eq + 1);

        if (!*section) {
            if (strcmp(key, "title")) {
                fprintf(stderr, "error: %s:%d: key '%s' outside any section\n", path, ln, key);
                fclose(fp);
                return -1;
            }
            snprintf(in->title, sizeof in->title, "%s", val);
            in->has_title = 1;
        } else if (!strcmp(section, "geometry")) {
            if (!strcmp(key, "width")) {
                if (!parse_num(val, &in->width)) {
                    fprintf(stderr, "error: %s:%d: bad width\n", path, ln);
                    fclose(fp);
                    return -1;
                }
                gseen |= 1;
            } else if (!strcmp(key, "mesh")) {
                char *end;
                in->mesh = strtol(val, &end, 10);
                if (end == val || *end) {
                    fprintf(stderr, "error: %s:%d: bad mesh\n", path, ln);
                    fclose(fp);
                    return -1;
                }
                gseen |= 2;
            } else if (!strcmp(key, "boundary")) {
                if (!strcmp(val, "reflective"))
                    in->reflective = 1;
                else if (!strcmp(val, "vacuum"))
                    in->reflective = 0;
                else {
                    fprintf(stderr, "error: %s:%d: boundary must be vacuum or reflective\n", path, ln);
                    fclose(fp);
                    return -1;
                }
            } else if (!strcmp(key, "flux")) {
                in->flux_out = !strcmp(val, "yes");
            } else {
                fprintf(stderr, "error: %s:%d: unknown geometry key '%s'\n", path, ln, key);
                fclose(fp);
                return -1;
            }
        } else if (!strcmp(section, "layout")) {
            if (strcmp(key, "regions")) {
                fprintf(stderr, "error: %s:%d: unknown layout key '%s'\n", path, ln, key);
                fclose(fp);
                return -1;
            }
            if (parse_regions(in, val, path, ln)) {
                fclose(fp);
                return -1;
            }
        } else {  // material
            int hit = -1;
            for (int i = 0; i < (int)NMFIELD; i++)
                if (!strcmp(key, mfields[i].key))
                    hit = i;
            if (hit < 0) {
                fprintf(stderr, "error: %s:%d: unknown material key '%s'\n", path, ln, key);
                fclose(fp);
                return -1;
            }
            double v;
            if (!parse_num(val, &v)) {
                fprintf(stderr, "error: %s:%d: bad value for %s\n", path, ln, key);
                fclose(fp);
                return -1;
            }
            *(double *)((char *)cur + mfields[hit].off) = v;
            cur->got |= 1u << hit;
        }
    }
    fclose(fp);

    if (gseen != 3) {
        fprintf(stderr, "error: geometry needs width and mesh\n");
        return -1;
    }
    if (!in->nmats || !in->nregs) {
        fprintf(stderr, "error: need at least one material and one region\n");
        return -1;
    }
    for (int i = 0; i < in->nmats; i++) {
        if ((in->mats[i].got & REQ) != REQ) {
            for (int j = 0; j < (int)NMFIELD; j++)
                if (!(in->mats[i].got & (1u << j)) && (1u << j) <= REQ)
                    fprintf(stderr, "error: material '%s' missing key '%s'\n",
                            in->mats[i].name, mfields[j].key);
            return -1;
        }
    }
    return 0;
}

// Two-group 1D slab diffusion eigenvalue problem, finite differences,
// Gauss-Seidel inner sweeps, power iteration on k.
static double solve(const Input *in, double *p1, double *p2, int *iters)
{
    int n = in->mesh;
    double dx = in->width / n;
    const Mat **c = malloc(n * sizeof *c);
    double *src = malloc(n * sizeof *src);

    int r = 0;
    double xedge = in->regs[0].width;
    for (int i = 0; i < n; i++) {
        double xc = (i + 0.5) * dx;
        while (r < in->nregs - 1 && xc >= xedge) {
            r++;
            xedge += in->regs[r].width;
        }
        c[i] = &in->mats[in->regs[r].mat];
    }

    for (int i = 0; i < n; i++)
        p1[i] = p2[i] = 1.0;

    double k = 1.0;
    *iters = 0;
    for (;;) {
        (*iters)++;
        double F = 0;
        for (int i = 0; i < n; i++) {
            src[i] = c[i]->nusf[0] * p1[i] + c[i]->nusf[1] * p2[i];
            F += src[i];
        }
        for (int i = 0; i < n; i++)
            src[i] /= F;

        for (int sw = 0; sw < 1000; sw++) {
            double diff = 0;
            for (int i = 0; i < n; i++) {
                const Mat *m = c[i];
                double a = m->d[0] / (dx * dx);
                // vacuum: ghost = -phi (zero at face); reflective: ghost = +phi
                double L = i > 0 ? p1[i-1] : (in->reflective ? p1[i] : -p1[i]);
                double R = i < n-1 ? p1[i+1] : (in->reflective ? p1[i] : -p1[i]);
                double v = (m->chi[0] * src[i] / k + a * (L + R))
                         / (2*a + m->sa[0] + m->s12);
                diff = fmax(diff, fabs(v - p1[i]));
                p1[i] = v;
            }
            for (int i = 0; i < n; i++) {
                const Mat *m = c[i];
                double b = m->d[1] / (dx * dx);
                double L = i > 0 ? p2[i-1] : (in->reflective ? p2[i] : -p2[i]);
                double R = i < n-1 ? p2[i+1] : (in->reflective ? p2[i] : -p2[i]);
                double v = (m->chi[1] * src[i] / k + m->s12 * p1[i] + b * (L + R))
                         / (2*b + m->sa[1]);
                diff = fmax(diff, fabs(v - p2[i]));
                p2[i] = v;
            }
            if (diff < 1e-13)
                break;
        }

        double F2 = 0;
        for (int i = 0; i < n; i++)
            F2 += c[i]->nusf[0] * p1[i] + c[i]->nusf[1] * p2[i];
        double knew = k * F2;
        if (fabs(knew / k - 1.0) < 1e-11 || *iters > 500) {
            k = knew;
            break;
        }
        k = knew;
    }

    free(src);
    free(c);
    return k;
}

static int run(const char *in_path, FILE *out)
{
    Input in = {0};
    if (parse_file(in_path, &in))
        return 1;

    int n = in.mesh;
    double *p1 = malloc(n * sizeof *p1);
    double *p2 = malloc(n * sizeof *p2);
    int iters;

    double k = solve(&in, p1, p2, &iters);
    double rho = (k - 1.0) / k * 1e5;

    if (in.has_title)
        fprintf(out, "title = %s\n", in.title);
    fprintf(out, "k_eff = %.6f\n", k);
    fprintf(out, "state = %s\n", k < 1.0 ? "subcritical" : k > 1.0 ? "supercritical" : "critical");
    fprintf(out, "rho_pcm = %+.2f\n", rho);
    fprintf(out, "outer_iterations = %d\n", iters);

    if (in.flux_out) {
        double m1 = 0, m2 = 0;
        for (int i = 0; i < n; i++) {
            m1 = fmax(m1, p1[i]);
            m2 = fmax(m2, p2[i]);
        }
        fprintf(out, "# x_cm phi_fast phi_thermal\n");
        for (int i = 0; i < n; i++)
            fprintf(out, "%8.3f %.6e %.6e\n", (i + 0.5) * (in.width / n), p1[i]/m1, p2[i]/m2);
    }

    free(p1);
    free(p2);
    return 0;
}

int main(int argc, char **argv)
{
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
