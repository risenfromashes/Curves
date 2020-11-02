#include "ext.h"
#include "expr.h"

char expr[256] = "";
int  exprPos   = 0;

int width = 1280, height = 720;

void drawAxii()
{
    iSetColor(255, 255, 255);
    iFilledRectangle(0, height / 2 - 2.5, width, 5);
    iFilledRectangle(width / 2 - 2.5, 0, 5, height);
}

void drawTextBox()
{
    iSetColor(255, 255, 255);
    iRectangle(0, 0, width, 36);
    iText(14, 14, expr, GLUT_BITMAP_HELVETICA_18);
}

void drawCurve(double X[], double Y[], int n)
{
    iSetColor(255, 0, 0);
    iPath(X, Y, n, 4);
}

void iDraw()
{
    iClear();
    drawAxii();
    drawTextBox();
    exprPlot(expr, drawCurve);
}
void iMouseMove(int mx, int my) {}

void iMouse(int button, int state, int mx, int my)
{
    if (state == GLUT_DOWN) {
        if (button == 3)
            exprScale += exprScale * 0.01;
        else if (button == 4)
            exprScale -= exprScale * 0.01;
    }
    exprScaleBy(exprScale);
}
void iPassiveMouseMove(int, int) {}
void iResize(int w, int h)
{
    width = w, height = h;
    exprSetScreenRes(w, h);
}
void iKeyboard(unsigned char key)
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
    // place your codes for other keys here
}

void iSpecialKeyboard(unsigned char key)
{
    static double panX = 0.0, panY = 0.0;
    switch (key) {
        case GLUT_KEY_LEFT: panX -= 10; break;
        case GLUT_KEY_RIGHT: panX += 10; break;
        case GLUT_KEY_UP: panY += 10; break;
        case GLUT_KEY_DOWN: panY -= 10; break;
        default: break;
    }
    exprPan(panX, panY);
    if (key == GLUT_KEY_END) { exit(0); }
    // place your codes for other keys here
}

int main()
{
    exprInit();
    iSetTransparency(1);
    iInitializeEx(width, height, "Demo!");
    return 0;
}
