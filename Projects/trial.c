#define EXPR_DEBUG_LOG 1
#include "expr.h"

int main()
{
    char expr[EXPR_MAX_LEN] = "e^-x";
    // scanf("%[^n]", &expr);
    // printf("expr")
    printf("%lf\n", exprEval(expr, -1, 1, 1));
    if (exprGetError()) printf("error flag: %d\n", exprGetError());
    printf("%lf\n", sin(INFINITY));
    // struct interval x, y;
    // x                 = exprCreateInterval(-1.25, 0.0);
    // struct interval r = exprEvalInterval("pi", -1, x, y);
    // // struct interval r = exprDiv(exprTan(x), x);
    // printf("%lf, %lf\n", r.l, r.r);
    // printf("%d, %d\n", r.def, r.cont);
}