#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#ifndef PI
#define PI 3.1415926535897932
#endif
#ifndef ERR
#define ERR 1e-8
#endif
#ifndef min
#define min(x, y) ((x < y) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) ((x > y) ? (x) : (y))
#endif

#define EXPR_MAX_LEN 128

static unsigned int error_flags = 0;
// error enums
#define EXPR_UNMATCHED_BRACKET 1
#define EXPR_UNKNOWN_SYMBOL    2
#define EXPR_UNDEFINED         4
// function enums
#define EXPR_SIN  0
#define EXPR_COS  1
#define EXPR_TAN  2
#define EXPR_ASIN 3
#define EXPR_ACOS 4
#define EXPR_ATAN 5
#define EXPR_LOG  6
#define EXPR_ABS  7
#define EXPR_SQRT 8
#define EXPR_CBRT 9

// Using methods described here: http://www.dgp.toronto.edu/~mooncake/papers/SIGGRAPH2001_Tupper.pdf
// and multivariate Newton's method

struct interval {
    double l, r;
    int    def;  // defined at atleast one point
    int    cont; // continuous at all points
};

int             exprIsConst(struct interval x) { return x.def && fabs(x.l - x.r) < ERR; }
struct interval exprCreateInterval(double l, double r)
{
    struct interval v;
    v.l = l, v.r = r;
    v.cont = v.def = 1;
    return v;
}
struct interval exprToInterval(double x)
{
    struct interval v;
    v.l = v.r = x;
    v.cont = v.def = 1;
    return v;
}
struct interval exprUndef()
{
    struct interval v;
    v.def = 0;
    return v;
}

struct interval exprMul(struct interval x, struct interval y)
{
    struct interval v;
    v.def  = x.def && y.def;
    v.cont = x.cont && y.cont;
    if (!v.def) return v;
    if (!x.cont)
        return x;
    else if (!y.cont)
        return y;
    v.l = min(x.l * y.l, min(x.l * y.r, min(x.r * y.l, x.r * y.r)));
    v.r = max(x.l * y.l, max(x.l * y.r, max(x.r * y.l, x.r * y.r)));
    return v;
}

struct interval exprMulC(struct interval x, double c)
{
    struct interval v;
    double          l = x.l * c, r = x.r * c;
    v.def  = x.def;
    v.cont = x.cont;
    if (!v.def) return v;
    v.l = min(l, r);
    v.r = max(l, r);
    return v;
}

struct interval exprInv(struct interval x)
{
    struct interval v;
    v.def  = x.def;
    v.cont = x.cont;
    double l, r;
    if (x.l < 0 && x.r >= 0) {
        l = -INFINITY, r = INFINITY;
        v.cont = 0;
    }
    else
        l = 1 / x.r, r = 1 / x.l;
    v.l = l;
    v.r = r;
    return v;
}

struct interval exprDiv(struct interval x, struct interval y) { return exprMul(x, exprInv(y)); }

struct interval exprAdd(struct interval x, struct interval y)
{
    struct interval v;
    v.def  = x.def && y.def;
    v.cont = x.cont && y.cont;
    if (!v.def) return v;
    v.l = x.l + y.l;
    v.r = x.r + y.r;
    return v;
}
struct interval exprNeg(struct interval x)
{
    struct interval v;
    v.def  = x.def;
    v.cont = x.cont;
    v.l    = -x.r;
    v.r    = -x.l;
    return v;
}
struct interval exprSub(struct interval x, struct interval y) { return exprAdd(x, exprNeg(y)); }

// TODO: implement cont here
struct interval exprPowEC(struct interval x, double n)
{
    struct interval v;
    double          l = pow(x.l, n), r = pow(x.r, n);
    v.def  = (!isnan(l) || !isnan(r)) && x.def; // atleast one has a defined value
    v.cont = x.cont;
    if (!v.def) return v;
    v.l = min(l, r);
    if (x.l <= 0 && 0 <= x.r) {
        if (min(l, r) >= 0) v.l = 0;
        if (n < 0) v.cont = 0;
    }
    v.r = max(l, r);
    return v;
}
struct interval exprPowBC(double a, struct interval x)
{

    struct interval v;
    double          l = pow(a, x.l), r = pow(a, x.r);
    v.def  = (a >= 0) && x.def;
    v.cont = x.cont;
    if (a == 0 && x.l < 0) v.cont = 0;
    if (!v.def) return v;
    v.l = min(l, r);
    v.r = max(l, r);
    return v;
}
struct interval exprPow(struct interval x, struct interval y)
{
    struct interval v;
    v.def  = (x.r >= 0) && x.def;
    v.cont = x.cont && y.cont;
    if (!v.def) return v;
    if (x.l < 0) {
        x.l = 0;
        if (y.l < 0) v.cont = 0;
    }
    double p, q, r, s;
    p = pow(x.l, y.l), q = pow(x.l, y.r), r = pow(x.r, y.l), s = pow(x.r, y.r);
    v.l = min(p, min(q, min(r, s)));
    v.l = max(p, max(q, max(r, s)));
    return v;
}

struct interval exprSin(struct interval x)
{
    struct interval v;
    v.def  = x.def;
    v.cont = x.cont;
    if (!isfinite(x.l) || !isfinite(x.r)) { v.l = -1, v.r = 1; }
    else {
        double l, p1, p2, r, t;
        l = sin(x.l), r = sin(x.r);
        t   = ceil(2 * x.l / PI) * PI / 2;
        p1  = t < x.r ? sin(t) : l;
        p2  = t + PI < x.r ? sin(t + PI) : l;
        v.l = min(l, min(p1, min(p2, r)));
        v.r = max(l, max(p1, max(p2, r)));
    }
    return v;
}
struct interval exprCos(struct interval x)
{
    struct interval v;
    v.def  = x.def;
    v.cont = x.cont;
    if (!isfinite(x.l) || !isfinite(x.r)) { v.l = -1, v.r = 1; }
    else {
        double l, p1, p2, r, t;
        l = cos(x.l), r = cos(x.r);
        t   = ceil(2 * x.l / PI) * PI / 2;
        p1  = t < x.r ? cos(t) : l;
        p2  = t + PI < x.r ? cos(t + PI) : l;
        v.l = min(l, min(p1, min(p2, r)));
        v.r = max(l, max(p1, max(p2, r)));
    }
    return v;
}
struct interval exprTan(struct interval x)
{
    struct interval v;
    double          l = tan(x.l), r = tan(x.r);
    v.def  = x.def;
    v.cont = x.cont;
    if (l < r && x.r - x.l < PI) {
        v.l = l;
        v.r = r;
    }
    else {
        v.l    = -INFINITY;
        v.r    = INFINITY;
        v.cont = 0;
    }
    return v;
}
struct interval exprAsin(struct interval x)
{
    struct interval v;
    v.def  = (-1 <= x.r && x.l <= 1) && x.def;
    v.cont = x.cont;
    if (!v.def) return v;
    if (x.l < -1)
        v.l = -PI / 2;
    else
        v.l = asin(x.l);
    if (x.r > 1)
        v.r = PI / 2;
    else
        v.r = asin(x.r);
    return v;
}
struct interval exprAcos(struct interval x)
{
    struct interval v;
    v.def  = (-1 <= x.r && x.l <= 1) && x.def;
    v.cont = x.cont;
    if (!v.def) return v;
    if (x.l < -1)
        v.r = PI;
    else
        v.r = acos(x.l);
    if (x.r > 1)
        v.l = 0;
    else
        v.l = acos(x.r);
    return v;
}
struct interval exprAtan(struct interval x)
{
    struct interval v;
    v.def  = x.def;
    v.cont = x.cont;
    v.l    = atan(x.l);
    v.r    = atan(x.r);
    return v;
}
struct interval exprLog(struct interval x)
{
    struct interval v;
    v.def  = (x.r >= 0) && x.def;
    v.cont = x.cont;
    if (!v.def) return v;
    if (x.l <= 0) {
        v.l    = -INFINITY;
        v.cont = 0;
    }
    else
        v.l = log(x.l);
    v.r = log(x.r); // if x.r < 0 it won't be defined in the interval anyway
    return v;
}
struct interval exprAbs(struct interval x)
{
    struct interval v;
    v.def  = x.def;
    v.cont = x.cont;
    if (!v.def) return v;
    double l = fabs(x.l), r = fabs(x.r);
    v.l = min(l, r);
    v.r = max(l, r);
    return v;
}
struct interval exprSqrt(struct interval x)
{
    struct interval v;
    v.def  = (x.r >= 0) && x.def;
    v.cont = x.cont;
    if (!v.def) return v;
    if (x.l <= 0 && 0 <= x.r)
        v.l = 0;
    else
        v.l = sqrt(x.l);
    v.r = sqrt(x.r);
    return v;
}
struct interval exprCbrt(struct interval x)
{
    struct interval v;
    v.def    = x.def;
    v.cont   = x.cont;
    double l = cbrt(x.l), r = cbrt(x.r);
    v.l = min(l, r);
    v.r = max(l, r);
    return v;
}
int exprGetError() { return error_flags; }

static int len, bracketEnds[EXPR_MAX_LEN + 1];
static int exprMatchBrackets(const char* expr)
{
    int i, r;
    len = strlen(expr);
    for (i = len - 1, r = -1; i >= 0; i--) {
        if (expr[i] == ')')
            bracketEnds[i] = r, r = i;
        else if (expr[i] == '(') {
            if (r == -1) {
                error_flags |= EXPR_UNMATCHED_BRACKET;
                return 0;
            }
            bracketEnds[i] = r, r = bracketEnds[r];
        }
    }
    if (r != -1) {
        error_flags |= EXPR_UNMATCHED_BRACKET;
        return 0;
    }
#if EXPR_DEBUG_LOG
    for (int i = 0; i < len; i++)
        printf("%2d ", i);
    printf("\n");
    for (int i = 0; i < len; i++)
        printf("%2d ", bracketEnds[i]);
    printf("\n");
    for (int i = 0; i < len; i++)
        printf(" %c ", expr[i]);
    printf("\n");
#endif
    return 1;
}

// Evaluates expression
// l = -1 indicates the string is new and its the first call
// l = -2 indicates its the same as the one in the last call
// l = -3 indicates until_op
// indicates the index to end otherwise
double exprEval(const char* expr, int l, double x, double y)
{
#if EXPR_DEBUG_LOG
    printf("Called with l: %d\n", l);
#endif
    static int i;
    int        until_op = 0;
    if (l == -1 || l == -2) {                       // new evaluation
        i = 0;                                      // reset counter
        error_flags &= 3;                           // clear point specific flags
        if (l == -1) {                              // if string is new
            error_flags = 0;                        // clear all flags
            if (!exprMatchBrackets(expr)) return 0; // remember bracket endings
        }
        l = len;
    }
    else if (l == -3) {
        until_op = 1;
        l        = len;
    }
    int    denom = 0, first = 1;
    double s, p[2], c, t;
    c = s = 0.0;
    p[0] = p[1] = 1;

#if EXPR_DEBUG_LOG
    printf("Starting loop with l: %d, len: %d\n", l, len);
#endif
    for (; i <= l; i++) {
        while (expr[i] == ' ')
            i++;
#if EXPR_DEBUG_LOG
        if (until_op) printf("[until op] ");
        if (denom)
            printf("%d/%d) %lf + %lf/(%lf) [%lf] -%c->\n", i, l, s, p[0], p[1], c, expr[i]);
        else
            printf("%d/%d) %lf + (%lf)/%lf [%lf] -%c->\n", i, l, s, p[0], p[1], c, expr[i]);
#endif
        if (expr[i] == '^') {
            i++, t = exprEval(expr, -3, x, y);
            if (exprGetError()) return 0;
            c = pow(c, t);
            if (isnan(c)) {
                error_flags |= EXPR_UNDEFINED;
                return 0;
            }
        }
        else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '=' || expr[i] == ')' || expr[i] == '\0') {
            p[denom] *= c;
            if (until_op && !(expr[i] == '-' && first)) break;
            if (first) first = 0;
            s += p[0] / p[1];
            c = p[0] = p[1] = 1, denom = 0;
            if (expr[i] == '-')
                p[0] = -1;
            else if (expr[i] == '=')
                s = -s, c = 0, first = 1;
            else if (expr[i] == ')') {
#if EXPR_DEBUG_LOG
                printf("[bracket call] Returning %lf (%d/%d)\n", s, i, l);
#endif
                return s;
            }
            else if (expr[i] == '\0')
                return s;
        }
        else {
            if (first)
                first = 0;
            else
                p[denom] *= c, c = 1;
            if (expr[i] == 'x')
                c = x;
            else if (expr[i] == 'y')
                c = y;
            else if (expr[i] == 'e')
                c = exp(1);
            else if (!strnicmp(expr + i, "pi", 2))
                c = PI, i++;
            else if (isdigit(expr[i]) || expr[i] == '.') {
                char* end;
                c = strtod(expr + i, &end);
                i = end - expr - 1;
            }
            else if (expr[i] == '*') {
                if (until_op) break;
                denom = 0;
            }
            else if (expr[i] == '/') {
                if (until_op) break;
                denom = 1;
            }
            else if (expr[i] == '(') {
                t = exprEval(expr, bracketEnds[i++], x, y);
                if (exprGetError()) return 0;
                c = t;
            }
            else {
                int f;
                if (!strncmp(expr + i, "sin", 3))
                    i += 3, f = EXPR_SIN;
                else if (!strncmp(expr + i, "cos", 3))
                    i += 3, f = EXPR_COS;
                else if (!strncmp(expr + i, "tan", 3))
                    i += 3, f = EXPR_TAN;
                else if (!strncmp(expr + i, "arcsin", 6))
                    i += 6, f = EXPR_ASIN;
                else if (!strncmp(expr + i, "arccos", 6))
                    i += 6, f = EXPR_ACOS;
                else if (!strncmp(expr + i, "arctan", 6))
                    i += 6, f = EXPR_ATAN;
                else if (!strncmp(expr + i, "ln", 2))
                    i += 2, f = EXPR_LOG;
                else if (!strncmp(expr + i, "log", 3))
                    i += 3, f = EXPR_LOG;
                else if (!strncmp(expr + i, "sqrt", 4))
                    i += 4, f = EXPR_SQRT;
                else if (!strncmp(expr + i, "cbrt", 4))
                    i += 4, f = EXPR_CBRT;
                else if (!strncmp(expr + i, "abs", 3))
                    i += 3, f = EXPR_ABS;
                else {
                    error_flags |= EXPR_UNKNOWN_SYMBOL;
#if EXPR_DEBUG_LOG
                    printf("ERROR: Unkown symbol at %d [%d] (l: %d)\n", i, (int)expr[i], l);
#endif
                    return 0;
                }
                if (expr[i] == '(')
                    t = exprEval(expr, bracketEnds[i++], x, y);
                else
                    t = exprEval(expr, -3, x, y);
                if (exprGetError()) return 0;
                switch (f) {
                    case EXPR_SIN: c = sin(t); break;
                    case EXPR_COS: c = cos(t); break;
                    case EXPR_TAN: c = tan(t); break;
                    case EXPR_ASIN:
                    case EXPR_ACOS:
                        if (t > 1.0 || t < -1.0) {
                            error_flags |= EXPR_UNDEFINED;
                            return 0;
                        }
                        if (f == EXPR_ASIN)
                            c = asin(t);
                        else
                            c = acos(t);
                        break;
                    case EXPR_ATAN: c = atan(t); break;
                    case EXPR_LOG:
                        if (t < 0) {
                            error_flags |= EXPR_UNDEFINED;
                            return 0;
                        }
                        c = log(t);
                        break;
                    case EXPR_ABS: c = fabs(t); break;
                    case EXPR_SQRT:
                        if (t < 0) {
                            error_flags |= EXPR_UNDEFINED;
                            return 0;
                        }
                        c = sqrt(t);
                        break;
                    case EXPR_CBRT: c = cbrt(t); break;
                }
            }
        }
#if EXPR_DEBUG_LOG
        if (denom)
            printf("%d/%d) %lf + %lf/(%lf) [%lf]\n", i, l, s, p[0], p[1], c);
        else
            printf("%d/%d) %lf + (%lf)/%lf [%lf]\n", i, l, s, p[0], p[1], c);
#endif
    }
    if (until_op) {
#if EXPR_DEBUG_LOG
        printf("[until op] Returning %lf\n", p[0]);
#endif
        i--;
        return p[0];
    }
    return s;
}

// l = -1 indicates the string is new and its the first call
// l = -2 indicates its the same as the one in the last call
// l = -3 indicates until_op
// indicates the index to end otherwise
struct interval exprEvalInterval(const char* expr, int l, struct interval x, struct interval y)
{
#if EXPR_DEBUG_LOG
    printf("Called with l: %d\n", l);
#endif
    static int             f = 1;
    static struct interval unit, zero;
    if (f) {
        unit = exprToInterval(1), zero = exprToInterval(0);
        f = 0;
    }
    static int i;
    int        until_op = 0;
    if (l == -1 || l == -2) {        // new evaluation
        i = 0;                       // reset counter
        error_flags &= 3;            // clear point specific flags
        if (l == -1) {               // if string is new
            error_flags = 0;         // clear all flags
            exprMatchBrackets(expr); // remember bracket endings
        }
        l = len;
    }
    else if (l == -3) {
        until_op = 1;
        l        = len;
    }
    int             denom = 0, first = 1;
    struct interval s, p[2], c, t;
    c = s = zero;
    p[0] = p[1] = unit;
#if EXPR_DEBUG_LOG
    printf("Starting loop with l: %d, len: %d\n", l, len);
#endif
    for (; i <= l; i++) {
        while (expr[i] == ' ')
            i++;
#if EXPR_DEBUG_LOG
        if (until_op) printf("[until op] ");
        if (denom)
            printf("%d/%d) [%lf,%lf] + [%lf,%lf]/([%lf, %lf]) ", i, l, s.l, s.r, p[0].l, p[0].r, p[1].l, p[1].r);
        else
            printf("%d/%d) [%lf,%lf] + ([%lf,%lf])/[%lf, %lf] ", i, l, s.l, s.r, p[0].l, p[0].r, p[1].l, p[1].r);
        printf(" [[%lf,%lf]] -%c->\n", c.l, c.r, expr[i]);
#endif
        if (expr[i] == '^') {
            i++, t = exprEvalInterval(expr, -3, x, y);
            if (!t.def) return t;
            int bc, ec;
            bc = exprIsConst(c), ec = exprIsConst(t);
            if (bc && ec)
                c = exprToInterval(pow(c.l, t.l));
            else if (bc)
                c = exprPowBC(c.l, t);
            else if (ec)
                c = exprPowEC(c, t.l);
            else
                c = exprPow(c, t);
        }
        else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '=' || expr[i] == ')' || expr[i] == '\0') {
            p[denom] = exprMul(p[denom], c);
            if (until_op && !(expr[i] == '-' && first)) break;
            if (first) first = 0;
            s = exprAdd(s, exprDiv(p[0], p[1]));
            c = p[0] = p[1] = unit, denom = 0;
            if (expr[i] == '-')
                p[0] = exprNeg(p[0]);
            else if (expr[i] == '=')
                s = exprNeg(s), c = zero, first = 1;
            else if (expr[i] == ')') {
#if EXPR_DEBUG_LOG
                printf("[bracket call] Returning [%lf,%lf] (%d/%d)\n", s.l, s.r, i, l);
#endif
                return s;
            }
            else if (expr[i] == '\0')
                return s;
        }
        else {
            if (first)
                first = 0;
            else
                p[denom] = exprMul(p[denom], c), c = unit;

            if (expr[i] == 'x')
                c = x;
            else if (expr[i] == 'y')
                c = y;
            else if (expr[i] == 'e')
                c = exprToInterval(exp(1));
            else if (!strnicmp(expr + i, "pi", 2))
                i++, c = exprToInterval(PI);
            else if (isdigit(expr[i]) || expr[i] == '.') {
                char* end;
                c = exprToInterval(strtod(expr + i, &end));
                i = end - expr - 1;
            }
            else if (expr[i] == '/') {
                if (until_op) break;
                denom = 1;
            }
            else if (expr[i] == '*') {
                if (until_op) break;
                denom = 0;
            }
            else if (expr[i] == '(') {
                t = exprEvalInterval(expr, bracketEnds[i++], x, y);
                if (!t.def) return t;
                c = t;
            }
            else {
                int f;
                if (!strncmp(expr + i, "sin", 3))
                    i += 3, f = EXPR_SIN;
                else if (!strncmp(expr + i, "cos", 3))
                    i += 3, f = EXPR_COS;
                else if (!strncmp(expr + i, "tan", 3))
                    i += 3, f = EXPR_TAN;
                else if (!strncmp(expr + i, "arcsin", 6))
                    i += 6, f = EXPR_ASIN;
                else if (!strncmp(expr + i, "arccos", 6))
                    i += 6, f = EXPR_ACOS;
                else if (!strncmp(expr + i, "arctan", 6))
                    i += 6, f = EXPR_ATAN;
                else if (!strncmp(expr + i, "ln", 2))
                    i += 2, f = EXPR_LOG;
                else if (!strncmp(expr + i, "log", 3))
                    i += 3, f = EXPR_LOG;
                else if (!strncmp(expr + i, "sqrt", 4))
                    i += 4, f = EXPR_SQRT;
                else if (!strncmp(expr + i, "cbrt", 4))
                    i += 4, f = EXPR_CBRT;
                else if (!strncmp(expr + i, "abs", 3))
                    i += 3, f = EXPR_ABS;
                else {
                    error_flags |= EXPR_UNKNOWN_SYMBOL;
#if EXPR_DEBUG_LOG
                    printf("ERROR: Unkown symbol at %d [%d] (l: %d)\n", i, (int)expr[i], l);
#endif
                    return exprUndef();
                }
                if (expr[i] == '(')
                    t = exprEvalInterval(expr, bracketEnds[i++], x, y);
                else
                    t = exprEvalInterval(expr, -3, x, y);
                if (!t.def) return t;
                switch (f) {
                    case EXPR_SIN: c = exprSin(t); break;
                    case EXPR_COS: c = exprCos(t); break;
                    case EXPR_TAN: c = exprTan(t); break;
                    case EXPR_ASIN: c = exprAsin(t); break;
                    case EXPR_ACOS: c = exprAcos(t); break;
                    case EXPR_ATAN: c = exprAtan(t); break;
                    case EXPR_LOG: c = exprLog(t); break;
                    case EXPR_ABS: c = exprAbs(t); break;
                    case EXPR_SQRT: c = exprSqrt(t); break;
                    case EXPR_CBRT: c = exprCbrt(t); break;
                }
            }
        }
#if EXPR_DEBUG_LOG
        if (denom)
            printf("%d/%d) [%lf,%lf] + [%lf,%lf]/([%lf, %lf]) ", i, l, s.l, s.r, p[0].l, p[0].r, p[1].l, p[1].r);
        else
            printf("%d/%d) [%lf,%lf] + ([%lf,%lf])/[%lf, %lf] ", i, l, s.l, s.r, p[0].l, p[0].r, p[1].l, p[1].r);
        printf(" [[%lf,%lf]]\n", c.l, c.r);
#endif
    }
    if (until_op) {
#if EXPR_DEBUG_LOG
        printf("[until op] Returning [%lf,%lf]\n", p[0].l, p[0].r);
#endif
        i--;
        return p[0];
    }
    return s;
}

static int exprIsOp(char c)
{
    switch (c) {
        case '+':
        case '-':
        case '^':
        case '*':
        case '/':
        case '=': return 1;
    }
    return 0;
}

int exprIsValid(const char* expr)
{
    int i, len, lastOp = 0, eq = 0;
    for (i = 0; expr[i]; i++) {
        // invalid if there are consecutive operators
        if (expr[i] == '=') eq = 1;
        if (exprIsOp(expr[i])) {
            if (lastOp) {
                if (expr[i] == '+' || expr[i] == '-') {
                    if (expr[i - 1] == '+' || expr[i - 1] == '-') return 0;
                }
            }
            lastOp = 1;
        }
        else
            lastOp = 0;
    }
    len = i;
    if (len > 0 && eq == 1) {
        char f = expr[0], l = expr[len - 1];
        switch (f) {
            case '^':
            case '*':
            case '/':
            case '=': return 0;
        }
        switch (l) {
            case '+':
            case '-':
            case '^':
            case '*':
            case '/':
            case '=': return 0;
        }
        return exprMatchBrackets(expr);
    }
    return 0;
}

static int exprScreenWidth = 1280, exprScreenHeight = 720;

void exprSetScreenRes(int w, int h) { exprScreenWidth = w, exprScreenHeight = h; }

#define EXPR_MAX_POINTS 4096

static double exprCurveX[EXPR_MAX_POINTS + 10], exprCurveY[EXPR_MAX_POINTS + 10];

// limit of the graph space surrounding the origin
static double rX0 = 5, lX0 = -5, tY0 = 5, bY0 = -5;
static double rX, lX, tY, bY;
static double exprScale = 1.0;

#define EXPR_GRID_SIZE 256

static int G[EXPR_GRID_SIZE + 10][EXPR_GRID_SIZE + 10];

void exprInit() { rX = rX0, lX = lX0, tY = tY0, bY = bY0; }

void exprScaleBy(double del)
{
    if ((del < 0.0 && rX - lX > 0.5 && tY - bY > 0.5) || (del > 0.0 && rX - lX < 50.0 && tY - bY < 50.0)) {
        lX -= del, rX += del;
        bY -= del, tY += del;
    }
}
void exprSetInitBounds(double l, double r, double b, double t)
{
    rX0 = r, lX0 = l, tY0 = t, bY0 = b;
    rX = rX0, lX = lX0, tY = tY0, bY = bY0;
}
// scale by screen coordinate scale factor
void exprScaleBounds(double s)
{
    exprScale = 1.0 / s;
    lX = lX0 * exprScale, rX = rX0 * exprScale;
    bY = bY0 * exprScale, tY = tY0 * exprScale;
}
// pan by screen coordinates
void exprPan(double sX, double sY)
{
    double dx = -(rX0 - lX0) / exprScreenWidth * sX;
    double dy = -(rX0 - lX0) / exprScreenWidth * sY;
    lX = (lX0 + dx) * exprScale, rX = (rX0 + dx) * exprScale;
    bY = (bY0 + dy) * exprScale, tY = (tY0 + dy) * exprScale;
}

double exprLeft() { return lX; }
double exprRight() { return rX; }
double exprTop() { return tY; }
double exprBottom() { return bY; }

double exprInitLeft() { return lX0; }
double exprInitRight() { return rX0; }
double exprInitTop() { return tY0; }
double exprInitBottom() { return bY0; }

static int getGridH(double x) { return floor((x - lX) / (rX - lX) * EXPR_GRID_SIZE); }
static int getGridV(double y) { return ceil((tY - y) / (tY - bY) * EXPR_GRID_SIZE); }
double     exprGetScreenX(double x) { return (x - lX) / (rX - lX) * exprScreenWidth; }
double     exprGetScreenY(double y)
{
    return (y - tY) / (tY - bY) * exprScreenWidth + (exprScreenWidth + exprScreenHeight) / 2.0;
}

double exprLength(double sL) { return (rX - lX) / exprScreenWidth * sL; }

double exprScreenLength(double gL) { return exprScreenWidth / (rX - lX) * gL; }

static double getGridMidX(int h) { return lX + (rX - lX) * (h + 0.5) / EXPR_GRID_SIZE; }
static double getGridMidY(int v) { return tY - (tY - bY) * (v + 0.5) / EXPR_GRID_SIZE; }

static void markDone(int h, int v, int f)
{
    G[h][v] = f;
    if (h > 0) {
        G[h - 1][v] = f;
        if (v > 0) G[h - 1][v - 1] = G[h][v - 1] = f;
        if (v < EXPR_GRID_SIZE - 1) G[h - 1][v + 1] = G[h][v + 1] = f;
    }
    if (h < EXPR_GRID_SIZE - 1) {
        G[h + 1][v] = f;
        if (v > 0) G[h + 1][v - 1] = f;
        if (v < EXPR_GRID_SIZE - 1) G[h + 1][v + 1] = f;
    }
}

// ref: http://web.mit.edu/18.06/www/Spring17/Multidimensional-Newton.pdf
static void exprTraceCurves(const char* expr, void (*drawFunc)(double[], double[], int))
{
#define F(x, y) exprEval(expr, -2, x, y)
    const double err = 1e-7;
    double       x, y, x0, y0, F0, du, ds, dx, dy, Fx, Fy, D, d;
    du = 1e-6;
    ds = min(rX - lX, tY - bY) / EXPR_GRID_SIZE / 2.0;
    for (int h = 0; h < EXPR_GRID_SIZE; h++) {
        for (int v = 0; v < EXPR_GRID_SIZE; v++) {
            if (G[h][v] > 0) {
                double initX, initY;
                int    i, j, H, V, rev = 0, n_overlap = 0;
                x = getGridMidX(h), y = getGridMidY(v);
                // iSetColorEx(0, 0, 255, 0.5);
                // iCircle(exprGetScreenX(x), exprGetScreenY(y), 10);
                // iterate in the inital direction and after following the trail a while
                // go back to the initial point and go the other way
                // the first one is not a solution
                int s, r, undef = 0;
                for (i = 0; i <= EXPR_MAX_POINTS; i++) {
                    if (i > 0) {
                        exprCurveX[i - 1] = exprGetScreenX(x), exprCurveY[i - 1] = exprGetScreenY(y);
                        if (i == 1) initX = x, initY = y;
                    }
                    for (j = 0; j <= 3; j++) {
                        Fx = (F(x + du, y) - F0) / du;
                        if (exprGetError() & EXPR_UNDEFINED) {
                            undef = 1;
                            break;
                        }
                        Fy = (F(x, y + du) - F0) / du;
                        if (exprGetError() & EXPR_UNDEFINED) {
                            undef = 1;
                            break;
                        }
                        if (j == 0) {
                            // use the gradient to approximate the next point in the first run
                            x0 = x, y0 = y; // last point
                            D  = sqrt(Fx * Fx + Fy * Fy);
                            dx = ds * Fy / D, dy = -ds * Fx / D;
                            s = 1 - 2 * rev;
                            if (fabs(Fx) > 1e2)
                                dx = s * ds, dy = 0;
                            else if (fabs(Fy) > 1e2)
                                dx = 0, dy = s * ds;
                            else
                                dx = s * ds * Fy / D, dy = -s * ds * Fx / D;
                            x += dx, y += dy;
                        }
                        else {
                            // improve solution in the next 3 runs
                            dx = x - x0;
                            dy = y - y0;
                            D  = Fx * dy - Fy * dx; // this will fail for unit slopes and some other cases maybe
                            // so try to find solutions with the same distance on the x-direction then
                            if (fabs(D) < 1e-7) {
                                if (fabs(Fy) < 1e5)
                                    y += F0 / Fy;
                                else
                                    x += F0 / Fx; // what if both fails?
                            }
                            else
                                x -= F0 * dy / D, y += F0 * dx / D;
                        }
                        F0 = F(x, y);
                        if (exprGetError() & EXPR_UNDEFINED) {
                            undef = 1;
                            break;
                        }
                    }
                    int isSolution = !undef && (fabs(F0) < 1e-5);
                    int inBoundary = lX <= x && x <= rX && bY <= y && y <= tY;
                    if (i == 0) {
                        // iSetColorEx(0, 255, 0, 0.5);
                        // iCircle(exprGetScreenX(initX), exprGetScreenY(initY), 10);
                    }
                    H = getGridH(x), V = getGridV(y);
                    if (inBoundary) {
                        if (isSolution) {
                            if (G[H][V] == -1 && i == 0)
                                if (i == 0) break;
                            markDone(H, V, -1);
                        }
                        else
                            markDone(H, V, 0);
                    }
                    if (!isSolution || !inBoundary || (!rev && i >= EXPR_MAX_POINTS / 2) ||
                        (i == EXPR_MAX_POINTS - 1)) {
                        if (rev) {
                            drawFunc(exprCurveX + r, exprCurveY + r, i - r);
                            break;
                        }
                        else if (i > 1) {
                            r = i;
                            drawFunc(exprCurveX, exprCurveY, r);
                            if (i < EXPR_MAX_POINTS - 1) {
                                rev       = 1;
                                n_overlap = 0;
                                x = initX, y = initY;
                                F0 = F(x, y);
                            }
                            else
                                break;
                        }
                        else
                            break;
                    }
                }
            }
        }
    }
#undef F
}

static int exprChanged = 0, exprChangedExt = 0;
void       exprUpdate() { exprChanged = exprChangedExt = 1; }
int        exprUpdated()
{
    int ret = exprChangedExt && !exprChanged;
    if (ret) exprChangedExt = 0;
    return ret;
}
int exprPlot(const char* expr, void (*drawFunc)(double[], double[], int))
{
    if (!exprIsValid(expr)) return 0;
    int n_divs = 0;
    for (int h = 0; h < EXPR_GRID_SIZE; h++)
        for (int v = 0; v < EXPR_GRID_SIZE; v++)
            G[h][v] = 0;
    G[0][0] = 1;
    int T   = round(log2(EXPR_GRID_SIZE));
    for (int k = 0; k <= T; k++) {
        int P = 1 << k;
        int Q = EXPR_GRID_SIZE / P;
        for (int h = 0; h < EXPR_GRID_SIZE; h += Q) {
            for (int v = 0; v < EXPR_GRID_SIZE; v += Q) {
                // starts from top-left
                if (G[h][v]) {
                    struct interval x = exprCreateInterval(lX + (rX - lX) * h / EXPR_GRID_SIZE,
                                                           lX + (rX - lX) * (h + Q) / EXPR_GRID_SIZE),
                                    y = exprCreateInterval(tY - (tY - bY) * (v + Q) / EXPR_GRID_SIZE,
                                                           tY - (tY - bY) * v / EXPR_GRID_SIZE);
                    struct interval r = exprEvalInterval(expr, -1 + exprChanged, x, y);
                    if (exprChanged) exprChanged = 0;
                    if (exprGetError()) return 0;
                    if (r.def && (r.l <= 0 && 0 <= r.r)) {
                        if (P == EXPR_GRID_SIZE) {
                            if (r.cont && isfinite(r.l) && isfinite(r.r))
                                G[h][v] = 1, n_divs++;

                            else
                                G[h][v] = 0;
                        }
                        else {
                            int d       = EXPR_GRID_SIZE / P / 2;
                            G[h + d][v] = G[h + d][v + d] = G[h][v + d] = 1;
                        }
                    }
                    else
                        G[h][v] = 0;
                }
            }
        }
    }
    if ((double)n_divs / (EXPR_GRID_SIZE * EXPR_GRID_SIZE) > 0.05) return 0;
    exprTraceCurves(expr, drawFunc);
    return 1;
}