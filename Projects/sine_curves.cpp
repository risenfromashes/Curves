/*
 *
 * "Curves"
 * Submission for the mid-term project for CSE 102, BUET
 *
 * A visualizer and a feature-rich editor for sinusoids.
 *
 * Ashrafur Rahman
 * October, 2020
 *
 */

#include "ext.h"
#include "expr.h"

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
int       newOverlay   = 0;
int       drawCurves   = 1;
int       resized      = 0;
int       isFullScreen = 0;

int showGrid = 0;

int drawMode = 0, drawing = 0;
// coordinates from the user or from the grapher for fourier estimation
double fY[MAX_WIDTH + 10];
double drawingX[MAX_WIDTH + 10], drawingY[MAX_WIDTH + 10];
int    drawingIndex;
// for integer input
char numStr[8] = "";
int  numPos    = 0;
// for taking mathematical expressions
char   expr[256] = "";
int    exprPos   = 0;
int    graphMode = 0, integerInput = 0;
double originX = width / 2.0, originY = height / 2.0;
double panX = 0, panY = 0.0;
double panX0 = 0, panY0 = 0.0;
int    panning = 0, panningActive = 0;
double scale = 1.0;

double tracerX[MAX_SINES + 5], tracerY[MAX_SINES + 5];
int    tracerState[MAX_SINES + 5] = {0}; // first bit set if tracer showed, second bit set if tracer paused
double tracerPt[MAX_SINES + 5] = {0}, tracerdt[MAX_SINES + 5] = {0};
double tracerSyncPt, tracerSyncdt                             = 0.0;
int    tracersSynced = 0, tracersUnidirectional = 1;
double tracerSpeed      = 150.0; // pixel/second
int    tracerButtonMode = 0;

double markedCosine[MAX_SINES + 5] = {0};

int drawSummation = 1;

int showHelp = 0;

double amplitude(int index);
double frequency(int index);
double phase(int index);
void   equation(int index, char* str, int size);

void zoom(int dir, int x, int y);
void pan(double dx, double dy);
void backToOrigin();
void drawSines();
void drawAxis();
void drawGrid();
void pauseTracer(int);
void resumeTracer(int);
void showTracer(int);
void hideTracer(int);
void locateTracers();
void hideAllTracers();
void showAllTracers();
void resumeAllTracers();
void pauseAllTracers();
void drawTracers();
void changeTracerSpeed(int dir);
void drawBottomOverlay();
void handleBottomOverlay(int);
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
void drawHelpScreen();
void handleSupOverlay(int);
void handleSinOverlay(int);
void handleGenOverlay(int);
void drawTextBox();
int  takeIntInput(unsigned char key);
void takeMathInput(unsigned char key);
void fourier();

int inOverlay(int x, int y);

void iDraw()
{
    if (clickedState && Y0 <= 22)
        handleBottomOverlay(1);
    else if (overlayState && clickedState && inOverlay(X0, Y0)) {
        switch (overlayState) {
            case SUP_OVERLAY: handleSupOverlay(1); break;
            case SIN_OVERLAY: handleSinOverlay(1); break;
            case GEN_OVERLAY: handleGenOverlay(1); break;
        }
    }
    static double scale0 = scale;
    static double t0     = -1;
    double        t      = iGetTime();
    if (scale0 != scale) {
        t0     = t;
        scale0 = scale;
    }
    iClear();
    if (showGrid || graphMode || panningActive || t < t0 + 0.25) drawGrid();
    drawAxis();
    if (graphMode) {
        for (int i = 0; i < MAX_WIDTH; i++)
            fY[i] = -INFINITY;
        if ((exprPlot(expr, drawFunction) && exprUpdated()) || resized) fourier();
    }
    locateTracers();
    drawSines();
    drawTracers();
    switch (overlayState) {
        case SUP_OVERLAY: drawSupOverLay(); break;
        case SIN_OVERLAY: drawSinOverLay(); break;
        case GEN_OVERLAY: drawGenOverLay(); break;
    }
    if (drawing) drawHandDrawnCurve();
    if (graphMode) drawTextBox();
    drawBottomOverlay();
    if (resized) resized = 0;
    if (showHelp) drawHelpScreen();
}

void iPassiveMouseMove(int x, int y) { mX = x, mY = y; }

void iMouseMove(int x, int y)
{
    if (overlayState) {
        if (inOverlay(x, y)) {
            X0 = x, Y0 = y;
            switch (overlayState) {
                case SUP_OVERLAY: handleSupOverlay(1); break;
                case SIN_OVERLAY: handleSinOverlay(1); break;
                case GEN_OVERLAY: handleGenOverlay(1); break;
            }
        }
    }
    else if (drawMode) {
        double ds = sqrt((x - X0) * (x - X0) + (y - Y0) * (y - Y0));
        if (ds > 5.0) {
            drawingX[drawingIndex] = x, drawingY[drawingIndex] = y;
            drawingIndex++;
            X0 = x;
            Y0 = y;
        }
    }
    else if (panning) {
        if (!panningActive) panningActive = 1;
        panX = panX0 + x - X0, panY = panY0 + y - Y0;
        originX = width / 2.0 + panX;
        originY = height / 2.0 + panY;
        exprPan(panX, panY);
    }
    else if (clickedState && n_selected > 0) {
        int one = n_selected == 1;
        for (int i = 0; i < n_sines; i++) {
            if (selected[i]) {
                if (fabs(Y0 - originY) > 10.0) A[i] = (y - originY) / (Y0 - originY) * A0[i];
                if (one) {
                    if (fabs(PX[i] - X0) < 10.0) continue;
                    L[i] = L0[i] * (x - PX[i]) / (X0 - PX[i]);
                }
                else {
                    double x0 = X0;
                    if (x0 - originX < -20)
                        x0 -= 20;
                    else if (x0 - originX < 20)
                        x0 += 20;
                    if (fabs(x - originX) >= 5.0) L[i] = L0[i] * (x - originX) / (x0 - originX);
                }
                if (fabs(L0[i]) >= 50.0) {
                    if (L[i] > 0) // min wavelength threshold
                        L[i] = max(L[i], 50);
                    else if (L[i] < 0)
                        L[i] = min(L[i], -50);
                }
                P[i] = (1.0 - L[i] / L0[i]) * (PX[i] - originX) / scale +
                       L[i] / L0[i] * P0[i]; // change phase to keep the curves at the same place
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
                clickedState = 1;
                integerInput = 0;
                if (y <= 22) {
                    deselectAll();
                    handleBottomOverlay(0);
                    return;
                }
                if (showHelp) {
                    if (x >= width - 25 && y >= height - 25) {
                        showHelp = 0;
                        return;
                    }
                }
                if (drawMode) {
                    // remember state of superposition
                    for (int i = 0; i < width; i++) {
                        fY[i] = originY;
                        for (int j = 0; j < n_sines; j++)
                            fY[i] += A[j] * sin(2 * PI / L[j] * ((i - originX) / scale - P[j])) * scale;
                    }
                    drawing = 1;
                    return;
                }
                if (overlayState) {
                    if (inOverlay(x, y)) {
                        switch (overlayState) {
                            case SUP_OVERLAY: handleSupOverlay(0); break;
                            case SIN_OVERLAY: handleSinOverlay(0); break;
                            case GEN_OVERLAY: handleGenOverlay(0); break;
                        }
                        return;
                    }
                    else
                        overlayState = 0;
                }
            }
            int r          = selectCurve(x, y);
            shouldDeselect = r > 1; // should deselect others on mouse up
            if (r) {
                if (graphMode) graphMode = 0;
                // remember mouse click position and curve states
                for (int i = 0; i < n_sines; i++) {
                    A0[i] = A[i], L0[i] = L[i], P0[i] = P[i]; // (2m + 1)pi/2 = 2pi/L((x-X)/z - P)
                    PX[i] =
                        originX + scale * (L[i] / 4 * (2 * round(2 / L[i] * ((x - originX) / scale - P[i]) - 0.5) + 1) +
                                           P[i]); // take the x coord of the nearest peak
                }
            }
            else {
                // deselect all since empty space was clicked
                deselectAll();
                if (button == GLUT_LEFT_BUTTON) {
                    panning = 1;
                    panX0 = panX, panY0 = panY;
                    originX = width / 2.0 + panX;
                    originY = height / 2.0 + panY;
                }
            }
            if (button == GLUT_RIGHT_BUTTON) {
                if (r > 1 || n_selected == 1)
                    overlayState = SIN_OVERLAY;
                else if (n_selected == n_sines)
                    overlayState = SUP_OVERLAY;
                else
                    overlayState = GEN_OVERLAY;
                newOverlay = 1;
            }
        }
        else if (button == 3 || button == 4) {
            if (n_selected > 0) {
                int s = button == 3 ? 1 : -1;
                for (int j = 0; j < n_sines; j++)
                    if (selected[j]) P[j] = P[j] + s * 10;
            }
            else {
                if (button == 3) {
                    if (scale < 1e5) zoom(1, x, y);
                }
                else {
                    if (scale > 5e-5) zoom(-1, x, y);
                }
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
                            drawingY[i - 1]; // interpolate pointts
            }
            fourier();
        }
        // stop input on click on empty space
        if (graphMode && panning && sqrt((x - X0) * (x - X0) + (y - Y0) * (y - Y0)) < 3.0) graphMode = 0;
        if (panning) panningActive = panning = 0;
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
            // replace negative amplitude and wavelength with L/2 phase shift
            for (int j = 0; j < n_sines; j++) {
                if (selected[j] && A[j] < 0) P[j] += L[j] / 2, A[j] = -A[j];
                // if (selected[j] && L[j] < 0) P[j] += L[j] / 2, L[j] = -L[j];
            }
        }
        clickedState = 0;
    }
}

void iResize(int w, int h)
{
    int w0, h0;
    w0 = width, h0 = height;
    width = w, height = h;
    N = width / dx + 2;
    originX += (w - w0) / 2.0;
    originY += (h - h0) / 2.0;
    panX = originX - w / 2.0;
    panY = originY - h / 2.0;
    exprSetScreenRes(w, h);
    resized = 1;
}

void toggleFullScreen()
{
    if (isFullScreen)
        glutReshapeWindow(1280, 720);
    else
        glutFullScreen();
    isFullScreen = !isFullScreen;
}

void iKeyboard(unsigned char key)
{
    if (glutGetModifiers() & GLUT_ACTIVE_ALT) {
        switch (toupper(key)) {
            case 'A':
                if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
                    // alt+shift+a
                    for (int i = 0; i < n_sines; i++)
                        A[i] *= 0.99;
                }
                else {
                    // alt+a
                    // alt+shift+a
                    for (int i = 0; i < n_sines; i++)
                        A[i] *= 1.01;
                }
                break;
            case 'F':
                if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
                    // alt+shift+f
                    for (int i = 0; i < n_sines; i++)
                        L[i] /= 0.99;
                }
                else {
                    // alt+f
                    for (int i = 0; i < n_sines; i++)
                        L[i] /= 1.01;
                }
                break;
            case 'P':
                if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
                    // alt+shift+p
                    for (int i = 0; i < n_sines; i++)
                        P[i] -= 10 * L[i] / 360;
                }
                else {
                    // alt+p
                    for (int i = 0; i < n_sines; i++)
                        P[i] += 10 * L[i] / 360;
                }
                break;
            case 'S': // alt + S
                if (drawSummation == 2)
                    drawSummation = 0;
                else
                    drawSummation = 2;
                break;
            case 'C':
                if (drawSummation == 2)
                    drawSummation = 1;
                else
                    drawCurves = !drawCurves;
                if (!drawCurves) deselectAll();
                break;
        }
    }
    else {
        switch (key) {
            case 'Z' - 'A' + 1: // ctrl + Z
                for (int i = 0; i < n_sines; i++)
                    A[i] = A0[i], L[i] = L0[i], P[i] = P0[i];
                break;
            case 'D' - 'A' + 1: // ctrl + G
                drawMode     = 1;
                drawingIndex = 0;
                deselectAll();
                break;
            case 'G' - 'A' + 1: // ctrl + G
                deselectAll();
                graphMode = !graphMode;
                break;
            case 'O' - 'A' + 1: // ctrl + 0
                backToOrigin();
                break;
            case 'S' - 'A' + 1: // ctrl + 0
                tracersSynced = !tracersSynced;
                break;
            case 'T' - 'A' +
                1: // ctrl + T
            {
                int i = 0;
                while (i <= n_sines && !(tracerState[i] & 1))
                    i++;
                if (i == n_sines + 1)
                    showAllTracers();
                else
                    hideAllTracers();
            } break;
            case 'P' - 'A' +
                1: // ctrl + T
            {
                int i = 0;
                while (i <= n_sines && (tracerState[i] & 2))
                    i++;
                if (i == n_sines + 1)
                    resumeAllTracers();
                else
                    pauseAllTracers();
            } break;
            case 127: // delete
                if (n_sines >= 0) removeSine();
                break;
            case 27: // escape
                if (graphMode) { graphMode = 0; }
                else if (isFullScreen) {
                    glutReshapeWindow(1280, 720);
                    isFullScreen = 0;
                }
                break;
            case 'f':
            case 'F': toggleFullScreen(); break;
            case 'z':
            case 'Z':
                if (glutGetModifiers() & GLUT_ACTIVE_SHIFT)
                    zoom(-1, -1, -1);
                else
                    zoom(1, -1, -1);
                break;
            case 'q': exit(0); break;
            default: {
                if (integerInput) {
                    if (!takeIntInput(key))
                        integerInput = 0;
                    else {
                        int n0 = n_sines;
                        int n  = atoi(numStr);
                        if (n > n0) {
                            for (int i = n0; i < n; i++)
                                addSine();
                        }
                        else {
                            for (int i = n0; i > n; i--)
                                removeSine();
                        }
                    }
                }
                else if (graphMode) {
                    takeMathInput(key);
                    return;
                }
                else {
                    switch (key) {
                        case '+': changeTracerSpeed(1); break;
                        case '-': changeTracerSpeed(-1); break;
                        case 'A' - 'A' + 1: { // ctrl + A
                            for (int i = 0; i < n_sines; i++)
                                selected[i] = 1;
                            n_selected = n_sines;
                        }
                    }
                }
            } break;
        }
    }
}

void iSpecialKeyboard(unsigned char key)
{
    switch (key) {
        case GLUT_KEY_F11: toggleFullScreen(); break;
        case GLUT_KEY_END: exit(0); break;
        case GLUT_KEY_INSERT: addSine(); break;
        case GLUT_KEY_RIGHT: pan(-10, 0); break;
        case GLUT_KEY_LEFT: pan(10, 0); break;
        case GLUT_KEY_UP: pan(0, -10); break;
        case GLUT_KEY_DOWN: pan(0, 10); break;
        case GLUT_KEY_PAGE_UP: zoom(1, -1, -1); break;
        case GLUT_KEY_PAGE_DOWN: zoom(-1, -1, -1); break;
        case GLUT_KEY_F1: showHelp = !showHelp; break;
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
    for (i = 0; i < 7; i++)
        addSine();
    exprSetInitBounds(-5.0, 5.0, -5.0, 5.0);
    iSetTransparency(1);
    iInitializeEx(width, height, "Curves");
    return 0;
}

double amplitude(int index) { return exprLength(A[index]); }
double frequency(int index) { return 1.0 / exprLength(L[index]); }
double phase(int index)
{
    double p = fmod(-360.0 * P[index] / L[index], 360.0);
    if (markedCosine[index]) p -= 90.0;
    if (p > 180.0)
        p -= 360.0;
    else if (p < -180.0)
        p += 360.0;
    if (fabs(p) < 0.001) p = 0;
    return p;
}
void equation(int index, char* str, int size)
{
    double      a = amplitude(index);
    double      w = 2 * PI * frequency(index);
    double      p = phase(index);
    const char* f = markedCosine[index] ? "cos" : "sin";
    if (fabs(p) < 0.001)
        snprintf(str, size, "y = %0.3lf %s(%0.3lfx)", a, f, w);
    else if (p > 0.0)
        snprintf(str, size, "y = %0.3lf %s(%0.3lfx + %0.3lf)", a, f, w, fabs(p));
    else
        snprintf(str, size, "y = %0.3lf %s(%0.3lfx - %0.3lf)", a, f, w, fabs(p));
}

void backToOrigin()
{
    panX = panY = 0;
    originX     = width / 2;
    originY     = height / 2;
}
void backToUnitScale()
{
    double scale0 = scale;
    double x, y;
    x       = originX;
    y       = originY;
    scale   = 1.0;
    panX    = originX - width / 2.0;
    panY    = originY - height / 2.0;
    originX = width / 2.0 + panX;
    originY = height / 2.0 + panY;
    exprScaleBounds(scale);
    exprPan(panX, panY);
    resized = 1;
}

void drawAxis()
{
    iSetColor(255, 255, 255);
    iLineEx(0, originY, width, 0, 3);
    iLineEx(originX, 0, 0, height, 3);
    iSetColor(0, 0, 0);
    iLineEx(0, originY, width, 0, 1);
    iLineEx(originX, 0, 0, height, 1);
}

void drawGrid()
{
    double l = exprLeft(), r = exprRight(), t = exprTop(), b = exprBottom();
    double R = min(r - l, t - b);
    int    k, m;
    k = m     = floor(log10(R));
    double du = pow(10, k);
    switch ((int)floor(R / du)) {
        case 1: du *= 0.2, m--; break;
        case 2: du *= 0.25, m -= 2; break;
        case 3:
        case 4:
        case 5: du *= 0.5, m--; break;
    }
    char format[16];
    char num[16];
    snprintf(format, 16, "%%0.%dlf", (int)fabs(min(0.0, m)));
    iSetColorEx(255, 255, 255, 0.25);
    double x = ceil(l / du) * du, sX;
    while (x < r) {
        sX = exprGetScreenX(x);
        snprintf(num, 16, format, x);
        iLine(sX, 0, sX, height);
        iText(sX + 10, originY + 10, num, GLUT_BITMAP_HELVETICA_12);
        x += du;
    }
    double y = ceil(b / du) * du, sY;
    while (y < t) {
        sY = exprGetScreenY(y);
        snprintf(num, 16, format, y);
        iLine(0, sY, width, sY);
        iText(originX + 10, sY + 10, num, GLUT_BITMAP_HELVETICA_12);
        y += du;
    }
}
void drawSines()
{
    static double pX[4], pY[4], qY[MAX_WIDTH], cY[MAX_WIDTH];
    int           crossesAxis;
    pY[2] = pY[3] = originY;
    double alpha  = (n_sines <= 15) ? .2 : .1;
    // draw trapezoids with the x-axis
    for (int j = 0; j <= n_sines; j++) {
        for (int i = 0; i < N; i++) {
            if (j == 0) cY[i] = 0;
            pX[2] = pX[1] = X[i];
            if (j < n_sines) {
                qY[i] = pY[1] = originY + A[j] * sin(2 * PI / L[j] * ((X[i] - originX) / scale - P[j])) * scale;
                cY[i] += qY[i] - originY;
                if (drawCurves) iSetColorEx(C[j][0], C[j][1], C[j][2], alpha);
            }
            else {
                qY[i] = pY[1] = cY[i] += originY;
                if (drawSummation) iSetColorEx(255, 255, 255, alpha / 2);
            }
            if (fabs(tracerX[j] - X[i]) < 0.5) tracerY[j] = qY[i];
            if ((j < n_sines && drawCurves) || (j == n_sines && drawSummation)) {
                if (i > 0) {
                    crossesAxis = (pY[0] >= originY && pY[1] < originY) || (pY[0] < originY && pY[1] >= originY);
                    if (crossesAxis) {
                        // draw two triangles instead of a quadrilateral
                        pX[3] = pX[1], pY[3] = pY[1], pX[2] = pX[0];
                        pX[1] = (originY - pY[0]) * (pX[1] - pX[0]) / (pY[1] - pY[0]) + pX[0], pY[1] = originY;
                        iFilledPolygon(pX, pY, 3);
                        pX[2] = pX[0] = pX[3], pY[0] = pY[3];
                        iFilledPolygon(pX, pY, 3);
                        pX[3] = pX[0], pY[3] = originY;
                        continue;
                    }
                    iFilledPolygon(pX, pY, 4);
                }
                pX[3] = pX[0] = pX[1], pY[0] = pY[1];
            }
        }
        if ((j < n_sines && drawCurves) || (j == n_sines && drawSummation)) {
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
                    stroke *= 1.5;
                    dashed = 0;
                }
            }
            iPath(X, qY, N, stroke, 0, dashed, 10, 5);
        }
    }
}
// return 1 if just a curve is selected/ctrl-selected
// retursn 2 if a curve is selected and other previously selected ones should be deselected on mouse up
// returns 0 if no curve is selected
int selectCurve(int x, int y)
{
    y -= (height / 2 + panY);
    double Y, C = 0, minY = INFINITY;
    int    sine_index = -1;
    for (int i = 0; i <= n_sines; i++) {
        if (i < n_sines) {
            Y = A[i] * sin(2 * PI / L[i] * ((x - originX) / scale - P[i])) * scale;
            C += Y;
        }
        else
            Y = C;
        if (fabs(Y) < minY) {
            if ((0 <= y && y <= Y + 2) || (0 >= y && y >= Y - 2)) {
                if (i < n_sines && !drawCurves) continue;
                if (i == n_sines && !drawSummation) continue;
                sine_index = i;
                minY       = fabs(Y);
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
    if (n_sines < MAX_SINES) {
        int i = n_sines++;
        iRandomColor(1, 1, C[i]);
        if (!graphMode) {
            A[i] = iRandom(50, height / 4);
            L[i] = iRandom(150, width / 2);
            P[i] = iRandom(0, width);
        }
        else
            fourier();
    }
}

void removeSine()
{
    if (n_sines > 0) {
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
}

void drawTextBox()
{
    double w = width * 0.25;
    iSetColorEx(45, 52, 54, 0.8);
    iFilledRectangle(width * 0.375, 50, width * 0.25, 40);
    iSetColor(255, 255, 255);
    iLineEx(width * 0.375, 50, width * 0.25, 0);
    if (strlen(expr) == 0) {
        iSetColorEx(255, 255, 255, 0.33);
        iText(width * 0.375 + (w - 190.0) / 2.0, 65, "Enter your equation", GLUT_BITMAP_TIMES_ROMAN_24);
    }
    else {
        iSetColor(255, 255, 255);
        double l = strlen(expr) * 10.0;
        iText(width * 0.375 + (w - l) / 2.0, 65, expr, GLUT_BITMAP_TIMES_ROMAN_24);
    }
}

int takeIntInput(unsigned char key)
{
    if (key == '\b') {
        if (numPos >= 0) numStr[--numPos] = '\0';
    }
    else {
        if (!isdigit(key) || numPos == 3) return 0;
        if (numPos == 1 && numStr[0] == '0')
            numStr[0] = key;
        else
            numStr[numPos++] = key;
    }
    return 1;
}
void takeMathInput(unsigned char key)
{
    if (isalnum(key))
        expr[exprPos++] = (char)key;
    else {
        switch (key) {
            case 22: // Ctrl + V
            {
#ifdef WIN32
                HWND hnd = GetActiveWindow();
                if (OpenClipboard(hnd)) {
                    if (IsClipboardFormatAvailable(CF_TEXT)) {
                        const char* text = (const char*)GetClipboardData(CF_TEXT);
                        for (int i = 0; text[i]; i++)
                            if (text[i] != ' ') expr[exprPos++] = text[i];
                    }
                    CloseClipboard();
                }
#endif
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

void drawFunction(double X[], double Y[], int n)
{
    for (int i = 0; i < n; i++) {
        int k = (int)max(round(X[i]), 0.0);
        fY[k] = max(Y[i], fY[k]);
    }
    iSetColor(45, 52, 54);
    iPath(X, Y, n, 3);
}

void drawHandDrawnCurve()
{
    iSetColorEx(255, 255, 255, 1.0);
    iPath(drawingX, drawingY, drawingIndex, 3, 0, 1, 10, 5);
}

void zoom(int dir, int x, int y)
{
    double scale0 = scale;
    if (x < 0) x = originX;
    if (y < 0) y = originY;
    scale += dir * scale * 0.05;
    panX    = x - scale / scale0 * (x - originX) - width / 2.0;
    panY    = y - scale / scale0 * (y - originY) - height / 2.0;
    originX = width / 2.0 + panX;
    originY = height / 2.0 + panY;
    exprScaleBounds(scale);
    exprPan(panX, panY);
    resized = 1;
}
void pan(double dx, double dy)
{
    panX += dx, panY += dy;
    originX = width / 2.0 + panX;
    originY = height / 2.0 + panY;
    exprPan(panX, panY);
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
        y1 = fY[lt] - originY, y2 = fY[t] - originY;
        dx = x2 - x1;
        for (int j = 1; j <= n_sines; j++) {
            // trapezoid approximation
            a[j - 1] += 2 / period * (y1 * cos(j * w * x1) + y2 * cos(j * w * x2)) * dx / 2.0;
            b[j - 1] += 2 / period * (y1 * sin(j * w * x1) + y2 * sin(j * w * x2)) * dx / 2.0;
        }
        lt = t++;
        while (!isfinite(fY[t]))
            t++;
    }
    for (int i = 0; i < n_sines; i++) {
        A[i] = sqrt(a[i] * a[i] + b[i] * b[i]) / scale;
        L[i] = period / (i + 1) / scale;
        P[i] = (-atan2(a[i], b[i])) * L[i] / (2 * PI) + (l - originX) / scale;
    }
}

// the scale and delta factors for the overlays
double supA, supF, supP;
int    sinI;

void drawBasicOverlay(int w, int h)
{
    if (newOverlay) {
        supA = supF = 1, supP = 0;
        if (overlayState == SIN_OVERLAY) {
            sinI = 0;
            while (!selected[sinI])
                sinI++;
        }
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
    iSetColorEx(45, 52, 54, 0.95);
    iFilledRectangle(overLayLeft, overlayBottom, w, h);
}

void drawMenu(const char* text, int w, int h, int dx, int dy, int filled, int aligned)
{
    if (filled) {
        if (overLayLeft + dx <= mX && mX <= overLayLeft + dx + w && overlayTop - dy <= mY &&
            mY <= overlayTop - dy + h) {
            iSetColorEx(255, 255, 255, 0.1);
            iFilledRectangle(overLayLeft + dx, overlayTop - dy, w, h);
            iSetColorEx(255, 255, 255, 1.0);
            iRectangleEx(overLayLeft + dx, overlayTop - dy, w, h, 0.5, 1, 10, 5);
        }
    }
    iSetColorEx(255, 255, 255, 1);
    if (aligned)
        dx += (w - strlen(text) * 6) / 2.0;
    else
        dx += 10;
    iText(overLayLeft + dx, overlayTop - dy + 10, text, GLUT_BITMAP_HELVETICA_12);
}

void drawScaleMenu(const char* text, double value, const char* format, int w, int h, int dx, int dy)
{
    char str[8];
    drawMenu(text, w, h, dx, dy, 1, 0);
    snprintf(str, 8, format, value);
    int l = strlen(str) * 6;
    drawMenu(str, 30, h, dx + w - 45 - l, dy, 0, 0);
    drawMenu("-", 30, h, dx + w - 100, dy, 1, 1);
    drawMenu("+", 30, h, dx + w - 30, dy, 1, 1);
}

void drawSlideMenu(const char* title, double value, double min, double max, int w, int h, int dx, int dy)
{
    if (overLayLeft + dx <= mX && mX <= overLayLeft + dx + w && overlayTop - dy <= mY && mY <= overlayTop - dy + h) {
        iSetColorEx(255, 255, 255, 0.1);
        iFilledRectangle(overLayLeft + dx, overlayTop - dy, w, h);
        iSetColorEx(255, 255, 255, 1.0);
        iRectangleEx(overLayLeft + dx, overlayTop - dy, w, h, 0.5, 1, 10, 5);
    }
    char str[8];
    iSetColor(255, 255, 255);
    iText(overLayLeft + dx + 10, overlayTop - dy + h - 18, title, GLUT_BITMAP_HELVETICA_12);
    snprintf(str, 8, "%0.3f", value);
    iText(overLayLeft + dx + w - strlen(str) * 6 - 10, overlayTop - dy + h - 18, str, GLUT_BITMAP_HELVETICA_12);
    iSetColorEx(255, 255, 255, 0.2);
    iFilledRectangle(overLayLeft + dx + 5, overlayTop - dy + 8, w - 10, 5);
    min      = min(value, min);
    max      = max(value, max);
    double p = (value - min) / (max - min) * (w - 10) + overLayLeft + dx + 5;
    iSetColorEx(255, 255, 255, 0.67);
    iFilledRectangle(p - 2, overlayTop - dy + 3, 4, 15);
}

void drawColorPicker(int w, int h, int dx, int dy, int outlined)
{
    double rgb[3];
    int    mouseInside =
        overLayLeft + dx <= mX && mX <= overLayLeft + dx + w && overlayTop - dy <= mY && mY <= overlayTop - dy + h;
    double hsv[3];
    iRGBtoHSV(C[sinI][0], C[sinI][1], C[sinI][2], hsv);
    double H0     = hsv[0];
    int    marked = 0;
    for (int i = 0; i < w; i++) {
        double H = 360 * i / w;
        iHSVtoRGB(H, 1.0, 1.0, rgb);
        iSetColorEx(rgb[0], rgb[1], rgb[2], 0.5 + 0.5 * mouseInside);
        iLine(overLayLeft + i + dx, overlayTop - dy, overLayLeft + i + dx, overlayTop - dy + h);
    }
    // if (H0 < 0) printf("Fuck\n");
    iSetColorEx(C[sinI][0], C[sinI][1], C[sinI][2], 1);
    iFilledRectangle(overLayLeft + w * H0 / 360 + dx - 2.5, overlayTop - dy - 2, 5, h + 4);
    iSetColorEx(255, 255, 255, 0.85);
    iRectangleEx(overLayLeft + w * H0 / 360 + dx - 2.5, overlayTop - dy - 2, 5, h + 4, 1);
    if (outlined) {
        iSetColorEx(255, 255, 255, 1);
        iLine(overLayLeft + dx, overlayTop - dy, overLayLeft + dx + w, overlayTop - dy);
    }
}
void deselectAll()
{
    // deselect all since empty space was clicked
    for (int j = 0; j < n_sines; j++)
        selected[j] = 0;
    n_selected = 0;
}
void drawSupOverLay()
{
    static int w = 230, h = 270;
    drawBasicOverlay(w, h);
    drawMenu("Approximate graph of equation", w, 30, 0, 30, 1, 0);
    drawMenu("Approximate hand-drawn curve", w, 30, 0, 60, 1, 0);
    drawScaleMenu("Scale Amplitude", supA, "%0.3lf", w, 30, 0, 90);
    drawScaleMenu("Scale Frequencies", supF, "%0.3lf", w, 30, 0, 120);
    drawScaleMenu("Change Phase", supP, "%0.1lf", w, 30, 0, 150);
    if (tracerState[n_sines] & 2)
        drawMenu("Resume Tracer", w, 30, 0, 180, 1, 0);
    else
        drawMenu("Pause Tracer", w, 30, 0, 180, 1, 0);
    if (tracerState[n_sines] & 1)
        drawMenu("Hide Tracer", w, 30, 0, 210, 1, 0);
    else
        drawMenu("Show Tracer", w, 30, 0, 210, 1, 0);
    if (drawCurves)
        drawMenu("Hide Composing Sinusoids", w, 30, 0, 240, 1, 0);
    else
        drawMenu("Show Composing Sinusoids", w, 30, 0, 240, 1, 0);
    drawMenu("Hide", w, 30, 0, 270, 1, 0);
}

void handleSupOverlay(int dragging)
{
    static int w = 230, h = 240;
    int        dx = X0 - overLayLeft, dy = overlayTop - Y0;
    if (60 <= dy && dy <= 90) {
        // scale amp
        if (dx >= w - 100 && dx <= w - 70) {
            if (supA > 0.01) supA -= 0.01;
        }
        else if (dx >= w - 30)
            supA += 0.01;
        for (int i = 0; i < n_sines; i++)
            A[i] = A0[i] * supA;
    }
    else if (90 <= dy && dy <= 120) {
        // scale freq
        if (dx >= w - 100 && dx <= w - 70)
            supF -= 0.005;
        else if (dx >= w - 30)
            supF += 0.005;
        for (int i = 0; i < n_sines; i++) {
            L[i] = L0[i] / supF;
            P[i] = P0[i] / supF + L0[i] / 360.0 * supP / supF; // common eqn
        }
    }
    else if (120 <= dy && dy <= 150) {
        // change phase
        if (dx >= w - 100 && dx <= w - 70)
            supP -= 2.5;
        else if (dx >= w - 30)
            supP += 2.5;
        fmod(supP, 360.0);
        if (supP > 180.0) supP -= 360.0;
        if (supP < -180.0) supP += 360.0;
        for (int i = 0; i < n_sines; i++)
            P[i] = P0[i] / supF + L0[i] / 360.0 * supP / supF; // common eqn
    }
    else {
        if (dragging) return;
        if (0 <= dy && dy <= 30) {
            // graph
            graphMode = 1;
        }
        else if (dy <= 60) {
            // hand-draw
            drawMode     = 1;
            drawingIndex = 0;
            deselectAll();
        }
        else if (dy <= 180) {
            // resume/pause tracer
            if (tracerState[n_sines] & 2)
                resumeTracer(n_sines);
            else
                pauseTracer(n_sines);
        }
        else if (dy <= 210) {
            // show/hide tracer
            if (tracerState[n_sines] & 1)
                hideTracer(n_sines);
            else
                showTracer(n_sines);
        }
        else if (dy <= 240) {
            // show/hide curves
            drawCurves = !drawCurves;
        }
        else if (dy <= 270) {
            // hide summation
            drawSummation = 0;
        }
        overlayState = 0;
    }
}
void drawSinOverLay()
{
    static int w = 230, h = 315;
    drawBasicOverlay(w, h);
    if (markedCosine[sinI])
        drawMenu("Cosine", w, 30, 0, 30, 1, 0);
    else
        drawMenu("Sine", w, 30, 0, 30, 1, 0);
    double a = amplitude(sinI);
    double f = fabs(frequency(sinI));
    double p = phase(sinI);
    drawSlideMenu("Amplitude", a, 0, exprLength(height / 2.0 / scale), w, 45, 0, 75);
    drawSlideMenu("Frequency", f, 0.01, 1.0 / exprLength(25.0), w, 45, 0, 120);
    drawSlideMenu("Phase", p, -180, 180, w, 45, 0, 165);
    drawMenu("Change Color", w, 30, 0, 195, 1, 0);
    drawColorPicker(w, 20, 0, 225, 0);
    if (tracerState[sinI] & 2)
        drawMenu("Resume Tracer", w, 30, 0, 255, 1, 0);
    else
        drawMenu("Pause Tracer", w, 30, 0, 255, 1, 0);
    if (tracerState[sinI] & 1)
        drawMenu("Hide Tracer", w, 30, 0, 285, 1, 0);
    else
        drawMenu("Show Tracer", w, 30, 0, 285, 1, 0);
    drawMenu("Remove Curve", w, 30, 0, 315, 1, 0);
}
void handleSinOverlay(int dragging)
{
    int        dx = X0 - overLayLeft, dy = overlayTop - Y0;
    static int dragSelection = 0;
    static int w = 230, h = 315;
    if (30 <= dy && dy <= 75) {
        // amplitude
        if (!dragging || dragSelection == 1) {
            dragSelection = 1;
            if (5 <= dx && dx <= w - 5 && (dragging || dy >= 55))
                A[sinI] = max(height / 2.0 / scale, A[sinI]) / (w - 10) * (dx - 5);
        }
    }
    else if (75 <= dy && dy <= 120) {
        // frequency
        if (!dragging || dragSelection == 2) {
            dragSelection = 2;
            if (5 <= dx && dx <= w - 5 && (dragging || dy >= 100)) {
                double s  = L[sinI] >= 0 ? 1 : -1;
                double f0 = fabs(frequency(sinI));
                double L0 = L[sinI];
                L[sinI] =
                    s * exprScreenLength(
                            1.0 / ((max(f0, 1.0 / exprLength(25.0)) - min(f0, 0.01)) / (w - 10) * (dx - 5) + 0.01));
                P[sinI] *= L[sinI] / L0;
            }
        }
    }
    else if (120 <= dy && dy <= 165) {
        // phase
        if (!dragging || dragSelection == 4) {
            dragSelection = 4;
            if (5 <= dx && dx <= w - 5 && (dragging || dy >= 55)) {
                double deg = -360.0 * (dx - 5) / (w - 10) + 180.0 - 90.0 * markedCosine[sinI];
                if (fabs(fmod(deg, 30.0)) < 5.0) deg = round(deg / 30.0) * 30.0;
                if (fabs(fmod(deg, 45.0)) < 5.0) deg = round(deg / 45.0) * 45.0;
                P[sinI] = deg / 360.0 * L[sinI];
            }
        }
    }
    else if (205 <= dy && dy <= 225) {
        if (!dragging || dragSelection == 3) {
            dragSelection = 3;
            double H      = 360.0 * dx / w;
            iHSVtoRGB(H, 1.0, 1.0, C[sinI]);
        }
    }
    else {
        if (dragging) return;
        if (dy <= 30) {
            // sine/cosine
            if (markedCosine[sinI])
                P[sinI] += L[sinI] / 4.0;
            else
                P[sinI] -= L[sinI] / 4.0;
            markedCosine[sinI] = !markedCosine[sinI];
            return;
        }
        else if (dy <= 195) {
            iRandomColor(1.0, 1.0, C[sinI]);
            return;
        }
        else if (dy <= 255) {
            // pause/resume tracer
            if (tracerState[sinI] & 2)
                resumeTracer(sinI);
            else
                pauseTracer(sinI);
        }
        else if (dy <= 285) {
            // change tracer speed
            if (tracerState[sinI] & 1)
                hideTracer(sinI);
            else
                showTracer(sinI);
        }
        else if (dy <= 315) {
            // resume tracer
            removeSine();
        }
        overlayState = 0;
    }
}
void drawGenOverLay()
{
    static int w = 220, h = 420;
    drawBasicOverlay(w, h);
    drawMenu("Add Curve", w, 30, 0, 30, 1, 0);
    drawMenu("Remove Curve", w, 30, 0, 60, 1, 0);
    if (drawCurves)
        drawMenu("Hide Curves", w, 30, 0, 90, 1, 0);
    else
        drawMenu("Show Curves", w, 30, 0, 90, 1, 0);
    if (drawSummation)
        drawMenu("Hide Summation", w, 30, 0, 120, 1, 0);
    else
        drawMenu("Show Summation", w, 30, 0, 120, 1, 0);
    drawMenu("Resume All Tracers", w, 30, 0, 150, 1, 0);
    drawMenu("Pause All Tracers", w, 30, 0, 180, 1, 0);
    drawMenu("Show All Tracers", w, 30, 0, 210, 1, 0);
    drawMenu("Hide All Tracers", w, 30, 0, 240, 1, 0);
    if (tracersSynced)
        drawMenu("Desync Tracers", w, 30, 0, 270, 1, 0);
    else
        drawMenu("Sync Tracers", w, 30, 0, 270, 1, 0);
    if (tracersUnidirectional)
        drawMenu("Tracer Motion >>", w, 30, 0, 300, 1, 0);
    else
        drawMenu("Tracer Motion <>", w, 30, 0, 300, 1, 0);
    drawScaleMenu("Tracer Speed", tracerSpeed, "%0.1lf", w, 30, 0, 330);
    drawMenu("Back to Origin", w, 30, 0, 360, 1, 0);
    if (showGrid)
        drawMenu("Hide Grid", w, 30, 0, 390, 1, 0);
    else
        drawMenu("Show Grid", w, 30, 0, 390, 1, 0);
    drawMenu("Show Help", w, 30, 0, 420, 1, 0);
}

void changeTracerSpeed(int dir)
{
    double t  = iGetTime();
    double v0 = tracerSpeed, v;
    v         = tracerSpeed += dir * 0.5;
    if (fabs(v) < 0.01) tracerSpeed = v = -v0;
    if (tracersSynced)
        tracerSyncdt = t - v0 / v * (t - tracerSyncdt);
    else {
        for (int i = 0; i <= n_sines; i++)
            tracerdt[i] = t - v0 / v * (t - tracerdt[i]);
    }
}

void hideAllTracers()
{
    for (int i = 0; i <= n_sines; i++)
        hideTracer(i);
    tracerButtonMode = 0;
}
void showAllTracers()
{
    for (int i = 0; i <= n_sines; i++)
        showTracer(i);
    if (tracersSynced)
        tracerButtonMode = 2;
    else
        tracerButtonMode = 1;
}
void resumeAllTracers()
{
    double t = iGetTime();
    for (int i = 0; i <= n_sines; i++) {
        tracerdt[i] += t - tracerPt[i];
        tracerState[i] &= ~2;
    }
}
void pauseAllTracers()
{
    double t = iGetTime();
    for (int i = 0; i <= n_sines; i++) {
        tracerPt[i] = t;
        tracerState[i] |= 2;
    }
}

void handleGenOverlay(int dragging)
{
    static int w = 220, h = 420;
    int        dx = X0 - overLayLeft, dy = overlayTop - Y0;
    if (300 <= dy && dy <= 330) {
        // tracer speed
        if (dx >= w - 100 && dx <= w - 70)
            changeTracerSpeed(-1);
        else if (dx >= w - 30)
            changeTracerSpeed(1);
    }
    else {
        if (dragging) return;
        if (0 <= dy && dy <= 30) {
            // Add curve
            addSine();
        }
        else if (dy <= 60) {
            // remove curve
            removeSine();
        }
        else if (dy <= 90) {
            // show/hide curves
            drawCurves = !drawCurves;
        }
        else if (dy <= 120) {
            // Hide/show superposition
            drawSummation = !drawSummation;
        }
        else if (dy <= 150) {
            // resume tracers
            resumeAllTracers();
        }
        else if (dy <= 180) {
            // pause tracers
            pauseAllTracers();
        }
        else if (dy <= 210) {
            // show tracers
            showAllTracers();
        }
        else if (dy <= 240) {
            // hide tracers
            hideAllTracers();
        }
        else if (dy <= 270) {
            // sync/desync
            tracersSynced = !tracersSynced;
        }
        else if (dy <= 300) {
            // motion direction
            tracersUnidirectional = !tracersUnidirectional;
        }
        else if (dy <= 360) {
            // back to origin
            backToOrigin();
        }
        else if (dy <= 390) {
            // show/hide grid
            showGrid = !showGrid;
        }
        else if (dy <= 420) {
            // show help
            showHelp = 1;
        }
        overlayState = 0;
    }
}
void locateTracers()
{
    double t = iGetTime();
    double x = t * tracerSpeed - tracerSyncdt;
    for (int i = 0; i <= n_sines; i++) {
        if ((tracerState[i] & 1) && !(tracerState[i] & 2)) { // showed and resumed
            if (tracersSynced) {
                if (tracersUnidirectional) {
                    x          = fmod(x, width);
                    tracerX[i] = fmod(x + width, width);
                }
                else {
                    x          = fmod(x, 2 * width);
                    tracerX[i] = fabs(fmod(x + 2 * width, 2 * width) - width);
                }
            }
            else {
                x = (t - tracerdt[i]) * tracerSpeed;
                if (tracersUnidirectional) {
                    x = fmod(x, width);
                    if (i < n_sines)
                        tracerX[i] = fmod(x + P[i] + width + originX, width);
                    else
                        tracerX[n_sines] = fmod(x + width + originX, width);
                }
                else {
                    x = fmod(x, 2 * width);
                    if (i < n_sines)
                        tracerX[i] = fabs(fmod(x + P[i] + 2 * width + originX, 2 * width) - width);
                    else
                        tracerX[n_sines] = fabs(fmod(x + 2 * width + originX, 2 * width) - width);
                }
            }
        }
    }
}
void drawTracers()
{
    double r = 5;
    for (int i = 0; i <= n_sines; i++) {
        if (tracerState[i] & 1) {
            if (i < n_sines) {
                iSetColor(C[i][0], C[i][1], C[i][2]);
                iFilledCircle(tracerX[i], tracerY[i], r);
            }
            else {
                iSetColorEx(255, 255, 255, 0.85);
                iFilledCircle(tracerX[n_sines], tracerY[n_sines], r + 1);
                iSetColorEx(255, 255, 255, 1);
                iCircleEx(tracerX[n_sines], tracerY[n_sines], r + 3, 2, 1, 50, 5, 1);
            }
        }
    }
}

void pauseTracer(int i)
{

    tracerPt[i] = iGetTime();
    tracerState[i] |= 2;
}
void resumeTracer(int i)
{

    tracerdt[i] += iGetTime() - tracerPt[i];
    tracerState[i] &= ~2;
}
void showTracer(int i) { tracerState[i] |= 1; }
void hideTracer(int i) { tracerState[i] &= ~1; }

// 1 highlight, 2 to always highlight
void drawBottomMenu(const char* text, int w, int dx, int highlight, int large)
{
    if (dx < 0) dx += width;
    iSetColor(255, 255, 255);
    if (strlen(text) > 0)
        iText(dx + 5, 4 + 3 * (!large), text, large ? GLUT_BITMAP_TIMES_ROMAN_24 : GLUT_BITMAP_HELVETICA_12);
    if (highlight) {
        if (highlight == 2 || mY <= 22 && mX >= dx && mX <= dx + w) {
            iSetColorEx(255, 255, 255, 0.1);
            iFilledRectangle(dx, 0, w, 22);
            iSetColor(255, 255, 255);
            iRectangleEx(dx, 0, w, 22, 1, 10, 5);
        }
    }
}

void drawBottomScaleMenu(const char* text1, const char* text2, int w1, int w2, int dx, int h1, int h2, int highlight)
{
    drawBottomMenu("-", 25, dx, 1, 1);
    drawBottomMenu(text1, w1, dx + 30, h1, 0);
    drawBottomMenu(text2, w2, dx + w1 + 30, h2, 0);
    drawBottomMenu("+", 25, dx + w1 + w2 + 30, 1, 1);
}

void drawMiniGrid(int w, int h, int dx, int dy)
{
    if (dx < 0) dx += width;
    iSetColor(255, 255, 255);
    iLineEx(dx, dy + h / 3.0, w, 0);
    iLineEx(dx, dy + 2 * h / 3.0, w, 0);
    iLineEx(dx + w / 3.0, dy, 0, h);
    iLineEx(dx + 2 * w / 3.0, dy, 0, h);
}

void drawMiniTracers(int w, int h, int dx, int dy)
{
    if (dx < 0) dx += width;
    iSetColor(255, 255, 255);
    for (int i = 0; i < 3; i++)
        iFilledCircle(dx + i * w / 2.0, dy + i * h / 2, 2.5, 25);
}
void drawMiniSyncTracers(int w, int h, int dx, int dy)
{
    if (dx < 0) dx += width;
    iSetColor(255, 255, 255);
    for (int i = 0; i < 3; i++)
        iFilledCircle(dx + w / 2.0, dy + i * h / 2.0, 2.5, 25);
}

int eqnLen, cntrLen;

// 0 for all tracers hidden
// 1 for all tracers showed and not synced
// 2 for all tracers showed and synced

void drawBottomOverlay()
{
    char text[64];
    int  w_;
    iSetColorEx(45, 52, 54, 0.95);
    iFilledRectangle(0, 0, width, 22);
    snprintf(text, 64, "%3d", n_sines);
    drawBottomScaleMenu(text, " sinusoids", 30, 65, -165, 1 + integerInput, 0, 1);
    snprintf(text, 64, "Tracer Speed %0.1lf", tracerSpeed);
    drawBottomScaleMenu(text, tracersUnidirectional ? ">>" : "<>", 125, 25, -385, 0, 1, 1);
    if (tracerButtonMode) {
        iSetColorEx(255, 255, 255, .2);
        iFilledRectangle(width - 415, 0, 25, 22);
    }
    if (tracersSynced)
        drawMiniSyncTracers(14, 10, -410, 6);
    else
        drawMiniTracers(14, 10, -410, 6);
    drawBottomMenu("", 25, -415, 1, 1);
    if (showGrid) {
        iSetColorEx(255, 255, 255, .2);
        iFilledRectangle(width - 455, 0, 25, 22);
    }
    drawMiniGrid(16, 16, -450, 3);
    drawBottomMenu("", 25, -455, 1, 1);
    snprintf(text, 64, " Scale: %0.3lf", scale);
    drawBottomMenu(text, 85, -545, 1, 0);
    snprintf(
        text, 64, " Center: (%0.2lf, %0.2lf)", exprLength(width / 2.0 - originX), exprLength(height / 2.0 - originY));
    w_ = cntrLen = strlen(text) * 6 + 2;
    drawBottomMenu(text, w_, -(545 + w_), 1, 0);
    if (n_selected == 1) {
        int i = 0;
        while (!selected[i])
            i++;
        equation(i, text, 64);
        w_ = eqnLen = strlen(text) * 6 + 2;
        drawBottomMenu(text, w_, 50, 1, 0);
    }
    else if (iGetTime() < 20.0)
        drawBottomMenu("Press F1 for Help", 136, 50, 0, 0);

    iSetColorEx(0, 0, 0, 0.2);
    iFilledRectangle(0, 0, 40, 22);
    iSetColor(255, 255, 255);
    if (drawMode)
        iText(5, 8, "_--", GLUT_BITMAP_HELVETICA_18);
    else if (graphMode)
        iText(5, 8, "f(x,y)", GLUT_BITMAP_HELVETICA_12);
    else
        iText(5, 5, " ~ ", GLUT_BITMAP_TIMES_ROMAN_24);
}
void handleBottomOverlay(int dragging)
{
    int x = X0, xn = width - X0;
    if (360 <= xn && xn <= 385) {
        // - tracer speed
        changeTracerSpeed(-1);
    }
    else if (185 <= xn && xn <= 210) {
        // + tracer speed
        changeTracerSpeed(1);
    }
    else {
        if (dragging) return;
        if (50 <= x && x <= 50 + eqnLen) {
            // eqn
            X0 = 50, Y0 = 22;
            overlayState = SIN_OVERLAY;
            newOverlay   = 1;
        }
        else if (545 <= xn && xn <= 545 + cntrLen) {
            // center
            backToOrigin();
        }
        else if (460 <= xn && xn <= 545) {
            // scale
            backToUnitScale();
        }
        else if (390 <= xn && xn <= 415) {
            // show/hide grid
            tracerButtonMode = (tracerButtonMode + 1) % 3;
            switch (tracerButtonMode) {
                case 0: hideAllTracers(); break;
                case 1:
                    tracersSynced = 0;
                    showAllTracers();
                    break;
                case 2: tracersSynced = 1; break;
            }
        }
        else if (430 <= xn && xn <= 455) {
            // show/hide grid
            showGrid = !showGrid;
        }
        else if (210 <= xn && xn <= 235) {
            // sync/desync tracers
            tracersUnidirectional = !tracersUnidirectional;
        }
        else if (135 <= xn && xn <= 165) {
            // - n_sines
            removeSine();
        }
        else if (105 <= xn && xn <= 135) {
            // edit n_sines
            integerInput = 1;
        }
        else if (10 <= xn && xn <= 40) {
            // + n_sinesif (n_sines < MAX_SINES) {
            int i = n_sines;
            deselectAll();
            addSine();
            n_selected = selected[i] = 1;
            X0 = width, Y0 = 22;
            overlayState = SIN_OVERLAY;
            newOverlay   = 1;
        }
    }
}

int inOverlay(int x, int y) { return overLayLeft <= x && x <= overlayRight && overlayBottom <= y && y <= overlayTop; }

const char helpStrings[15][128] = {
    "Drag mouse on blank area to pan. Scroll (mouse middle button) to zoom in/out.",
    "Click to select a curve and drag to adjust the shape. Scroll to change phase.",
    "You can also Ctrl select multiple curves and drag adjust all of them.",
    "Selecting the summation curve selects and adjusts all the composing sinusoids.",
    "Right clicking anywhere gives you context menus for further customization options.",
    "Tracers can be adjusted via the general context menu (right clicking on empty area).",
    "You can reshape the summation to the graph of an equation or a hand-drawn curve.",
    "Ctrl + G switches you to Graph Mode.",
    "Ctrl + D switches you to Drawing Mode.",
    "You can also access these modes via the summation curve context menu.",
    "The bottom bar gives you information about the scale, translation of the graph",
    "as well as the tracer speed, the number of curves and their equations.",
    "Press the +/- button in the bottomleft corner to add/remove curves.",
    "Or you can enter any number (upto 256) to randomly generate that number of curves.",
    "Enjoy playing with the sinusoids!"};

const char shortcuts[26][2][128] = {
    {"F F11", "Fullscreen"},
    {"Insert", "Add curve"},
    {"Delete", "Remove Curve"},
    {"Ctrl + A", "Select All Curves"},
    {"Alt + A", "Increase Amplitude"},
    {"Alt + Shift + A", "Decrease Amplitude"},
    {"Alt + F", "Increase Frequency"},
    {"Alt + Shift + F", "Decrease Frequency"},
    {"Alt + P", "Add Phase"},
    {"Alt + Shift + P", "Subtract Phase"},
    {"Alt + C", "Show/Hide All Curves"},
    {"Alt + S", "Show/Hide Summation Only"},
    {"F1", "Show Help"},
    {"Ctrl + Z", "Undo Last Adjustment"},
    {"+", "Increase Tracer Speed"},
    {"-", "Decrease Tracer Speed"},
    {"Ctrl + S", "Sync/Desync Tracers"},
    {"Ctrl + P", "Pause/Resume Tracers"},
    {"Ctrl + T", "Show/Hide Tracers"},
    {"Up/Downt", "Pan"},
    {"Right/Left", ""},
    {"Page Up/Down", "Zoom In/Out"},
    {"Ctrl + G", "Graph Mode"},
    {"Ctrl + D", "Drawing Mode"},
    {"Ctrl + O", "Back to Origin"},
    {"Q End", "Exit"},
};

const char aboutString[][128] = {"A simple application written in C based the OpenGL-based iGraphics library,",
                                 "to explore and play with sines and cosines.",
                                 "Ashrafur Rahman | October 2020"};

void drawHelpScreen()
{
    iSetColorEx(45, 52, 54, 0.85);
    iFilledRectangle(width - 500, 0, 500, height);
    iSetColorEx(255, 255, 255, 0.1);
    iFilledRectangle(width - 25, height - 25, 25, 25);
    iSetColor(255, 255, 255);
    iText(width - 15, height - 15, "x", GLUT_BITMAP_HELVETICA_12);
    iText(width - 485, height - 30, "Help", GLUT_BITMAP_HELVETICA_18);
    iLine(width - 490, height - 40, width - 15, height - 40);
    for (int i = 0; i < 15; i++) {
        iText(width - 485, height - i * 20 - 60 - 5 * (i == 14), helpStrings[i], GLUT_BITMAP_HELVETICA_12);
    }
    iText(width - 485, height - 380, "Shortcuts", GLUT_BITMAP_HELVETICA_18);
    iLine(width - 490, height - 390, width - 15, height - 390);
    for (int i = 0; i < 26; i++) {
        iText(width - 485 + 250 * (i >= 13), height - (i % 13) * 15 - 410, shortcuts[i][0], GLUT_BITMAP_HELVETICA_12);
        iText(width - 395 + 250 * (i >= 13), height - (i % 13) * 15 - 410, shortcuts[i][1], GLUT_BITMAP_HELVETICA_12);
    }
    iText(width - 485, 65, "About", GLUT_BITMAP_HELVETICA_12);
    iLine(width - 490, 60, width - 115, 60);
    for (int i = 0; i < 3; i++)
        iText(width - 485, 45 - i * 14, aboutString[i], GLUT_BITMAP_HELVETICA_10);
}
