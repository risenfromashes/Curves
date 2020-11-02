#include "../iGraphics.h"
#include <chrono>

#ifdef FREEGLUT
#include "../OpenGL/include/freeglut.h"
#endif

#ifndef PI
#define PI 3.1415926535897932
#endif
#ifndef ERR
#define ERR 1e-8
#endif

#define min(x, y) ((x < y) ? (x) : (y))
#define max(x, y) ((x > y) ? (x) : (y))

struct point {
    double x, y;
};

static int transparent = 0;

void iSetColorEx(double r, double g, double b, double a)
{
    double mmx = 255;
    if (r > mmx) r = mmx;
    if (g > mmx) g = mmx;
    if (b > mmx) b = mmx;
    r /= mmx;
    g /= mmx;
    b /= mmx;
    glColor4f(r, g, b, a);
}

static point solve_sim_eqn(double a1, double b1, double c1, double a2, double b2, double c2)
{
    double       d = a1 * b2 - a2 * b1;
    struct point p;
    p.x = (b1 * c2 - b2 * c1) / d;
    p.y = (c1 * a2 - c2 * a1) / d;
    return p;
}
int isLeft(double x1, double y1, double x2, double y2, double x3, double y3)
{
    return ((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)) > 0 ? 1 : 0;
}
// to prevent const to non-const str conversion warning
void iText(double x, double y, const char* str, void* font = GLUT_BITMAP_8_BY_13)
{
    glRasterPos3d(x, y, 0);
    int i;
    for (i = 0; str[i]; i++) {
        glutBitmapCharacter(font, str[i]); //,GLUT_BITMAP_8_BY_13, GLUT_BITMAP_TIMES_ROMAN_24
    }
}

void iPath(double X[],
           double Y[],
           int    n,
           double d       = 1,
           int    closed  = 0,
           int    dashed  = 0,
           double dash    = 10,
           double gap     = 5,
           int    aligned = 0)
{
    //  p1 ------------------- p2
    //     -------------------
    //  p0 ------------------- p3
    double pX[4], pY[4];
    double dy, dx, a1, b1, c1, a2, b2, c2, M1, M2, S;
    int    s1, s2, end = n + 2 * closed;
    point  p1, p2;
    S = 0;
    // repeating first two points like points in the middle for closed
    for (int i = 0; i < end; i++) {
        if (i == n - 1 && !closed) {
            s2 = s1;
            a2 = a1, b2 = b1, c2 = c1;
        }
        else {
            dy = Y[(i + 1) % n] - Y[i % n];
            dx = X[(i + 1) % n] - X[i % n];
            if (i == 0)
                s2 = 1;
            else {
                // only combination of inequalities that gives the right signs :3
                s2 = (X[i % n] > X[(i - 1) % n] && X[(i + 1) % n] > X[i % n]) ||
                     (X[i % n] <= X[(i - 1) % n] && X[(i + 1) % n] <= X[i % n]);
                s2 = s1 * (2 * s2 - 1);
            }
            if (dx > 0)
                a2 = -dy, b2 = dx;
            else
                a2 = dy, b2 = -dx;
            c2 = -(a2 * X[i % n] + b2 * Y[i % n]);
            M2 = sqrt(a2 * a2 + b2 * b2);
        }
        // we don't want to draw the end points normally for closed
        if ((!closed && (i == 0 || i == n - 1)) || fabs(a1 * b2 - a2 * b1) < ERR) {
            // solving the perpendicular and two parralel st lines with distance d/2 from the original
            a1 = b2, b1 = -a2, c1 = -(a1 * X[i % n] + b1 * Y[i % n]);
            p1 = solve_sim_eqn(a1, b1, c1, a2, b2, c2 + s2 * d * M2 / 2),
            p2 = solve_sim_eqn(a1, b1, c1, a2, b2, c2 - s2 * d * M2 / 2);
        }
        // we want to draw for i == n - 1 normally for closed, but not i == 0
        else if (i != 0) {
            // solving two consecutive parallel d distanced st lines
            p1 = solve_sim_eqn(a1, b1, c1 + s1 * d * M1 / 2, a2, b2, c2 + s2 * d * M2 / 2),
            p2 = solve_sim_eqn(a1, b1, c1 - s1 * d * M1 / 2, a2, b2, c2 - s2 * d * M2 / 2);
        }
        // keeping last two points
        pX[2] = p1.x, pY[2] = p1.y;
        pX[3] = p2.x, pY[3] = p2.y;
        // nothing to draw when i == 0 and i == 1 if closed
        if (i != 0 && !(closed && i == 1)) {
            if (dashed) {
                dx        = X[i % n] - X[(i - 1) % n];
                dy        = Y[i % n] - Y[(i - 1) % n];
                double dS = sqrt(dx * dx + dy * dy);
                double x, y;
                double S1 = S, S2;
                double tX[4], tY[4];
                // taking the vector approach for points at perpendicular distance
                struct point dr {
                    .x = dy / dS * d / 2, .y = -dx / dS * d / 2
                };
                if (aligned) {
                    double t, dt;
                    int    m    = floor((dS - dash - gap) / (dash + gap));       // number of dashes in between
                    double gap_ = gap + (dS - (m + 1) * (dash + gap)) / (m + 1); // leading and trailing space
                    for (int j = -1; j <= m; j++) {
                        if (j == -1)
                            t = 0, dt = dash / 2;
                        else if (j == m)
                            t = dS - dash / 2, dt = dash / 2;
                        else {
                            t  = (dash / 2 + gap_ + j * (dash + gap_));
                            dt = dash;
                        }
                        for (int k = 0; k < 2; k++) {
                            x = (t + dt * k) / dS * dx + X[(i - 1) % n];
                            y = (t + dt * k) / dS * dy + Y[(i - 1) % n];
                            if ((j == -1 && !k) || (j == m && k)) {
                                tX[1 + k] = pX[1 + k], tY[1 + k] = pY[1 + k];
                                tX[3 * k] = pX[3 * k], tY[3 * k] = pY[3 * k];
                            }
                            else {
                                tX[1 + k] = x + dr.x, tY[1 + k] = y + dr.y;
                                tX[3 * k] = x - dr.x, tY[3 * k] = y - dr.y;
                            }
                        }
                        iFilledPolygon(tX, tY, 4);
                    }
                }
                else {
                    while (S1 < dS) {
                        S2 = S1 + dash;
                        for (int j = 0; j < 2; j++) {
                            if (j == 0 && S1 <= 0) {
                                tX[1] = pX[1], tY[1] = pY[1];
                                tX[0] = pX[0], tY[0] = pY[0];
                            }
                            else if (j == 1 && S2 >= dS) {
                                tX[2] = pX[2], tY[2] = pY[2];
                                tX[3] = pX[3], tY[3] = pY[3];
                            }
                            else {
                                double t  = (j == 0 ? S1 : S2);
                                x         = t / dS * dx + X[(i - 1) % n];
                                y         = t / dS * dy + Y[(i - 1) % n];
                                tX[1 + j] = x + dr.x, tY[1 + j] = y + dr.y;
                                tX[3 * j] = x - dr.x, tY[3 * j] = y - dr.y;
                            }
                        }
                        iFilledPolygon(tX, tY, 4);
                        S1 += (dash + gap);
                    }
                    if (S2 <= dS)
                        S = S1 - dS; // gap before first dash
                    else
                        S = S1 - dash - gap - dS; // unfinished dash
                }
            }
            else
                iFilledPolygon(pX, pY, 4);
        }
        a1 = a2, b1 = b2, c1 = c2, M1 = M2, s1 = s2;
        // shifting points left
        pX[1] = pX[2], pY[1] = pY[2];
        pX[0] = pX[3], pY[0] = pY[3];
    }
}

void iRectangleEx(
    double x, double y, double dx, double dy, double d = 1, int dashed = 0, double dash = 10, double gap = 5)
{
    double X[] = {x, x + dx, x + dx, x}, Y[] = {y, y, y + dy, y + dy};
    iPath(X, Y, 4, d, 1, dashed, dash, gap, 1);
}

void iCircleEx(
    double x, double y, double r, double d = 1, int dashed = 0, int slices = 100, double dash = 10, double gap = 5)
{
    double* X = (double*)malloc(sizeof(double) * slices);
    double* Y = (double*)malloc(sizeof(double) * slices);
    double  t, dt = 2 * PI / slices;
    int     i;
    for (t = i = 0; t < 2 * PI; t += dt, i++)
        X[i] = x + r * cos(t), Y[i] = y + r * sin(t);
    iPath(X, Y, slices, d, 1, dashed, dash, gap);
}

void iLineEx(double x1, double y1, double dx, double dy, double d = 1, int dashed = 0, double dash = 10, double gap = 5)
{
    double X[] = {x1, x1 + dx}, Y[] = {y1, y1 + dy};
    iPath(X, Y, 2, d, 0, dashed, dash, gap);
}

void iSetTransparency(int state) { transparent = (state == 0) ? 0 : 1; }

double iGetTime()
{
    static clock_t start = clock();
    return (clock() - start) / double(CLOCKS_PER_SEC);
}

double iRandom(double min, double max)
{
    static int f = 1;
    if (f) {
        srand((unsigned int)time(NULL));
        f = 0;
    }
    int s = 1 << 15;
    return min + (max - min) / s * (rand() % s);
}
void iHSVtoRGB(double H, double S, double V, double rgb[])
{
    double C = S * V;
    double X = C * (1 - fabs(fmod(H / 60.0, 2) - 1));
    double m = V - C;
    double r, g, b;
    if (H >= 0 && H < 60) { r = C, g = X, b = 0; }
    else if (H >= 60 && H < 120)
        r = X, g = C, b = 0;
    else if (H >= 120 && H < 180)
        r = 0, g = C, b = X;
    else if (H >= 180 && H < 240)
        r = 0, g = X, b = C;
    else if (H >= 240 && H < 300)
        r = X, g = 0, b = C;
    else
        r = C, g = 0, b = X;
    rgb[0] = (r + m) * 255;
    rgb[1] = (g + m) * 255;
    rgb[2] = (b + m) * 255;
}
void iRGBtoHSV(double R, double G, double B, double hsv[])
{
    double r    = R / 255;
    double g    = G / 255;
    double b    = B / 255;
    double Cmax = max(r, max(g, b));
    double Cmin = min(r, min(g, b));
    double del  = Cmax - Cmin;
    double h, s, v;
    if (del == 0)
        h = 0;
    else if (Cmax == r) {
        h = 60 * fmod(6.0 + (g - b) / del, 6);
    }
    else if (Cmax == g)
        h = 60 * ((b - r) / del + 2);
    else if (Cmax == b)
        h = 60 * ((r - g) / del + 4);
    s      = Cmax == 0 ? 0 : del / Cmax;
    v      = Cmax;
    hsv[0] = h, hsv[1] = s, hsv[2] = v;
}
void iRandomColor(double S, double V, double rgb[]) { iHSVtoRGB(iRandom(0, 360), S, V, rgb); }

void iPassiveMouseMove(int, int);

void mousePassiveMoveHandlerFF(int mx, int my)
{
    iPassiveMouseMove(mx, iScreenHeight - my);
    glFlush();
}

void iResize(int width, int height);

void resizeFF(int width, int height)
{
    iScreenWidth  = width;
    iScreenHeight = height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    iResize(width, height);
    glOrtho(0.0, iScreenWidth, 0.0, iScreenHeight, -1.0, 1.0);
    glViewport(0.0, 0.0, iScreenWidth, iScreenHeight);
    glutPostRedisplay();
}

void iInitializeEx(int width = 500, int height = 500, const char* title = "iGraphics")
{

    iScreenHeight = height;
    iScreenWidth  = width;

#ifdef FREEGLUT
    int   n = 1;
    char* p[1];
    p[0] = (char*)malloc(8);
    glutInit(&n, p);
    glutSetOption(GLUT_MULTISAMPLE, 8);
#endif
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_MULTISAMPLE);
#ifdef FREEGLUT
    glEnable(GLUT_MULTISAMPLE);
#endif

    glutInitWindowSize(width, height);
    glutInitWindowPosition(10, 10);
    glutCreateWindow(title);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, width, 0.0, height, -1.0, 1.0);

    glutDisplayFunc(displayFF);
    glutReshapeFunc(resizeFF);            // added resize callback
    glutKeyboardFunc(keyboardHandler1FF); // normal
    glutSpecialFunc(keyboardHandler2FF);  // special keys
    glutMouseFunc(mouseHandlerFF);
    glutPassiveMotionFunc(mousePassiveMoveHandlerFF); // added passive mouse move callback
    glutMotionFunc(mouseMoveHandlerFF);
    glutIdleFunc(animFF);
    glAlphaFunc(GL_GREATER, 0.0f);
    glEnable(GL_ALPHA_TEST);

    if (transparent) { // added blending mode
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_LINEAR);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_LINEAR);

    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_LINEAR);

    glutMainLoop();
}