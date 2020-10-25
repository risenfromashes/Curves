#include "../ext.h"
#include "expr.h"

const int width = 1280, height = 720;

const int steps = 20;
double    X[2048], Y[2048];
double    graphH = 5, graphW = 5;

#define GRID_SIZE 512

char expr[256] = "";
char expr_pos  = 0;
int  changed   = 0;
// #define GRID_SIZE 16

// void trace()
// {
//     iClear();
//     int    i;
//     double x, y, x0, y0, F0, du, dt, dx, dy, Fx, Fy, D;
//     du = 1e-5;
//     dt = 0.5;
//     x = -1.5, y = 0;
//     for (i = 0; i < steps; i++) {
//         X[i] = height * x / 2 / 2 + width / 2, Y[i] = height * y / 2 / 2 + height / 2;
//         F0 = F(x, y);
//         Fx = (F(x + du, y) - F0) / du;
//         Fy = (F(x, y + du) - F0) / du;
//         D  = sqrt(Fx * Fx + Fy * Fy);
//         dx = dt * Fy / D, dy = -dt * Fx / D;
//         // if (dx < 0) dx = -dx, dy = -dy;
//         x0 = x, y0 = y;
//         x += dx, y += dy;
//         for (int j = 0; j < 1; j++) {
//             F0 = F(x, y);
//             Fx = (F(x + du, y) - F0) / du;
//             Fy = (F(x, y + du) - F0) / du;
//             dx = x - x0;
//             dy = y - y0;
//             D  = Fx * dy - Fy * dx;
//             x -= F0 * dy / D, y += F0 * dx / D;
//         }
//     }
//     iPath(X, Y, steps, 2);
// }
void drawTextBox()
{

    iRectangle(0, 0, width, 36);
    iText(10, 10, expr, GLUT_BITMAP_HELVETICA_18);
}

void solveExpr()
{
    if (strlen(expr) > 4) {
        int G[GRID_SIZE][GRID_SIZE] = {1};
        int T                       = round(log2(GRID_SIZE));
        for (int k = 0; k <= T; k++) {
            int P = 1 << k;
            int Q = GRID_SIZE / P;
            for (int h = 0; h < GRID_SIZE; h += Q) {
                for (int v = 0; v < GRID_SIZE; v += Q) {
                    // starts from top-left
                    if (G[h][v]) {
                        // if (changed) printf("h: %d, v: %d, P: %d\n", h, v, P);
                        struct interval x = exprCreateInterval(-5.0 + 10.0 * h / GRID_SIZE,
                                                               -5.0 + 10.0 * (h + Q) / GRID_SIZE),
                                        y = exprCreateInterval(5.0 - 10.0 * (v + Q) / GRID_SIZE,
                                                               5.0 - 10.0 * v / GRID_SIZE);
                        struct interval r = exprEvalInterval(expr, -1, x, y);
                        // if (changed) printf("([%lf,%lf],[%lf,%lf])->[%lf,%lf]\n", x.l, x.r, y.l, y.r, r.l, r.r);
                        if ((r.dl || r.dr) && (r.l <= 0 && 0 <= r.r)) {
                            if (P == GRID_SIZE) {
                                double d = (double)width / GRID_SIZE;
                                double x = h * d, y = -v * d + (width + height) / 2.0;
                                iRectangle(x, y - d, d, d);
                            }
                            else {
                                int d       = GRID_SIZE / P / 2;
                                G[h + d][v] = G[h + d][v + d] = G[h][v + d] = 1;
                            }
                        }
                        else
                            G[h][k] = 0;
                    }
                }
            }
        }
        if (changed) changed = 0;
    }
}
void iDraw()
{
    iClear();
    drawTextBox();
    solveExpr();
}
void iResize(int, int) {}
void iMouseMove(int mx, int my) {}

void iMouse(int button, int state, int mx, int my)
{
    if (state == GLUT_DOWN) {
        double d = (double)width / GRID_SIZE;
        int    h = round(mx / d), v = round((-my + (width + height) / 2) / d);
        printf("h: %d, v: %d, P: %d\n", h, v);
        struct interval x = exprCreateInterval(-5.0 + 10.0 * h / GRID_SIZE, -5.0 + 10.0 * (h + 1) / GRID_SIZE),
                        y = exprCreateInterval(5.0 - 10.0 * (v + 1) / GRID_SIZE, 5.0 - 10.0 * v / GRID_SIZE);
        struct interval r = exprEvalInterval(expr, -1, x, y);
        printf("([%lf,%lf],[%lf,%lf])->[%lf,%lf]\n", x.l, x.r, y.l, y.r, r.l, r.r);
    }
}

void iKeyboard(unsigned char key)
{
    changed = 1;
    if (isalnum(key))
        expr[expr_pos++] = (char)key;
    else {
        switch (key) {
            case ' ':
            case '=':
            case '*':
            case '/':
            case '+':
            case '-':
            case '^':
            case '.':
            case '(':
            case ')': expr[expr_pos++] = (char)key; break;
            case '\b':
                if (expr_pos > 0) expr[--expr_pos] = '\0';
                break;
            default: break;
        }
    }
    // place your codes for other keys here
}

void iSpecialKeyboard(unsigned char key)
{

    if (key == GLUT_KEY_END) { exit(0); }
    // place your codes for other keys here
}

int main()
{

    iSetTransparency(1);
    iInitializeEx(width, height, "Demo!");
    return 0;
}
