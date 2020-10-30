#include "../ext.h"
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
int       newOverlay = 0;
int       drawCurves = 1;
int       resized    = 0;

int drawMode = 0, drawing = 0;
// coordinates from the user or from the grapher for fourier estimation
double fY[MAX_WIDTH + 10];
double drawingX[MAX_WIDTH + 10], drawingY[MAX_WIDTH + 10];
int    drawingIndex;
// for taking mathematical expressions
char   expr[256]     = "";
char   exprPos       = 0;
int    takeExprInput = 0;
double originX = width / 2.0, originY = height / 2.0;
double panX = 0, panY = 0.0;
double panX0 = 0, panY0 = 0.0;
int    panning = 0;
double scale   = 1.0;

double tracerX[MAX_SINES + 5], tracerY[MAX_SINES + 5];
int    tracerState[MAX_SINES + 5] = {0}; // first bit set if tracer showed, second bit set if tracer paused
double tracerPt[MAX_SINES + 5] = {0}, tracerdt[MAX_SINES + 5] = {0};
double tracerSyncPt, tracerSyncdt                             = 0.0;
int    tracersSynced = 0, tracersUnidirectional = 1;
double tracerSpeed = 150.0; // pixel/second

double markedCosine[MAX_SINES + 5] = {0};

int showHelp = 1;

void zoom(int dir, int x, int y);
void pan(double dx, double dy);
void drawSines();
void drawAxis();
void locateTracers();
void drawTracers();
void drawBottomOverlay();
void handleBottomOverlay();
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
void takeMathInput(unsigned char key);
void fourier();

int inOverlay(int x, int y);

void iDraw()
{
    iClear();
    drawAxis();
    locateTracers();
    drawSines();
    drawTracers();
    switch (overlayState) {
        case SUP_OVERLAY: drawSupOverLay(); break;
        case SIN_OVERLAY: drawSinOverLay(); break;
        case GEN_OVERLAY: drawGenOverLay(); break;
    }
    if (takeExprInput) {
        drawTextBox();
        for (int i = 0; i < MAX_WIDTH; i++)
            fY[i] = -INFINITY;
        if (exprPlot(expr, drawFunction) && exprUpdated() || resized) fourier();
    }
    if (drawing) drawHandDrawnCurve();
    drawBottomOverlay();
    if (resized) resized = 0;
    if (showHelp) drawHelpScreen();
}

void iPassiveMouseMove(int x, int y) { mX = x, mY = y; }

void iMouseMove(int x, int y)
{
    if (overlayState && inOverlay(x, y)) {
        X0 = x, Y0 = y;
        switch (overlayState) {
            case SUP_OVERLAY: handleSupOverlay(1); break;
            case SIN_OVERLAY: handleSinOverlay(1); break;
            case GEN_OVERLAY: handleGenOverlay(1); break;
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
        panX = panX0 + x - X0, panY = panY0 + y - Y0;
        originX = width / 2.0 + panX;
        originY = height / 2.0 + panY;
        exprPan(panX, panY);
    }
    else if (clickedState && n_selected > 0) {
        int one = n_selected == 1;
        for (int i = 0; i < n_sines; i++) {
            if (selected[i]) {
                A[i] = (y - originY) / (Y0 - originY) * A0[i];
                if (one) {
                    if (fabs(PX[i] - X0) < 5) continue;
                    L[i] = L0[i] * (x - PX[i]) / (X0 - PX[i]);
                }
                else {
                    double x0 = X0;
                    if (fabs(x0 - originX) < 10) x0 += 10;
                    L[i] = L0[i] * (x - originX) / (x0 - originX);
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
                    // drawDir = 0;
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
                    A0[i] = A[i], L0[i] = L[i], P0[i] = P[i]; // (2m + 1)pi/2 = 2pi/L((x-X)/z - P)
                    PX[i] =
                        originX + scale * (L[i] / 4 * (2 * round(2 / L[i] * ((x - originX) / scale - P[i]) - 0.5) + 1) +
                                           P[i]); // take the x coord of the nearest peak
                }
            }
            else {
                // deselect all since empty space was clicked
                deselectAll();
                panning = 1;
                panX0 = panX, panY0 = panY;
                originX = width / 2.0 + panX;
                originY = height / 2.0 + panY;
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
                    if (scale < 20.0) zoom(1, x, y);
                }
                else {
                    if (scale > 0.01) zoom(-1, x, y);
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
                            drawingY[i - 1];
            }
            fourier();
        }
        if (panning) panning = 0;
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
                if (selected[j] && L[j] < 0) P[j] += L[j] / 2, L[j] = -L[j];
            }
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
    static int isFullScreen = 0;
    switch (key) {
        case 'Z' - 'A' + 1: // ctrl + Z
            for (int i = 0; i < n_sines; i++)
                A[i] = A0[i], L[i] = L0[i], P[i] = P0[i];
            break;
        case 'G' - 'A' + 1: // ctrl + G
            deselectAll();
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
        case 'z':
        case 'Z':
            if (glutGetModifiers() & GLUT_ACTIVE_SHIFT)
                zoom(-1, -1, -1);
            else
                zoom(1, -1, -1);
            break;
        case 'q': exit(0); break;
    }
    if (takeExprInput) {
        takeMathInput(key);
        return;
    }
    switch (key) {
        case '+':
            if (n_sines < MAX_SINES) addSine();
            break;
        case '-':
        case (unsigned char)127: // delete
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
        case GLUT_KEY_RIGHT: pan(-10, 0); break;
        case GLUT_KEY_LEFT: pan(10, 0); break;
        case GLUT_KEY_UP: pan(0, -10); break;
        case GLUT_KEY_DOWN: pan(0, 10); break;
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
    exprSetInitBounds(-5.0, 5.0, -5.0, 5.0);
    iSetTransparency(1);
    iInitializeEx(width, height, "Demo!");
    return 0;
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
                if (drawCurves) iSetColorEx(255, 255, 255, alpha / 2);
            }
            if (fabs(tracerX[j] - X[i]) < 0.5) tracerY[j] = qY[i];
            if (drawCurves) {
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
        if (drawCurves) {
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
    double Y, C, minY = height;
    int    sine_index = -1;
    for (int i = 0; i <= n_sines; i++) {
        if (i < n_sines) {
            Y = A[i] * sin(2 * PI / L[i] * ((x - originX) / scale - P[i])) * scale;
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
    L[n_sines] = iRandom(150, width / 2);
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
    iSetColorEx(45, 52, 54, 0.8);
    double w = width * 0.25;
    iFilledRectangle(width * 0.375, 50, width * 0.25, 40);
    iSetColor(255, 255, 255);
    iLineEx(width * 0.375, 50, width * 0.25, 0);
    double l = strlen(expr) * 10.0;
    iText(width * 0.375 + (w - l) / 2.0, 65, expr, GLUT_BITMAP_TIMES_ROMAN_24);
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

void zoom(int dir, int x, int y)
{
    double scale0 = scale;
    if (x < 0) x = originX;
    if (y < 0) y = originY;
    scale += dir * 0.01;
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
    // iSetColorEx(255, 255, 255, 1.0);
    // iRectangleEx(overLayLeft, overlayBottom, w, h, 2.5);
}

void drawMenu(const char* text, int w, int h, int dx, int dy, int filled, int outlined)
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
    iText(overLayLeft + dx + 5, overlayTop - dy + h - 18, title, GLUT_BITMAP_HELVETICA_12);
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
    if (tracerState[n_sines] & 1) drawMenu("Show Tracers", w, 30, 0, 180, 1, 0);
}
void deselectAll()
{
    // deselect all since empty space was clicked
    for (int j = 0; j < n_sines; j++)
        selected[j] = 0;
    n_selected = 0;
}
void handleSupOverlay(int dragging)
{
    if (dragging) return;
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
        // show/hide tracer
    }
}
void drawSinOverLay()
{
    static int w = 230, h = 275;
    drawBasicOverlay(w, h);
    drawMenu("Change Color", w, 30, 0, 30, 1, 0);
    drawColorPicker(w, 20, 0, 50, 0);
    double a = A[sinI];
    double f = width / L[sinI];
    double p = fmod(360.0 * P[sinI] / L[sinI], 360.0);
    if (p > 180.0)
        p -= 360.0;
    else if (p < -180.0)
        p += 360.0;
    drawSlideMenu("Amplitude", a, 0, height / 2.0, w, 45, 0, 95);
    drawSlideMenu("Frequency", f, 0.1, 100, w, 45, 0, 140);
    drawSlideMenu("Phase", p, -180, 180, w, 45, 0, 185);
    if (tracerState[sinI] & 2)
        drawMenu("Resume Tracer", w, 30, 0, 215, 1, 0);
    else
        drawMenu("Pause Tracer", w, 30, 0, 215, 1, 0);
    if (tracerState[sinI] & 1)
        drawMenu("Hide Tracer", w, 30, 0, 245, 1, 0);
    else
        drawMenu("Show Tracer", w, 30, 0, 245, 1, 0);
    drawMenu("Remove Curve", w, 30, 0, 275, 1, 0);
}
void handleSinOverlay(int dragging)
{
    int        dx = X0 - overLayLeft, dy = overlayTop - Y0;
    static int w = 230, h = 200;
    if (30 <= dy && dy <= 60) {
        double H = 360.0 * dx / w;
        iHSVtoRGB(H, 1.0, 1.0, C[sinI]);
    }
    else {
        if (dragging) return;
        if (0 <= dy && dy <= 30) { iRandomColor(1.0, 1.0, C[sinI]); }
        else if (dy <= 80) {
            // change tracer speed
        }
        else if (dy <= 140) {
            // resume tracer
        }
        else if (dy <= 170) {
            printf("pause/resume\n");
            // pause/resume tracer
            if (tracerState[sinI] & 2) {
                // resume
                tracerdt[sinI] += iGetTime() - tracerPt[sinI];
                tracerState[sinI] &= ~2;
            }
            else {
                // pause
                tracerPt[sinI] = iGetTime();
                tracerState[sinI] |= 2;
            }
            printf("state: %d\n", tracerState[sinI]);
            overlayState = 0;
        }
        else if (dy <= 200) {
            printf("show/hide\n");
            // show/hide tracers
            if (tracerState[sinI] & 1)
                tracerState[sinI] &= ~1;
            else
                tracerState[sinI] |= 1;
            printf("state: %d\n", tracerState[sinI]);
            overlayState = 0;
        }
        else if (dy <= 20) {
            // show/hide tracers
        }
    }
}
void drawGenOverLay()
{
    static int w = 250, h = 180;
    drawBasicOverlay(w, h);
    drawMenu("Select All Curves", w, 30, 0, 30, 1, 0);
    drawMenu("Add Curve", w, 30, 0, 60, 1, 0);
    drawMenu("Change Tracer Speed", w, 30, 0, 90, 1, 0);
    drawScaleMenu(0, 40, 30, w - 73, 90);
    drawMenu("Resume Tracers", w, 30, 0, 120, 1, 0);
    if (drawCurves)
        drawMenu("Hide Curves", w, 30, 0, 150, 1, 0);
    else
        drawMenu("Show Curves", w, 30, 0, 150, 1, 0);
    drawMenu("Show Help", w, 30, 0, 180, 1, 0);
}
void handleGenOverlay(int dragging)
{
    if (dragging) return;
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
        showHelp     = 1;
        overlayState = 0;
    }
}
void locateTracers()
{
    double t = iGetTime();
    double x = t * tracerSpeed - tracerSyncdt;
    double s = 0.0;
    for (int i = 0; i <= n_sines; i++) {
        if ((tracerState[i] & 1) && !(tracerState[i] & 2)) { // showed and resumed
            if (tracersSynced) {
                if (tracersUnidirectional)
                    tracerX[i] = fmod(x + width, width);
                else
                    tracerX[i] = fabs(fmod(x + 2 * width, 2 * width) - width);
            }
            else {
                x = (t - tracerdt[i]) * tracerSpeed;
                if (i < n_sines) {
                    if (tracersUnidirectional)
                        tracerX[i] = fmod(x + P[i] + width + originX, width), s += fmod(s + P[i] + width, width);
                    else
                        tracerX[i] = fabs(fmod(x + P[i] + 2 * width + originX, 2 * width) - width),
                        s += fmod(s + P[i] + 2 * width, 2 * width);
                }
                else {
                    if (tracersUnidirectional)
                        tracerX[n_sines] = fmod(x + s + width + originX, width);
                    else
                        tracerX[n_sines] = fabs(fmod(x + s + 2 * width + originX, 2 * width) - width);
                }
            }
        }
    }
}
void drawTracers()
{
    for (int i = 0; i <= n_sines; i++) {
        if (tracerState[i] & 1) {
            if (i < n_sines) {
                iSetColor(C[i][0], C[i][1], C[i][2]);
                iFilledCircle(tracerX[i], tracerY[i], 4);
            }
            else {
                iSetColorEx(255, 255, 255, 0.85);
                iFilledCircle(tracerX[n_sines], tracerY[n_sines], 5);
                iSetColorEx(255, 255, 255, 1);
                iCircleEx(tracerX[n_sines], tracerY[n_sines], 8, 2, 1, 50, 5, 1);
            }
        }
    }
}
void drawBottomOverlay()
{
    char text[64];
    iSetColorEx(45, 52, 54, 0.95);
    iFilledRectangle(0, 0, width, 22);
    iSetColor(255, 255, 255);
    iText(width - 20, 4, "+", GLUT_BITMAP_TIMES_ROMAN_24);
    iSetColorEx(0, 0, 0, 0.2);
    iFilledRectangle(width - 26, 0, 26, 22);
    if (mY <= 22 && mX >= width - 26) {
        iSetColor(255, 255, 255);
        iRectangleEx(width - 26, 0, 25, 22, 1, 10, 5);
    }
    snprintf(text, 80, "%3d", n_sines);
    iSetColor(255, 255, 255);
    iText(width - 110, 8, text, GLUT_BITMAP_HELVETICA_12);
    if (mY <= 22 && mX >= width - 110 && mX <= width - 95) {
        iSetColor(255, 255, 255);
        iRectangleEx(width - 112, 0, 26, 22, 1, 10, 5);
    }
    iText(width - 85, 8, "sinusoids", GLUT_BITMAP_HELVETICA_12);
    iText(width - 137, 4, "-", GLUT_BITMAP_TIMES_ROMAN_24);
    iSetColorEx(0, 0, 0, 0.2);
    iFilledRectangle(width - 143, 0, 26, 22);
    if (mY <= 22 && mX >= width - 143 && mX <= width - 118) {
        iSetColor(255, 255, 255);
        iRectangleEx(width - 143, 0, 25, 22, 1, 10, 5);
    }

    iSetColor(255, 255, 255);
    iText(width - 166, 4, "+", GLUT_BITMAP_TIMES_ROMAN_24);
    iSetColorEx(0, 0, 0, 0.2);
    iFilledRectangle(width - 172, 0, 26, 22);
    if (mY <= 22 && mX >= width - 172) {
        iSetColor(255, 255, 255);
        iRectangleEx(width - 172, 0, 25, 22, 1, 10, 5);
    }
    snprintf(text, 64, "Tracer speed %0.0lf px/s", tracerSpeed);
    if (tracersUnidirectional)
        strcat(text, "  >>");
    else
        strcat(text, "  <>");
    iSetColor(255, 255, 255);
    iText(width - 325, 8, text, GLUT_BITMAP_HELVETICA_12);
    if (mY <= 22 && mX >= width - 198 && mX <= width - 55) {
        iSetColor(255, 255, 255);
        iRectangleEx(width - 198, 0, 26, 22, 1, 10, 5);
    }
    iText(width - 348, 4, "-", GLUT_BITMAP_TIMES_ROMAN_24);
    iSetColorEx(0, 0, 0, 0.2);
    iFilledRectangle(width - 354, 0, 26, 22);
    if (mY <= 22 && mX >= width - 354 && mX <= width - 258) {
        iSetColor(255, 255, 255);
        iRectangleEx(width - 354, 0, 25, 22, 1, 10, 5);
    }
    iSetColor(255, 255, 255);
    snprintf(text, 64, "Scale: %0.2lf", scale);
    iText(width - 430, 8, text, GLUT_BITMAP_HELVETICA_12);
    snprintf(text, 64, "Origin: (%4.0lf, %4.0lf)", originX, originY);
    iText(width - 550, 8, text, GLUT_BITMAP_HELVETICA_12);
    if (n_selected == 1) {
        int i = 0;
        while (!selected[i])
            i++;
        double a = A[i];
        double w = 2 * PI / L[i];
        double p = fmod(360.0 / L[i] * P[i], 360);
        if (p > 180.0)
            p -= 360.0;
        else if (p < -180.0)
            p += 360.0;
        if (p >= 0)
            snprintf(text, 64, "y = %03.3lf sin ( %3.3lf x + %3.3lf )", a, w, p);
        else
            snprintf(text, 64, "y = %3.3lf sin ( %3.3lf x - %3.3lf )", a, w, fabs(p));
        iText(55, 8, text, GLUT_BITMAP_HELVETICA_12);
    }
    iSetColor(255, 255, 255);
    if (drawMode)
        iText(5, 8, "_--", GLUT_BITMAP_HELVETICA_18);
    else if (takeExprInput)
        iText(5, 8, "f(x,y)", GLUT_BITMAP_HELVETICA_12);
    else
        iText(5, 5, " ~ ", GLUT_BITMAP_TIMES_ROMAN_24);
    iSetColorEx(0, 0, 0, 0.2);
    iFilledRectangle(0, 0, 40, 22);
}
void handleBottomOverlay() {}

int inOverlay(int x, int y) { return overLayLeft <= x && x <= overlayRight && overlayBottom <= y && y <= overlayTop; }

const char helpStrings[14][128] = {
    "Drag mouse on empty area to pan. Scroll (mouse middle button) to zoom in/out.",
    "Click to select a curve and drag to adjust the shape. Scroll to change phase.",
    "You can also Ctrl select multiple curves and drag adjust all of them.",
    "Selecting the summation curve selects and adjusts all the composing sinusoids.",
    "Right clicking anywhere gives you context menus for further customiztion options.",
    "Tracers can be adjusted via the general context menu (right clicking on empty area).",
    "You can reshape the summation to the graph of an equation or a hand-drawn curve.",
    "Ctrl + G switches you to Graph Mode.",
    "Ctrl + D switches you to Drawing Mode.",
    "The bottom bar gives you information about the scale, translation of the graph",
    "as well as the tracer speed, the number of curves and their equations.",
    "Press the +/- button in the bottomleft corner to add/remove curves.",
    "Or you can enter any number (upto 512) to randomly generate that number of curves.",
    "Enjoy playing with the sinusoids!"};

const char shortcuts[26][2][128] = {
    {"F", "Fullscreen"},
    {"Insert", "Add curve"},
    {"Delete", "Remove Curve"},
    {"Ctrl + A", "Select All Curves"},
    {"Alt + A", "Increase Amplitude"},
    {"Alt + Shift + A", "Decrease Amplitude"},
    {"Alt + F", "Increase Frequency"},
    {"Alt + Shift + F", "Decrease Frequency"},
    {"Alt + P", "Add Phase"},
    {"Alt + Shift + P", "Subtract Phase"},
    {"Ctrl + S", "Show/Hide All Curves"},
    {"Ctrl + Z", "Undo Last Adjustment"},
    {"F1 Ctrl + H", "Show Help"},
    {"+", "Increase Tracer Speed"},
    {"-", "Decrease Tracer Speed"},
    {"Alt + S", "Sync Tracers"},
    {"Alt + Shift + S", "Desync Tracers"},
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

const char aboutString[][128] = {
    "A simple application written in C, for you to explore and play with sines and cosines.",
    "Ashrafur Rahman | Octobar 2020"};

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
    for (int i = 0; i < 14; i++) {
        iText(width - 485, height - i * 20 - 60 - 5 * (i == 13), helpStrings[i], GLUT_BITMAP_HELVETICA_12);
    }
    iText(width - 485, height - 360, "Shortcuts", GLUT_BITMAP_HELVETICA_18);
    iLine(width - 490, height - 370, width - 15, height - 370);
    for (int i = 0; i < 26; i++) {
        iText(width - 485 + 250 * (i >= 13), height - (i % 13) * 15 - 390, shortcuts[i][0], GLUT_BITMAP_HELVETICA_12);
        iText(width - 395 + 250 * (i >= 13), height - (i % 13) * 15 - 390, shortcuts[i][1], GLUT_BITMAP_HELVETICA_12);
    }
    iText(width - 485, 45, "About", GLUT_BITMAP_HELVETICA_12);
    iLine(width - 490, 40, width - 95, 40);
    for (int i = 0; i < 2; i++)
        iText(width - 485, 25 - i * 14, aboutString[i], GLUT_BITMAP_HELVETICA_10);
}
