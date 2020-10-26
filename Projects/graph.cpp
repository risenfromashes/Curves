#include "../ext.h"
#include "expr.h"

const int width = 1280, height = 720;

#define MAX_POINTS 4096

const int steps = 20;
double    X[MAX_POINTS], Y[MAX_POINTS];
double    graphH = 5, graphW = 5;

#define GRID_SIZE 256

char expr[256] = "";
char expr_pos  = 0;
int  changed   = 0;

void reverseArray(double arr[], int n)
{
    for (int i = 0; i < n / 2; i++) {
        double t       = arr[i];
        arr[i]         = arr[n - i - 1];
        arr[n - i - 1] = t;
    }
}

// set all neighbours to be visited
// f = 0 indicates no solution
// f = -1 indicates solutions exist and it was visited
void markDone(int G[GRID_SIZE][GRID_SIZE], int h, int v, int f)
{
    G[h][v] = f;
    if (h > 0) {
        G[h - 1][v] = f;
        if (v > 0) G[h - 1][v - 1] = G[h][v - 1] = f;
        if (v < GRID_SIZE - 1) G[h - 1][v + 1] = G[h][v + 1] = f;
    }
    if (h < GRID_SIZE - 1) {
        G[h + 1][v] = f;
        if (v > 0) G[h + 1][v - 1] = f;
        if (v < GRID_SIZE - 1) G[h + 1][v + 1] = f;
    }
}

// ref: http://web.mit.edu/18.06/www/Spring17/Multidimensional-Newton.pdf
void trace(int G[GRID_SIZE][GRID_SIZE])
{
#define F(x, y) exprEval(expr, -2, x, y)
    const double err = 1e-5;
    double       x, y, x0, y0, F0, du, dt, dx, dy, Fx, Fy, D, d;
    du = 1e-6;
    dt = 10.0 / GRID_SIZE;
    for (int h = 0; h < GRID_SIZE; h++) {
        for (int v = 0; v < GRID_SIZE; v++) {
            if (G[h][v] > 0) {
                double l, t;
                l = -5.0 + 10.0 * h / GRID_SIZE;
                t = 5.0 - 10.0 * v / GRID_SIZE;

                double initX, initY, initDirX, initDirY;
                int    i, rev = 0, n_overlap = 0;
                x = l + dt / 2, y = t - dt / 2;
                iSetColorEx(0, 0, 255, 0.5);
                iCircle((x + 5.0) / 10.0 * width, (y - 5.0) / 10.0 * width + (width + height) / 2, 10);
                // iterate in the inital direction and after following the trail a while
                // go back to the initial point and go the other way
                // the first one is not a solution
                int r, undef = 0;
                for (i = 0; i <= MAX_POINTS; i++) {
                    if (i > 0)
                        X[i - 1] = (x + 5.0) / 10.0 * width, Y[i - 1] = (y - 5.0) / 10.0 * width + (width + height) / 2;
                    // use the gradient to approximate the next point in the first run
                    // improve solution in the next 3 runs
                    for (int j = 0; j <= 3; j++) {
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
                            x0 = x, y0 = y; // last point
                            if (i == 1) {
                                // printf("(%lf, %lf) -> (%lf, %lf)\n", x0, y0, x, y);
                                // printf("Fx: %lf, Fy: %lf, D: %lf\n", Fx, Fy, D);
                                initX = x, initY = y;
                            }
                            D  = sqrt(Fx * Fx + Fy * Fy);
                            dx = 0.5 * dt * Fy / D, dy = -0.5 * dt * Fx / D;
                            if (!rev)
                                x += dx, y += dy;
                            else
                                x -= dx, y -= dy;
                        }
                        else {
                            dx = x - x0;
                            dy = y - y0;
                            D  = Fx * dy - Fy * dx;
                            x -= F0 * dy / D, y += F0 * dx / D;
                        }
                        F0 = F(x, y);
                        if (exprGetError() & EXPR_UNDEFINED) {
                            undef = 1;
                            break;
                        }
                    }
                    int isSolution = !undef && fabs(F0) < 1e-5;
                    int inBoundary = -5.0 <= x && x <= 5.0 && -5.0 <= y && y <= 5.0;
                    int overlap    = 0;
                    if (i == 0) {
                        iSetColorEx(0, 255, 0, 0.5);
                        // printf("(%lf, %lf) -> (%lf, %lf)\n", x0, y0, x, y);
                        iCircle((x + 5.0) / 10.0 * width, (y - 5.0) / 10.0 * width + (width + height) / 2, 10);
                        if (!isSolution) {
                            // markDone(G, h, v, 0);
                            G[h][v] = 0;
                            break;
                        }
                        else
                            G[h][v] = -1;
                    }
                    if (isSolution && inBoundary) {
                        int H, V;
                        H = floor((5.0 + x) / 10.0 * GRID_SIZE);
                        V = ceil((5.0 - y) / 10.0 * GRID_SIZE);
                        // if (G[H][V] == -1)
                        //     n_overlap++;
                        // else
                        //     n_overlap = 0;
                        markDone(G, H, V, -1);
                        // if (n_overlap > 20) overlap = 1;
                    }
                    if (!isSolution || !inBoundary || overlap || i >= MAX_POINTS / 2) {
                        if (rev) {
                            iSetColor(255, 0, 0);
                            iPath(X + r, Y + r, i - r, 3);
                            break;
                        }
                        else {
                            r = i;
                            iSetColor(255, 0, 0);
                            iPath(X, Y, r, 3);
                            rev = 1;
                            x = initX, y = initY;
                            F0 = F(x, y);
                        }
                    }
                }
            }
        }
    }
#undef F
}

void drawAxii()
{
    iSetColor(255, 255, 255);
    iFilledRectangle(0, height / 2 - 2, width, 4);
    iFilledRectangle(width / 2 - 2, 0, 4, height);
}

void drawTextBox()
{
    iSetColor(255, 255, 255);
    iRectangle(0, 0, width, 36);
    iText(10, 10, expr, GLUT_BITMAP_HELVETICA_18);
}

void solveExpr()
{
    if (strlen(expr) > 0) {
        int G[GRID_SIZE][GRID_SIZE] = {1};
        int T                       = round(log2(GRID_SIZE));
        for (int k = 0; k <= T; k++) {
            int P = 1 << k;
            int Q = GRID_SIZE / P;
            for (int h = 0; h < GRID_SIZE; h += Q) {
                for (int v = 0; v < GRID_SIZE; v += Q) {
                    // starts from top-left
                    if (G[h][v]) {
                        // if (changed && k < 4) printf("h: %d, v: %d, P: %d\n", h, v, P);
                        struct interval x = exprCreateInterval(-5.0 + 10.0 * h / GRID_SIZE,
                                                               -5.0 + 10.0 * (h + Q) / GRID_SIZE),
                                        y = exprCreateInterval(5.0 - 10.0 * (v + Q) / GRID_SIZE,
                                                               5.0 - 10.0 * v / GRID_SIZE);
                        struct interval r = exprEvalInterval(expr, -2 + changed, x, y);
                        if (changed) changed = 0;
                        // if (changed && k < 4)
                        //     printf("([%lf,%lf],[%lf,%lf])->[%lf,%lf] <%d,%d>\n",
                        //            x.l,
                        //            x.r,
                        //            y.l,
                        //            y.r,
                        //            r.l,
                        //            r.r,
                        //            r.def,
                        //            r.cont);
                        if (r.def && (r.l <= 0 && 0 <= r.r)) {
                            if (P == GRID_SIZE) {
                                if (r.cont && isfinite(r.l) && isfinite(r.r))
                                    G[h][v] = 1;
                                else
                                    G[h][v] = 0;
                            }
                            else {
                                int d       = GRID_SIZE / P / 2;
                                G[h + d][v] = G[h + d][v + d] = G[h][v + d] = 1;
                            }
                        }
                        else
                            G[h][v] = 0;
                    }
                }
            }
        }
        trace(G);
        for (int h = 0; h < GRID_SIZE; h++) {
            for (int v = 0; v < GRID_SIZE; v++) {
                if (G[h][v]) {
                    double d = (double)width / GRID_SIZE;
                    double x = h * d, y = -v * d + (width + height) / 2.0;
                    iSetColorEx(255, 255, 255, 0.1);
                    iRectangle(x, y - d, d, d);
                }
            }
        }
    }
}
void iDraw()
{
    iClear();
    drawAxii();
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
        printf("([%lf,%lf],[%lf,%lf])->[%lf,%lf] <%d,%d>\n", x.l, x.r, y.l, y.r, r.l, r.r, r.def, r.cont);
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
