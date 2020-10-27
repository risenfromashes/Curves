#include "../ext.h"
#include "expr.h"

int width = 1280, height = 720;

#define MAX_POINTS 4096

const int steps = 20;
double    X[MAX_POINTS], Y[MAX_POINTS];

// limit of the graph space surrounding the origin
double rX = 5, lX = -5, tY = 5, bY = -5;

// percentage of the graph-space rendered (in the x direction)
double P = 0.9;

#define GRID_SIZE 256

int G[GRID_SIZE][GRID_SIZE];

char expr[256] = "";
char exprPos   = 0;
int  changed   = 0;

void reverseArray(double arr[], int n)
{
    for (int i = 0; i < n / 2; i++) {
        double t       = arr[i];
        arr[i]         = arr[n - i - 1];
        arr[n - i - 1] = t;
    }
}

int    getGridH(double x) { return floor((x - lX) / (rX - lX) * GRID_SIZE); }
int    getGridV(double y) { return ceil((tY - y) / (tY - bY) * GRID_SIZE); }
double getScreenX(double x) { return (x - lX) / (rX - lX) * width; }
double getScreenY(double y) { return (y - tY) / (tY - bY) * width + (width + height) / 2.0; }
double getGridMidX(int h) { return lX + (rX - lX) * (h + 0.5) / GRID_SIZE; }
double getGridMidY(int v) { return tY - (tY - bY) * (v + 0.5) / GRID_SIZE; }

void markDone(int h, int v, int f)
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
void trace()
{
#define F(x, y) exprEval(expr, -2, x, y)
    const double err = 1e-7;
    double       x, y, x0, y0, F0, du, ds, dx, dy, Fx, Fy, D, d;
    du = 1e-6;
    ds = min(rX - lX, tY - bY) / GRID_SIZE / 2.0;
    for (int h = 0; h < GRID_SIZE; h++) {
        for (int v = 0; v < GRID_SIZE; v++) {
            if (G[h][v] > 0) {
                double initX, initY;
                int    i, j, H, V, rev = 0, n_overlap = 0;
                x = getGridMidX(h), y = getGridMidY(v);
                iSetColorEx(0, 0, 255, 0.5);
                iCircle(getScreenX(x), getScreenY(y), 10);
                // iterate in the inital direction and after following the trail a while
                // go back to the initial point and go the other way
                // the first one is not a solution
                int s, r, undef = 0;
                for (i = 0; i <= MAX_POINTS; i++) {
                    if (i > 0) {
                        X[i - 1] = getScreenX(x), Y[i - 1] = getScreenY(y);
                        if (i == 1) initX = x, initY = y;
                    }
                    // use the gradient to approximate the next point in the first run
                    // improve solution in the next 3 runs
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
                            x0 = x, y0 = y; // last point
                            D  = sqrt(Fx * Fx + Fy * Fy);
                            dx = ds * Fy / D, dy = -ds * Fx / D;
                            s = 1 - 2 * rev;
                            if (fabs(Fx) > 1e5)
                                dx = s * ds, dy = 0;
                            else if (fabs(Fy) > 1e5)
                                dx = 0, dy = s * ds;
                            else
                                dx = s * ds * Fy / D, dy = -s * ds * Fx / D;
                            x += dx, y += dy;
                        }
                        else {
                            dx = x - x0;
                            dy = y - y0;
                            D  = Fx * dy - Fy * dx; // this will fail for unit slopes and some other cases maybe
                            // so try to find solutions with the same distance on the x-direction then
                            if (fabs(D) < 1e-7)
                                y += F0 / Fy;
                            else
                                x -= F0 * dy / D, y += F0 * dx / D;
                        }
                        F0 = F(x, y);
                        if (exprGetError() & EXPR_UNDEFINED) {
                            undef = 1;
                            break;
                        }
                    }
                    int isSolution = !undef && (fabs(F0) < 1e-7);
                    int inBoundary = lX <= x && x <= rX && bY <= y && y <= tY;
                    int overlap    = 0;
                    if (i == 0) {
                        iSetColorEx(0, 255, 0, 0.5);
                        iCircle(getScreenX(initX), getScreenY(initY), 10);
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
                    if (!isSolution || !inBoundary || overlap || (!rev && i >= MAX_POINTS / 2)) {
                        if (rev) {
                            iSetColor(255, 0, 0);
                            iPath(X + r, Y + r, i - r, 4);
                            break;
                        }
                        else if (i > 1) {
                            r = i;
                            iSetColor(255, 0, 0);
                            iPath(X, Y, r, 4);
                            rev       = 1;
                            n_overlap = 0;
                            x = initX, y = initY;
                            F0 = F(x, y);
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

void drawAxii()
{
    iSetColor(255, 255, 255);
    iFilledRectangle(0, height / 2 - 2.5, width, 7);
    iFilledRectangle(width / 2 - 2.5, 0, 7, height);
}

void drawTextBox()
{
    iSetColor(255, 255, 255);
    iRectangle(0, 0, width, 36);
    iText(14, 14, expr, GLUT_BITMAP_HELVETICA_18);
}

void solveExpr()
{
    if (exprIsValid(expr)) {
        int n_divs = 0;
        for (int h = 0; h < GRID_SIZE; h++)
            for (int v = 0; v < GRID_SIZE; v++)
                G[h][v] = 0;
        G[0][0] = 1;
        int T   = round(log2(GRID_SIZE));
        for (int k = 0; k <= T; k++) {
            int P = 1 << k;
            int Q = GRID_SIZE / P;
            for (int h = 0; h < GRID_SIZE; h += Q) {
                for (int v = 0; v < GRID_SIZE; v += Q) {
                    // starts from top-left
                    if (G[h][v]) {
                        // if (changed && k < 4) printf("h: %d, v: %d, P: %d\n", h, v, P);
                        struct interval x = exprCreateInterval(lX + (rX - lX) * h / GRID_SIZE,
                                                               lX + (rX - lX) * (h + Q) / GRID_SIZE),
                                        y = exprCreateInterval(tY - (tY - bY) * (v + Q) / GRID_SIZE,
                                                               tY - (tY - bY) * v / GRID_SIZE);
                        struct interval r = exprEvalInterval(expr, -2 + changed, x, y);
                        if (changed) changed = 0;
                        if (exprGetError()) return;
                        if (r.def && (r.l <= 0 && 0 <= r.r)) {
                            if (P == GRID_SIZE) {
                                if (r.cont && isfinite(r.l) && isfinite(r.r))
                                    G[h][v] = 1, n_divs++;
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
        if ((double)n_divs / (GRID_SIZE * GRID_SIZE) > 0.05) return;
        trace();
    }
}
void iDraw()
{
    iClear();
    drawAxii();
    drawTextBox();
    solveExpr();
}
void iMouseMove(int mx, int my) {}

void iMouse(int button, int state, int mx, int my)
{
    if (state == GLUT_DOWN) {
        // double d = (double)width / GRID_SIZE;
        // int    h = round(mx / d), v = round((-my + (width + height) / 2) / d);
        // printf("h: %d, v: %d, P: %d\n", h, v);
        // struct interval x = exprCreateInterval(-7.0 + 14.0 * h / GRID_SIZE, -7.0 + 14.0 * (h + 1) / GRID_SIZE),
        //                 y = exprCreateInterval(7.0 - 14.0 * (v + 1) / GRID_SIZE, 7.0 - 14.0 * v / GRID_SIZE);
        // struct interval r = exprEvalInterval(expr, -1, x, y);
        // printf("([%lf,%lf],[%lf,%lf])->[%lf,%lf] <%d,%d>\n", x.l, x.r, y.l, y.r, r.l, r.r, r.def, r.cont);
        if (button == 3) {
            if (rX - lX > 0.5 && tY - bY > 0.5) {
                lX += 0.1, rX -= 0.1;
                bY += 0.1, tY -= 0.1;
            }
        }
        else {
            lX -= 0.1, rX += 0.1;
            bY -= 0.1, tY += 0.1;
        }
    }
}
void iResize(int w, int h) { width = w, height = h; }
void iKeyboard(unsigned char key)
{
    changed = 1;
    if (isalnum(key))
        expr[exprPos++] = (char)key;
    else {
        switch (key) {
            case 22: // Ctrl + V
            {
                HWND hnd = GetActiveWindow();
                if (OpenClipboard(hnd)) {
                    if (IsClipboardFormatAvailable(CF_TEXT)) {
                        const char* text = (const char*)GetClipboardData(CF_TEXT);
                        for (int i = 0; text[i]; i++)
                            if (text[i] != ' ') expr[exprPos++] = text[i];
                    }
                    CloseClipboard();
                }
            } break;
            case ' ':
            case '=':
            case '*':
            case '/':
            case '+':
            case '-':
            case '^':
            case '.':
            case '(':
            case ')': expr[exprPos++] = (char)key; break;
            case '\b':
                if (exprPos > 0) expr[--exprPos] = '\0';
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
