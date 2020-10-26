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
    double l, p1, p2, r, t;
    l = sin(x.l), r = sin(x.r);
    t   = ceil(2 * x.l / PI) * PI / 2;
    p1  = t < x.r ? sin(t) : l;
    p2  = t + PI < x.r ? sin(t + PI) : l;
    v.l = min(l, min(p1, min(p2, r)));
    v.r = max(l, max(p1, max(p2, r)));
    return v;
}
struct interval exprCos(struct interval x)
{
    struct interval v;
    v.def  = x.def;
    v.cont = x.cont;
    double l, p1, p2, r, t;
    l = cos(x.l), r = cos(x.r);
    t   = ceil(2 * x.l / PI) * PI / 2;
    p1  = t < x.r ? cos(t) : l;
    p2  = t + PI < x.r ? cos(t + PI) : l;
    v.l = min(l, min(p1, min(p2, r)));
    v.r = max(l, max(p1, max(p2, r)));
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
    if (l == -1 || l == -2) { // new evaluation
        i = 0;                // reset counter
        error_flags &= 3;     // clear point specific flags
        if (l == -1) {        // if string is new
            error_flags = 0;  // clear all flags
            len         = strlen(expr);
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
        else {
            if (first)
                first = 0;
            else
                p[denom] *= c, c = 1;
            if (expr[i] == '+' || expr[i] == '-' || expr[i] == '=' || expr[i] == ')' || expr[i] == '\0') {
                if (until_op) break;
                s += p[0] / p[1];
                c = p[0] = p[1] = 1, denom = 0;
                if (expr[i] == '-')
                    p[0] = -1;
                else if (expr[i] == '=')
                    s = -s;
                else if (expr[i] == ')') {
#if EXPR_DEBUG_LOG
                    printf("[bracket call] Returning %lf (%d/%d)\n", s, i, l);
#endif
                    return s;
                }
                else if (expr[i] == '\0')
                    return s;
            }
            else if (expr[i] == 'x')
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
                    printf("ERROR: Unkown symbol at %d [%d] (l: %d)\n", i, (int)expr[i], l);
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
    if (l == -1 || l == -2) { // new evaluation
        i = 0;                // reset counter
        error_flags &= 3;     // clear point specific flags
        if (l == -1) {        // if string is new
            error_flags = 0;  // clear all flags
            len         = strlen(expr);
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
        else {
            if (first)
                first = 0;
            else
                p[denom] = exprMul(p[denom], c), c = unit;
            if (expr[i] == '+' || expr[i] == '-' || expr[i] == '=' || expr[i] == ')' || expr[i] == '\0') {
                if (until_op) break;
                s = exprAdd(s, exprDiv(p[0], p[1]));
                c = p[0] = p[1] = unit, denom = 0;
                if (expr[i] == '-')
                    p[0] = exprNeg(p[0]);
                else if (expr[i] == '=')
                    s = exprNeg(s);
                else if (expr[i] == ')') {
#if EXPR_DEBUG_LOG
                    printf("[bracket call] Returning [%lf,%lf] (%d/%d)\n", s.l, s.r, i, l);
#endif
                    return s;
                }
                else if (expr[i] == '\0')
                    return s;
            }
            else if (expr[i] == 'x')
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
