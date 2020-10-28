

#include <complex>
#include "../ext.h"
#include "expr.h"
typedef std::complex<double> complex;

#define MAX_WIDTH 3840
#define MAX_SINES 256

const int dx = 1;
int       N;
int       width = 1280, height = 720;
double    X[MAX_WIDTH + 10], Y[MAX_WIDTH + 10];
int       n_sines = 0;
double    A[MAX_SINES], L[MAX_SINES], P[MAX_SINES], C[MAX_SINES][3];
double    A0[MAX_SINES], L0[MAX_SINES], P0[MAX_SINES], PX[MAX_SINES];
int       selected[MAX_SINES] = {0}, n_selected = 0;
int       clickedState = 0;
double    X0, Y0;
int       mX, mY;
double    overLayLeft, overlayRight, overlayTop, overlayBottom;
int       newOverlay = 0;
int       drawCurves = 1;
int       resized    = 0;

int drawMode = 0, drawing = 0;
// coordinates from the user or from the grapher for fourier estimation
double fY[MAX_WIDTH + 10];
double drawingX[MAX_WIDTH + 10], drawingY[MAX_WIDTH + 10];
int    drawingIndex;
// for taking mathematical expressions
char expr[256]     = "";
char exprPos       = 0;
int  takeExprInput = 0;

void drawSines();
void drawAxis();
int  selectCurve(int x, int y);
void addSine();
void removeSine();
void deselectAll();
void drawFunction(double[], double[], int);
void drawHandDrawnCurve();

#define SUP_OVERLAY 1
#define SIN_OVERLAY 2
#define GEN_OVERLAY 3

int  overlayState = 0;
void drawSupOverLay();
void drawSinOverLay();
void drawGenOverLay();
void handleSupOverlay();
void handleSinOverlay();
void handleGenOverlay();
void drawTextBox();
void takeMathInput(unsigned char key);
void fourier();

int inOverlay(int x, int y);

void iDraw()
{
    iClear();
    if (drawCurves) drawSines();
    drawAxis();
    switch (overlayState) {
        case SUP_OVERLAY: drawSupOverLay(); break;
        case SIN_OVERLAY: drawSinOverLay(); break;
        case GEN_OVERLAY: drawGenOverLay(); break;
    }
    if (takeExprInput) {
        drawTextBox();
        for (int i = 0; i < MAX_WIDTH; i++)
            fY[i] = -INFINITY;
        if (resized || exprPlot(expr, drawFunction) && exprUpdated()) fourier();
    }
    if (drawing) drawHandDrawnCurve();
    if (resized) resized = 0;
}

void iPassiveMouseMove(int x, int y) { mX = x, mY = y; }

void iMouseMove(int x, int y)
{
    if (drawMode) {
        double ds = sqrt((x - X0) * (x - X0) + (y - Y0) * (y - Y0));
        if (ds > 5.0) {
            drawingX[drawingIndex] = x, drawingY[drawingIndex] = y;
            drawingIndex++;
            X0 = x;
            Y0 = y;
        }
    }
    else if (clickedState && n_selected > 0) {
        int one = n_selected == 1;
        for (int i = 0; i < n_sines; i++) {
            if (selected[i]) {
                A[i] = (y - height / 2) / (Y0 - height / 2) * A0[i];
                if (one) {
                    if (fabs(PX[i] - X0) < 5) continue;
                    L[i] = L0[i] * (x - PX[i]) / (X0 - PX[i]);
                }
                else
                    L[i] = L0[i] * x / X0;
                if (fabs(L0[i]) > 50) {
                    if (L[i] > 0)
                        L[i] = max(L[i], 50);
                    else if (L[i] < 0)
                        L[i] = min(L[i], -50);
                }
                P[i] = PX[i] - L[i] / L0[i] * (PX[i] - P0[i]);
            }
        }
    }
}

void iMouse(int button, int state, int x, int y)
{
    static int shouldDeselect = 0;
    if (state == GLUT_DOWN) {
        if (button < 3) {
            X0 = x, Y0 = y;
            if (button == GLUT_LEFT_BUTTON) {
                if (drawMode) {
                    for (int i = 0; i < width; i++) {
                        fY[i] = height / 2;
                        for (int j = 0; j < n_sines; j++)
                            fY[i] += A[j] * sin(2 * PI / L[j] * (i - P[j]));
                    }
                    drawing = 1;
                    // drawDir = 0;
                    return;
                }
                if (overlayState) {
                    if (inOverlay(x, y)) {
                        switch (overlayState) {
                            case SUP_OVERLAY: handleSupOverlay(); break;
                            case SIN_OVERLAY: handleSinOverlay(); break;
                            case GEN_OVERLAY: handleGenOverlay(); break;
                        }
                        return;
                    }
                    else
                        overlayState = 0;
                }
                clickedState = 1;
            }
            int r          = selectCurve(x, y);
            shouldDeselect = r > 1; // should deselect others on mouse up
            if (button == GLUT_RIGHT_BUTTON) {
                if (r > 1 || n_selected == 1)
                    overlayState = SIN_OVERLAY;
                else if (n_selected == n_sines)
                    overlayState = SUP_OVERLAY;
                else
                    overlayState = GEN_OVERLAY;
                newOverlay = 1;
            }
            if (r) {
                if (takeExprInput) takeExprInput = 0;
                // remember mouse click position and curve states
                for (int i = 0; i < n_sines; i++) {
                    A0[i] = A[i], L0[i] = L[i], P0[i] = P[i];
                    PX[i] = (2 * round(2 * (X0 - P[i]) / L[i] - 0.5) + 1) * L[i] / 4 + P[i]; // take the nearest peak;
                }
            }
            else {
                // deselect all since empty space was clicked
                deselectAll();
            }
        }
        else if (n_selected > 0) {
            if (button == 3 || button == 4) {
                int s = button == 3 ? 1 : -1;
                for (int j = 0; j < n_sines; j++)
                    if (selected[j]) P[j] = P[j] + s * 10;
            }
        }
    }
    else {
        if (drawMode && drawing) {
            drawMode = drawing = 0;
            for (int i = 1; i < drawingIndex; i++) {
                int s = drawingX[i] > drawingX[i - 1] ? 1 : -1;
                for (int x = drawingX[i - 1]; x < drawingX[i]; x += s)
                    fY[x] = (drawingY[i] - drawingY[i - 1]) / (drawingX[i] - drawingX[i - 1]) * (x - drawingX[i - 1]) +
                            drawingY[i - 1];
            }
            fourier();
        }
        if (n_selected > 0) {
            if (shouldDeselect) {
                if (x == X0 && y == Y0) {
                    for (int j = 0; j < n_sines; j++)
                        if (selected[j] < 0) selected[j] = 0;
                    n_selected = 1;
                }
                else {
                    for (int j = 0; j < n_sines; j++)
                        if (selected[j] < 0) selected[j] = 1;
                }
                shouldDeselect = 0;
            }
            // replace negative amplitude with L/2 phase shift
            for (int j = 0; j < n_sines; j++)
                if (selected[j] && A[j] < 0) P[j] += L[j] / 2, A[j] = -A[j];
        }
        clickedState = 0;
    }
}

void iResize(int w, int h)
{
    width = w, height = h;
    N = width / dx + 2;
    exprSetScreenRes(w, h);
    resized = 1;
}
void iKeyboard(unsigned char key)
{

    if (takeExprInput) takeMathInput(key);
    static int isFullScreen = 0;
    switch (key) {
        case 'Z' - 'A' + 1: // ctrl + Z
            for (int i = 0; i < n_sines; i++)
                A[i] = A0[i], L[i] = L0[i], P[i] = P0[i];
            break;
        case 'G' - 'A' + 1: // ctrl + G
            takeExprInput = !takeExprInput;
            break;
        case 27: // escape
            if (takeExprInput) { takeExprInput = 0; }
            else if (isFullScreen) {
                glutReshapeWindow(1280, 720);
                isFullScreen = 0;
            }
            break;
        case 'f':
        case 'F':
            if (isFullScreen)
                glutReshapeWindow(1280, 720);
            else
                glutFullScreen();
            isFullScreen = !isFullScreen;
            break;
        case 'q': exit(0); break;
        case '+':
            if (n_sines < MAX_SINES) addSine();
            break;
        case '-':
        case (unsigned char)127:
            if (n_sines >= 0) removeSine();
            break;
        case 'A' - 'A' + 1: { // ctrl + A
            for (int i = 0; i < n_sines; i++)
                selected[i] = 1;
            n_selected = n_sines;
        } break;
        default: break;
    }
}

void iSpecialKeyboard(unsigned char key)
{
    switch (key) {
        case GLUT_KEY_END: exit(0); break;
    }
}

int main()
{
    width = 1280, height = 720;
    N = width / dx + 2;
    double x;
    int    i;
    for (i = 0, x = 0; x <= MAX_WIDTH; x += dx, i++)
        X[i] = x;
    for (i = 0; i < 10; i++)
        addSine();
    iSetTransparency(1);
    iInitializeEx(width, height, "Demo!");
    return 0;
}

void drawAxis()
{
    double axisX[] = {0, (double)width}, axisY[] = {height / 2.0, height / 2.0};
    iSetColorEx(255, 255, 255, 1);
    iPath(axisX, axisY, 2, 3);
}
void drawSines()
{
    static double pX[4], pY[4], qY[MAX_WIDTH], cY[MAX_WIDTH];
    int           crossesAxis;
    pY[2] = pY[3] = height / 2;
    double alpha  = (n_sines <= 15) ? .2 : .1;
    for (int j = 0; j <= n_sines; j++) {
        for (int i = 0; i < N; i++) {
            if (j == 0) cY[i] = 0;
            pX[2] = pX[1] = X[i];
            if (j < n_sines) {
                qY[i] = pY[1] = height / 2 + A[j] * sin(2 * PI / L[j] * (X[i] - P[j]));
                cY[i] += qY[i] - height / 2;
                iSetColorEx(C[j][0], C[j][1], C[j][2], alpha);
            }
            else {
                qY[i] = pY[1] = cY[i] += height / 2;
                iSetColorEx(255, 255, 255, alpha / 2);
            }
            if (i > 0) {
                crossesAxis =
                    (pY[0] >= height / 2 && pY[1] < height / 2) || (pY[0] < height / 2 && pY[1] >= height / 2);
                if (crossesAxis) {
                    // draw two triangles instead of a quadrilateral
                    pX[3] = pX[1], pY[3] = pY[1], pX[2] = pX[0];
                    pX[1] = (height / 2 - pY[0]) * (pX[1] - pX[0]) / (pY[1] - pY[0]) + pX[0], pY[1] = height / 2;
                    iFilledPolygon(pX, pY, 3);
                    pX[2] = pX[0] = pX[3], pY[0] = pY[3];
                    iFilledPolygon(pX, pY, 3);
                    pX[3] = pX[0], pY[3] = height / 2;
                    continue;
                }
                iFilledPolygon(pX, pY, 4);
            }
            pX[3] = pX[0] = pX[1], pY[0] = pY[1];
        }
        double stroke;
        int    dashed = 0;
        if (j < n_sines) {
            iSetColorEx(C[j][0], C[j][1], C[j][2], 1.0);
            stroke = (n_sines <= 15) ? 2 : 1;
            if (selected[j]) stroke *= 1.5;
        }
        else {
            dashed = 1;
            iSetColorEx(255, 255, 255, 0.85);
            stroke = (n_sines <= 15) ? 2.5 : 1.5;
            if (n_selected == n_sines) {
                stroke *= 1.25;
                dashed = 0;
            }
        }
        iPath(X, qY, N, stroke, 0, dashed, 10, 5);
    }
}
// return 1 if just a curve is selected/ctrl-selected
// retursn 2 if a curve is selected and other previously selected ones should be deselected on mouse up
// returns 0 if no curve is selected
int selectCurve(int x, int y)
{
    y -= height / 2;
    double Y, C, minY = height;
    int    sine_index = -1;
    for (int i = 0; i <= n_sines; i++) {
        if (i < n_sines) {
            Y = A[i] * sin(2 * PI / L[i] * (x - P[i]));
            C += Y;
        }
        else
            Y = C;
        if (fabs(Y) < fabs(minY)) {
            if ((0 <= y && y <= Y + 2) || (0 >= y && y >= Y - 2)) {
                sine_index = i;
                minY       = Y;
            }
        }
    }
    if (sine_index == -1) return 0;
    int all = sine_index == n_sines, ctrlPressed = glutGetModifiers() & GLUT_ACTIVE_CTRL;
    if (selected[sine_index]) { // if already selected, mark others to be deselected on mouse up
        for (int i = 0; i < n_sines; i++)
            if (selected[i] && i != sine_index) selected[i] = -1;
        return 2;
    }
    else if (all) { // select all
        for (int i = 0; i < n_sines; i++)
            selected[i] = 1;
        n_selected = n_sines;
    }
    else if (ctrlPressed) { // just add this to selected if ctrl is pressed
        if (!selected[sine_index]) n_selected++;
        selected[sine_index] = 1;
    }
    else { // just select this
        for (int i = 0; i < n_sines; i++)
            selected[i] = i == sine_index;
        n_selected = 1;
    }
    return 1;
}
void addSine()
{
    A[n_sines] = iRandom(50, height / 4);
    L[n_sines] = iRandom(200, width / 2);
    P[n_sines] = iRandom(0, width);
    iRandomColor(1, 1, C[n_sines]);
    n_sines++;
}

void removeSine()
{
    if (n_selected > 0) {
        int i, r;
        for (i = 0, r = 0; i < n_sines; i++) {
            if (selected[i]) {
                r++;
                selected[i] = 0;
            }
            else if (r > 0) {
                A[i - r] = A[i], L[i - r] = L[i], P[i - r] = P[i];
                C[i - r][0] = C[i][0], C[i - r][1] = C[i][1], C[i - r][2] = C[i][2];
            }
        }
        n_sines -= r;
    }
    else
        n_sines--;
    n_selected = 0;
}

void drawTextBox()
{
    iSetColor(255, 255, 255);
    iRectangle(0, 0, width, 36);
    iText(14, 14, expr, GLUT_BITMAP_HELVETICA_18);
}

void takeMathInput(unsigned char key)
{
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
    exprUpdate();
}

// the scale and delta factors for the overlays
double supA, supF, supP;
double sinA, sinF, sinP;

int reverseBits(int v, int p)
{
    int ret = 0;
    for (int i = 0; i < p; i++)
        if (v & 1 << i) ret |= 1 << (p - i - 1);
    return ret;
}

void drawFunction(double X[], double Y[], int n)
{
    for (int i = 0; i < n; i++) {
        int k = (int)max(round(X[i]), 0.0);
        fY[k] = max(Y[i], fY[k]);
    }
    iSetColorEx(255, 255, 255, 0.15);
    iPath(X, Y, n, 3);
}

void drawHandDrawnCurve()
{
    iSetColorEx(255, 255, 255, 1.0);
    iPath(drawingX, drawingY, drawingIndex, 3, 0, 1, 10, 5);
}

void fourier()
{
    double x1, x2, y1, y2;

    int    l, r, t, lt;
    double dx;
    t = 0;
    while (!isfinite(fY[t]))
        t++;
    l = t, t = width - 1; // leftmost x-cord
    while (!isfinite(fY[t]))
        t--;
    r                   = t; // rightmost x-cord
    double a[MAX_SINES] = {0}, b[MAX_SINES] = {0};
    double period = r - l, w;
    w             = 2 * PI / period;
    for (t = l + 1, lt = l; t <= r;) {
        while (!isfinite(fY[t]))
            t++;
        x1 = lt - l, x2 = t - l;
        y1 = fY[lt] - height / 2.0, y2 = fY[t] - height / 2.0;
        dx = x2 - x1;
        // printf("x1: %lf, x2: %lf, y1: %lf, y2: %lf, dx: %lf\n", x1, x2, y1, y2, dx);
        for (int j = 1; j <= n_sines; j++) {
            a[j - 1] += 2 / period * (y1 * cos(j * w * x1) + y2 * cos(j * w * x2)) * dx / 2.0;
            b[j - 1] += 2 / period * (y1 * sin(j * w * x1) + y2 * sin(j * w * x2)) * dx / 2.0;
        }
        lt = t++;
        while (!isfinite(fY[t]))
            t++;
    }
    for (int i = 0; i < n_sines; i++) {
        A[i] = sqrt(a[i] * a[i] + b[i] * b[i]);
        L[i] = period / (i + 1);
        P[i] = (-atan2(a[i], b[i])) * L[i] / (2 * PI) + l;
    }
}

void drawBasicOverlay(int w, int h)
{
    if (newOverlay) {
        supA = supF = 1, supP = 0;
        sinA = sinF = 1, sinP = 0;
        if (X0 + w < width)
            overLayLeft = X0, overlayRight = X0 + w;
        else
            overLayLeft = X0 - w, overlayRight = X0;
        if (Y0 - h > 0)
            overlayBottom = Y0 - h, overlayTop = Y0;
        else
            overlayBottom = Y0, overlayTop = Y0 + h;
        newOverlay = 0;
    }
    iSetColorEx(0, 0, 0, 0.85);
    iFilledRectangle(overLayLeft, overlayBottom, w, h);
    // iSetColorEx(255, 255, 255, 1.0);
    // iRectangleEx(overLayLeft, overlayBottom, w, h, 2.5);
}

void drawMenu(const char* text, int w, int h, int dx, int dy, int filled, int outlined)
{
    if (filled) {
        if (overLayLeft + dx <= mX && mX <= overLayLeft + dx + w && overlayTop - dy <= mY &&
            mY <= overlayTop - dy + h) {
            iSetColorEx(255, 255, 255, 0.2);
            iFilledRectangle(overLayLeft + dx, overlayTop - dy, w, h);
        }
    }
    iSetColorEx(255, 255, 255, 1);
    if (outlined) iLine(overLayLeft + dx, overlayTop - dy, overLayLeft + dx + w, overlayTop - dy);
    iText(overLayLeft + dx + 10, overlayTop - dy + 10, text, GLUT_BITMAP_HELVETICA_12);
}

void drawScaleMenu(double x, int w, int h, int dx, int dy)
{
    char str[8];
    snprintf(str, 8, "%0.3f", x);
    drawMenu(str, w, h, dx, dy, 0, 0);
    drawMenu("-", 27, h, dx - w / 2, dy, 1, 0);
    drawMenu("+", 27, h, dx + w / 2 + 25, dy, 1, 0);
}

void drawColorPicker(int w, int h, int dx, int dy, int outlined)
{
    double rgb[3];
    int    mouseInside =
        overLayLeft + dx <= mX && mX <= overLayLeft + dx + w && overlayTop - dy <= mY && mY <= overlayTop - dy + h;
    for (int i = 0; i < w; i++) {
        double H = 360 * i / w;
        iHSVtoRGB(H, 1.0, 1.0, rgb);
        iSetColorEx(rgb[0], rgb[1], rgb[2], 0.5 + 0.5 * mouseInside);
        iLine(overLayLeft + i + dx, overlayTop - dy, overLayLeft + i + dx, overlayTop - dy + h);
    }
    if (outlined) {
        iSetColorEx(255, 255, 255, 1);
        iLine(overLayLeft + dx, overlayTop - dy, overLayLeft + dx + w, overlayTop - dy);
    }
}

void drawSupOverLay()
{
    static int w = 230, h = 210;
    drawBasicOverlay(w, h);
    drawMenu("Approximate graph of equation", w, 30, 0, 30, 1, 0);
    drawMenu("Approximate hand-drawn curve", w, 30, 0, 60, 1, 0);
    drawMenu("Scale Amplitude", w, 30, 0, 90, 0, 0);
    drawScaleMenu(supA, 40, 30, w - 75, 90);
    drawMenu("Scale Frequencies", w, 30, 0, 120, 0, 0);
    drawScaleMenu(supF, 40, 30, w - 75, 120);
    drawMenu("Change Phase", w, 30, 0, 150, 0, 0);
    drawScaleMenu(supP, 40, 30, w - 75, 150);
    drawMenu("Show Tracers", w, 30, 0, 180, 1, 0);
}
void deselectAll()
{
    // deselect all since empty space was clicked
    for (int j = 0; j < n_sines; j++)
        selected[j] = 0;
    n_selected = 0;
}
void handleSupOverlay()
{
    int dx = X0 - overLayLeft, dy = overlayTop - Y0;
    if (0 <= dy && dy <= 30) {
        // graph
        takeExprInput = 1;
        overlayState  = 0;
    }
    else if (dy <= 60) {
        // hand-draw
        drawMode     = 1;
        drawingIndex = 0;
        overlayState = 0;
        deselectAll();
    }
    else if (dy <= 90) {
        // scale amp
    }
    else if (dy <= 120) {
        // scale freq
    }
    else if (dy <= 150) {
        // change phase
    }
    else if (dy <= 180) {
        // show/hide tracers
    }
}
void drawSinOverLay()
{
    static int w = 230, h = 200;
    drawBasicOverlay(w, h);
    drawMenu("Change Color", w, 30, 0, 30, 0, 0);
    drawColorPicker(w, 20, 0, 50, 0);
    drawMenu("Scale Amplitude", w, 30, 0, 80, 0, 0);
    drawScaleMenu(sinA, 40, 30, w - 75, 80);
    drawMenu("Scale Frequencies", w, 30, 0, 110, 0, 0);
    drawScaleMenu(sinF, 40, 30, w - 75, 110);
    drawMenu("Change Phase", w, 30, 0, 140, 0, 0);
    drawScaleMenu(sinP, 40, 30, w - 75, 140);
    drawMenu("Show Tracer", w, 30, 0, 170, 1, 0);
    drawMenu("Remove Curve", w, 30, 0, 200, 1, 0);
}
void handleSinOverlay() {}
void drawGenOverLay()
{
    static int w = 250, h = 180;
    drawBasicOverlay(w, h);
    drawMenu("Select All Curves", w, 30, 0, 30, 1, 0);
    drawMenu("Add Curve", w, 30, 0, 60, 1, 0);
    drawMenu("Change Tracer Speed", w, 30, 0, 90, 1, 0);
    drawScaleMenu(sinP, 40, 30, w - 73, 90);
    drawMenu("Resume Tracers", w, 30, 0, 120, 1, 0);
    if (drawCurves)
        drawMenu("Hide Curves", w, 30, 0, 150, 1, 0);
    else
        drawMenu("Show Curves", w, 30, 0, 150, 1, 0);
    drawMenu("Show Help", w, 30, 0, 180, 1, 0);
}
void handleGenOverlay()
{
    int dx = X0 - overLayLeft, dy = overlayTop - Y0;
    if (0 <= dy && dy <= 30) {
        // select all
    }
    else if (dy <= 60) {
        addSine();
        overlayState = 0;
    }
    else if (dy <= 90) {
        // change tracer speed
    }
    else if (dy <= 120) {
        // resume tracer
    }
    else if (dy <= 150) {
        // show/hide curves
        drawCurves   = !drawCurves;
        overlayState = 0;
    }
    else if (dy <= 180) {
        // show/hide tracers
    }
}
int inOverlay(int x, int y) { return overLayLeft <= x && x <= overlayRight && overlayBottom <= y && y <= overlayTop; }