#include "../iGraphics.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mmsystem.h>
#include <windows.h>

#define PI acos(-1.0)
#define curves 27

int total_points = 260;
int total_curves = 0;
int delayTime    = 100;

// initializing the width and height
double window_width  = 1366;
double window_height = 768;

// base line denotes the x axis for the curves
double base_line_x1 = 0;
double base_line_y1 = window_height / 2;
double base_line_x2 = window_width;
double base_line_y2 = window_height / 2;

// text box parameters
double box_bottom_left_x = 928;
double box_bottom_left_y = 64;
double box_length        = 310;
double box_height        = 50;

// horizontal difference between two adjacent points
double point_dx = (base_line_x2 - base_line_x1) / total_points;

// multiplicative factor for frequency and amplitude
double frequency_change = 7.6;
double amplitude_change = 13.25;

double phase_change = 0;

int    isMoving                = 1;              // 1 for running mode, 0 for pause mode
double ball_dx                 = 7.5 * point_dx; // displacement of the balls after every delay time
double ball_position_x         = 0;              // the initial position of the balls
int    ball_movement_direction = 1;              // 1 for left to right movement, 0 for reversed one
int    showWaves               = 1;              // 1 for showing the waves, 0 for not showing
int    helpMode                = 1;              // 1 for the help mode and 0 for execution mode
int    TextBoxActive           = 0;              // used for moving the text box
int    PlayMode                = 0;              // waves move or not?

// double ball_position_y;

// randomly generated colors for all the curves
int curve_red[curves];
int curve_green[curves];
int curve_blue[curves];

// variables to store randomly generated color of the text box
int textBox_red;
int textBox_green;
int textBox_blue;

// coordinates of the texts
double text_starting_x = 100;
double text_starting_y = 670;

// arrays for storing parameters of the waves, 0 indexed
double allFrequency[curves];
double allAmplitude[curves];
double all_initial_phase[curves];

// array for storing points of the resultant curve
double resultant_y[265];

// texts to display
char number_of_waves[10];      // string that shows the total number of waves
char waves_number_message[30]; // string that contains the entire message
char texts[100][100] = {"PRESS THE FOLLOWING KEYS FOR EXPLORING DIFFERENT FEATURES!!!",
                        "Toggle Between Help Mode / Execution Mode: F1",
                        "More Curves: m / M",
                        "Less Curves: l / L",
                        "Pause Animation: p / P",
                        "Resume Animation: r / R",
                        "Increase Frequency: f",
                        "Decrease Frequency: F",
                        "Increase Amplitude: a",
                        "Decrease Amplitude: A",
                        "Increase Animation Speed: +",
                        "Decrease Animation Speed: -",
                        "Move Waves to Right: Left Arrow Key",
                        "Move Waves to Left: Right Arrow Key",
                        "Play / Pause: Space Key",
                        "Toggle between Curves / Only Tracers: s / S"};

// this function creates a string to show the total number of curves
void numberToString()
{
    if (total_curves < 9) {
        number_of_waves[0] = total_curves + '0' + 1;
        number_of_waves[1] = '\0';
    }
    else {
        int temp           = total_curves + 1;
        int digit          = temp % 10;
        number_of_waves[1] = digit + '0';
        temp /= 10;
        digit              = temp % 10;
        number_of_waves[0] = digit + '0';
        number_of_waves[2] = '\0';
    }
    strcpy(waves_number_message, "Total Number of Curves: ");
    strcat(waves_number_message, number_of_waves);
    return;
}

// calculates the points for all the curves given the index
double find_y(int wave_index, double x)
{
    double y;
    double amplitude = amplitude_change * allAmplitude[wave_index];
    double frequency = frequency_change * allFrequency[wave_index];

    double phase = phase_change + all_initial_phase[wave_index];

    y = base_line_y1 + amplitude * sin(x * frequency + phase);

    return y;
}

// this function draws a thicker line by drawing three parallel lines
// isSpecial for comparatively bolder curves
void drawThickerLine(double x1, double y1, double x2, double y2, int isSpecial)
{
    double dh = 1;
    iLine(x1, y1, x2, y2);
    iLine(x1, y1 - dh, x2, y2 - dh);
    iLine(x1, y1 + dh, x2, y2 + dh);

    if (isSpecial) {
        // place code here
    }

    return;
}

void drawCurve(int wave_index)
{
    int red   = curve_red[wave_index];
    int green = curve_green[wave_index];
    int blue  = curve_blue[wave_index];
    iSetColor(showWaves * red, showWaves * green, showWaves * blue);

    for (int i = 0; i < total_points; i++) {
        double y1 = find_y(wave_index, i * point_dx);
        double y2 = find_y(wave_index, i * point_dx + point_dx);
        iFilledCircle(i * point_dx, y1, 0.35);
        if (i < total_points - 1) drawThickerLine(i * point_dx, y1, i * point_dx + point_dx, y2, 0);
        resultant_y[i] += (y1 - base_line_y1);
    }

    return;
}

void drawResultant()
{

    iSetColor(showWaves * 255, showWaves * 255, showWaves * 255);

    for (int i = 0; i < total_points; i++) {
        iFilledCircle(i * point_dx, base_line_y1 + resultant_y[i], 0.35);
        if (i < total_points - 1)
            drawThickerLine(i * point_dx,
                            base_line_y1 + resultant_y[i],
                            i * point_dx + point_dx,
                            base_line_y1 + resultant_y[i + 1],
                            1);
    }

    for (int i = 0; i < total_points; i++)
        resultant_y[i] = 0;
    return;
}

void drawBall()
{
    double resultant_curve_y = 0;
    for (int i = 0; i < total_curves; i++) {
        iSetColor(curve_red[i], curve_green[i], curve_blue[i]);
        double y = find_y(i, ball_position_x);
        resultant_curve_y += (y - base_line_y1);
        iFilledCircle(ball_position_x, y, 7.5);
    }
    iSetColor(255, 255, 255);
    iFilledCircle(ball_position_x, resultant_curve_y + base_line_y1, 7.5);
    return;
}

void BallMove()
{
    if (ball_movement_direction) {
        ball_position_x += ball_dx;
        if (ball_position_x >= window_width) {
            ball_movement_direction = !ball_movement_direction;
            ball_position_x -= (2 * (ball_position_x - window_width));
        }
    }
    else {
        ball_position_x -= ball_dx;
        if (ball_position_x <= 0) {
            ball_movement_direction = !ball_movement_direction;
            ball_position_x         = -ball_position_x;
        }
    }
    return;
}

void addCurve()
{
    if (total_curves == 25) return;
    total_curves++;
    int wave_index          = total_curves - 1;
    curve_red[wave_index]   = rand() % (240 - 25 + 1) + 25;
    curve_green[wave_index] = rand() % (240 - 25 + 1) + 25;
    curve_blue[wave_index]  = rand() % (240 - 25 + 1) + 25;

    allFrequency[wave_index] = (rand() % (35000 - 12000 + 1) + 12000) * 1e-7;
    allAmplitude[wave_index] = (rand() % ((int)window_height / 6 - 1 + 1) + 1) * 0.1;

    all_initial_phase[wave_index] = (rand() % 62831) * 1e-4;

    delayTime -= (pow(total_curves, 1.6) / 2);

    return;
}

void reduceCurve()
{
    if (total_curves == 3) return;
    total_curves--;

    delayTime += (pow(total_curves, 1.6) / 2);

    return;
}

void boxColorChange()
{

    textBox_red   = rand() % (255 - 95 + 1) + 95;
    textBox_blue  = rand() % (255 - 95 + 1) + 95;
    textBox_green = rand() % (255 - 95 + 1) + 95;

    return;
}

void iDraw()
{
    iClear();

    if (helpMode) {
        iShowBMP(0, 0, "hello2.bmp");
        iSetColor(123, 252, 3);
        for (int i = 0; i < 16; i++) {
            if (i == 0)
                iText(text_starting_x + 200, text_starting_y + 50, texts[i], GLUT_BITMAP_TIMES_ROMAN_24);
            else
                iText(text_starting_x, text_starting_y - 25 * i, texts[i], GLUT_BITMAP_HELVETICA_18);
        }
    }

    else {
        numberToString();
        iSetColor(textBox_red, textBox_green, textBox_blue);
        iRectangle(box_bottom_left_x, box_bottom_left_y, box_length, box_height);
        iText(box_bottom_left_x + 30,
              box_bottom_left_y + (box_height) / 2 - (box_height) / 20,
              waves_number_message,
              GLUT_BITMAP_TIMES_ROMAN_24);
        iSetColor(255, 255, 0);
        iText(1000, 75, "Drag the box to move");
        iSetColor(showWaves * 255, showWaves * 255, showWaves * 255);
        drawThickerLine(base_line_x1, base_line_y1, base_line_x2, base_line_y2, 1);

        if (PlayMode) phase_change += PI / 15;

        for (int i = 0; i < total_curves; i++)
            drawCurve(i);
        drawResultant();
        drawBall();
        //        if (isMoving) {
        //            ballDraw(245, 15, 43, wave1_x, wave1_y, 1);
        //            ballDraw(136, 217, 220, wave2_x, wave2_y, 1);
        //            ballDraw(202, 149, 43, wave3_x, wave3_y, 1);
        //            ballDraw(46, 129, 4, wave4_x, wave4_y, 1);
        ////            ball_latest_x[5] = wave5_x[ball_position_index];
        ////            ball_latest_y[5] = wave5_y[ball_position_index];
        ////            ballDraw(89, 210, 120, wave5_x, wave5_y, 1, ball_latest_x[5], ball_latest_y[5]);
        ////            ball_latest_x[6] = wave6_x[ball_position_index];
        ////            ball_latest_y[6] = wave6_y[ball_position_index];
        ////            ballDraw(189, 12, 40, wave6_x, wave6_y, 1, ball_latest_x[6], ball_latest_y[6]);
        //            ballDraw(17, 141, 222, resultant_x, resultant_y, 1);
        //        }
        //
        //        else {
        //            ballDraw(245, 15, 43, wave1_x, wave1_y, 0);
        //            ballDraw(136, 217, 220, wave2_x, wave2_y, 0);
        //            ballDraw(202, 149, 43, wave3_x, wave3_y, 0);
        //            ballDraw(46, 129, 4, wave4_x, wave4_y, 0);
        ////            ballDraw(89, 210, 120, wave5_x, wave5_y, 1, ball_latest_x[5], ball_latest_y[5]);
        ////            ballDraw(189, 12, 40, wave6_x, wave6_y, 1, ball_latest_x[6], ball_latest_y[6]);
        //            ballDraw(17, 141, 222, resultant_x, resultant_y, 0);
        //        }
    }
    return;
}

void iKeyboard(unsigned char key)
{

    if (helpMode) return;

    switch (key) {
        case 's':
        case 'S': showWaves = !showWaves; break;
        case 'f': frequency_change += 0.3; break;
        case 'F':
            frequency_change -= 0.3;
            if (frequency_change < 0) frequency_change = 0;
            break;
        case 'a': amplitude_change += 0.3; break;
        case 'A':
            amplitude_change -= 0.3;
            if (amplitude_change < 0) amplitude_change = 0;
            break;
        case '+': ball_dx += 2; break;
        case '-':
            ball_dx -= 2;
            if (ball_dx < 0) ball_dx = 0;
            break;
        case 'p':
        case 'P':
            isMoving = 0;
            iPauseTimer(0);
            break;
        case 'r':
        case 'R':
            isMoving = 1;
            iResumeTimer(0);
            break;
        case 'm':
        case 'M': addCurve(); break;
        case 'l':
        case 'L': reduceCurve(); break;
        case ' ':
            PlayMode = !PlayMode;
            if (PlayMode)
                iPauseTimer(0);
            else
                iResumeTimer(0);
            break;
    }
}
void iSpecialKeyboard(unsigned char key)
{
    switch (key) {
        case GLUT_KEY_F1: helpMode = !helpMode; break;
        case GLUT_KEY_LEFT:
            if (!helpMode) phase_change -= PI / 12;
            break;
        case GLUT_KEY_RIGHT:
            if (!helpMode) phase_change += PI / 12;
            break;
    }
}

void iMouseMove(int mx, int my)
{
    if (TextBoxActive) {
        box_bottom_left_x = mx - 100;
        box_bottom_left_y = my - 50;

        if (box_bottom_left_x < 0)
            box_bottom_left_x = 0;
        else if (box_bottom_left_x + box_length > window_width)
            box_bottom_left_x = window_width - box_length;
        if (box_bottom_left_y < 0)
            box_bottom_left_y = 0;
        else if (box_bottom_left_y + box_height > window_height)
            box_bottom_left_y = window_height - box_height;
    }

    //    int selected_index = 0;
    //
    //    if (my > base_line_y1){
    //        double min_y = 1e7;
    //        for (int i=0; i<total_curves; i++){
    //            double y_now = find_y(i, mx);
    //            if (y_now < base_line_y1) continue;
    //            if (y_now < min_y){
    //                selected_index = i;
    //                min_y = y_now;
    //            }
    //        }
    //    }
    //
    //    else if (my < base_line_y1){
    //        double max_y = -1e7;
    //        for (int i=0; i<total_curves; i++){
    //            double y_now = find_y(i, mx);
    //            if (y_now > base_line_y1) continue;
    //            if (y_now > max_y){
    //                selected_index = i;
    //                max_y = y_now;
    //            }
    //        }
    //    }
    //
    //    active_mode[selected_index] = 1;
    //
    //    double frequency = allFrequency[selected_index];
    //    double amplitude = allAmplitude[selected_index];
    //    double phase = all_initial_phase[selected_index];
    //
    //    allFrequency[selected_index] = (asin((my - base_line_y1)/ amplitude) - phase) / mx;
    //    allFrequency[selected_index] /= frequency_change;

    //    active_mode[selected_index] = 0;

    //    return;
}

void iMouse(int button, int state, int mx, int my)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // now this condition is perfect
        if (mx >= box_bottom_left_x && mx <= box_bottom_left_x + box_length && my >= box_bottom_left_y &&
            my <= box_bottom_left_y + box_height && TextBoxActive == 0)
            TextBoxActive = 1;
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        if (TextBoxActive) TextBoxActive = 0;
    }
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {}
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {}
}

int main()
{

    iSetTimer(delayTime, BallMove);
    iSetTimer(750, boxColorChange);
    // PlaySound("music.wav", NULL, SND_ASYNC | SND_LOOP);

    for (int i = 0; i < 3; i++)
        addCurve();
    all_initial_phase[0] = all_initial_phase[2] = 0;
    all_initial_phase[1]                        = PI / 2;

    iInitialize(window_width, window_height, "Curve");

    return 0;
}