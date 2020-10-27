#include "../ext.h"
#include "expr.h"
#define MAX_WIDTH 3840

const int dx = 1;
int       N;
int       width = 1280, height = 720;
double    X[MAX_WIDTH + 10], Y[MAX_WIDTH + 10];
int       n_sines = 0;
double    A[100], L[100], P[100], C[100][3];
double    A0[100], L0[100], P0[100], PX[100];
int       selected[100] = {0}, n_selected = 0;
int       clickedState = 0;
double    X0, Y0;
int       mX, mY;
double    overLayLeft, overlayRight, overlayTop, overlayBottom;
int       newOverlay = 0;

void drawSines();
void drawAxis();
int  selectCurve(int x, int y);
void addSine();
void removeSine();

#define SUP_OVERLAY 1
#define SIN_OVERLAY 2
#define GEN_OVERLAY 3

int  overlayState = 0;
void drawSupOverLay();
void drawSinOverLay();
void drawGenOverLay();
int  inOverlay(int x, int y);

void iDraw()
{
    iClear();
    drawSines();
    drawAxis();
    switch (overlayState) {
        case SUP_OVERLAY: drawSupOverLay(); break;
        case SIN_OVERLAY: drawSinOverLay(); break;
        case GEN_OVERLAY: drawGenOverLay(); break;
    }
}

void iPassiveMouseMove(int x, int y) { mX = x, mY = y; }

void iMouseMove(int x, int y)
{
    if (clickedState && n_selected > 0) {
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
                if (L[i] > 0)
                    L[i] = max(L[i], 50);
                else if (L[i] < 0)
                    L[i] = min(L[i], -50);
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
                if (overlayState) {
                    if (inOverlay(x, y)) { return; }
                    else
                        overlayState = 0;
                }
                clickedState = 1;
            }
            int r          = selectCurve(x, y);
            shouldDeselect = r > 1; // should deselect others on mouse up
            if (button == GLUT_RIGHT_BUTTON) {
                if (n_selected == n_sines)
                    overlayState = SUP_OVERLAY;
                else if (r > 1 || n_selected == 1)
                    overlayState = SIN_OVERLAY;
                else
                    overlayState = GEN_OVERLAY;
                newOverlay = 1;
            }
            if (r) {
                // remember mouse click position and curve states
                for (int i = 0; i < n_sines; i++)
                    if (selected[i]) {
                        A0[i] = A[i], L0[i] = L[i], P0[i] = P[i];
                        PX[i] =
                            (2 * round(2 * (X0 - P[i]) / L[i] - 0.5) + 1) * L[i] / 4 + P[i]; // take the nearest peak;
                    }
            }
            else {
                // deselect all since empty space was clicked
                for (int j = 0; j < n_sines; j++)
                    selected[j] = 0;
                n_selected = 0;
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
}
void iKeyboard(unsigned char key)
{
    static int isFullScreen = 0;
    switch (key) {
        case 27: // escape
            if (isFullScreen) {
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
            if (n_sines < 100) addSine();
            break;
        case '-':
        case (unsigned char)127:
            if (n_sines >= 0) removeSine();
            break;
        case 1: {
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
    for (i = 0; i < 3; i++)
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
    for (int j = 0; j <= n_sines; j++) {
        for (int i = 0; i < N; i++) {
            if (j == 0) cY[i] = 0;
            pX[2] = pX[1] = X[i];
            if (j < n_sines) {
                qY[i] = pY[1] = height / 2 + A[j] * sin(2 * PI / L[j] * (X[i] - P[j]));
                cY[i] += qY[i] - height / 2;
                iSetColorEx(C[j][0], C[j][1], C[j][2], 0.2);
            }
            else {
                qY[i] = pY[1] = cY[i] += height / 2;
                iSetColorEx(255, 255, 255, 0.1);
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
        if (j < n_sines) {
            iSetColorEx(C[j][0], C[j][1], C[j][2], 1.0);
            stroke = 2;
            if (selected[j]) stroke += 2;
        }
        else {
            iSetColorEx(255, 255, 255, 0.75);
            stroke = 3 + 2 * (n_selected == n_sines);
        }
        iPath(X, qY, N, stroke);
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
void drawBasicOverlay(int w, int h)
{
    if (newOverlay) {
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
    iSetColorEx(0, 0, 0, 0.75);
    iFilledRectangle(overLayLeft, overlayBottom, w, h);
    iSetColorEx(255, 255, 255, 1.0);
    iRectangleEx(overLayLeft, overlayBottom, w, h, 1);
}

void drawMenu(const char* text, int w, int h, int dy, int filled)
{
    if (filled) {
        if (overLayLeft <= mX && mX <= overlayRight && overlayTop - dy <= mY && mY <= overlayTop - dy + h) {
            iSetColorEx(255, 255, 255, 0.2);
            iFilledRectangle(overLayLeft, overlayTop - dy, w, h);
        }
    }
    iSetColorEx(255, 255, 255, 1);
    iRectangleEx(overLayLeft, overlayTop - dy, w, h);
    iText(overLayLeft + 10, overlayTop - dy + 10, text, GLUT_BITMAP_HELVETICA_12);
}

void drawSupOverLay()
{
    static int w = 300, h = 210;
    drawBasicOverlay(w, h);
    drawMenu("Approximate graph of equation", w, 30, 30, 1);
    drawMenu("Approximate hand-drawn curve", w, 30, 60, 1);
    drawMenu("Scale Amplitude", w, 30, 90, 0);
    drawMenu("Scale Frequencies", w, 30, 120, 0);
    drawMenu("Change Phase", w, 30, 150, 0);
    drawMenu("Show Tracers", w, 30, 180, 1);
    drawMenu("Hide Curves", w, 30, 210, 1);
}
void drawSinOverLay()
{
    static int w = 200, h = 300;
    drawBasicOverlay(w, h);
}
void drawGenOverLay()
{
    static int w = 200, h = 300;
    drawBasicOverlay(w, h);
}
int inOverlay(int x, int y) { return overLayLeft <= x && x <= overlayRight && overlayBottom <= y && y <= overlayTop; }