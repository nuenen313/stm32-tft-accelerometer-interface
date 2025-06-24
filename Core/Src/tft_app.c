/*
 * tft_app.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Marta
 */

#include "tft_app.h"

uint32_t lastUpdateTime = 0;
uint32_t updateInterval = 2000;
volatile bool grid_needs_redraw = true;
int plot_index = 0;
bool plot_buffer_full = false;
float x_data[MAX_PLOT_POINTS] = {0};
volatile AxisCurr current_axis = AXIS_X;
AppScreen current_screen = SCREEN_START;

void drawStartButton(){
	Displ_CLS(DDDD_WHITE);
	char msg[64];
	sprintf(msg, "LSM6DS3 TFT Interface");
	Displ_WString(_width/2-145, _height/2-80, msg, Font20, 1, WHITE, DDDD_WHITE);
	Displ_fillRoundRect(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_H/6, DD_GREEN);
	Displ_CString(BUTTON_X + 6, BUTTON_Y + 6,
	                  BUTTON_X + BUTTON_W - 6, BUTTON_Y + BUTTON_H - 4,
	                  "START", Font24, 1, WHITE, DD_GREEN);
}

void initPlot() {
    for (int i = 0; i < MAX_PLOT_POINTS; i++) {
        x_data[i] = 0.0f;
    }
    plot_index = 0;
    plot_buffer_full = false;
    grid_needs_redraw = true;
    Displ_FillArea(0, 0, _width, 30, DDDD_WHITE);
    char label[32];
    switch (current_axis) {
        case AXIS_X: strcpy(label, "LSM6DS3 X-Axis Plot"); break;
        case AXIS_Y: strcpy(label, "LSM6DS3 Y-Axis Plot"); break;
        case AXIS_Z: strcpy(label, "LSM6DS3 Z-Axis Plot"); break;
    }
        Displ_WString(10, 5, label, Font16, 1, WHITE, DDDD_WHITE);
}

void drawPlot() {
    int points_to_draw = plot_buffer_full ? MAX_PLOT_POINTS : plot_index;
    if (points_to_draw < 2) return;
    float x_step = (float)PLOT_WIDTH / (MAX_PLOT_POINTS - 1);
    for (int i = 0; i < points_to_draw - 1; i++) {
        int current_idx = plot_buffer_full ? (plot_index + i) % MAX_PLOT_POINTS : i;
        int next_idx = plot_buffer_full ? (plot_index + i + 1) % MAX_PLOT_POINTS : i + 1;

        float current_val = x_data[current_idx];
        float next_val = x_data[next_idx];

        int x1 = PLOT_X + (int)(i * x_step);
        int x2 = PLOT_X + (int)((i + 1) * x_step);

        int y1 = PLOT_Y + PLOT_HEIGHT/2 - (int)((current_val / PLOT_SCALE) * (PLOT_HEIGHT/2));
        int y2 = PLOT_Y + PLOT_HEIGHT/2 - (int)((next_val / PLOT_SCALE) * (PLOT_HEIGHT/2));

        if (y1 < PLOT_Y) y1 = PLOT_Y;
        if (y1 > PLOT_Y + PLOT_HEIGHT) y1 = PLOT_Y + PLOT_HEIGHT;
        if (y2 < PLOT_Y) y2 = PLOT_Y;
        if (y2 > PLOT_Y + PLOT_HEIGHT) y2 = PLOT_Y + PLOT_HEIGHT;

        Displ_Line(x1, y1, x2, y2, ORANGE);
        Displ_fillCircle(x1, y1, 1, YELLOW);
    }
    if (points_to_draw > 0) {
        int latest_idx = plot_buffer_full ? (plot_index - 1 + MAX_PLOT_POINTS) % MAX_PLOT_POINTS : plot_index - 1;
        if (latest_idx >= 0) {
            float latest_val = x_data[latest_idx];
            int latest_x = PLOT_X + (int)((points_to_draw - 1) * x_step);
            int latest_y = PLOT_Y + PLOT_HEIGHT/2 - (int)((latest_val / PLOT_SCALE) * (PLOT_HEIGHT/2));
            if (latest_y < PLOT_Y) latest_y = PLOT_Y;
            if (latest_y > PLOT_Y + PLOT_HEIGHT) latest_y = PLOT_Y + PLOT_HEIGHT;

            Displ_fillCircle(latest_x, latest_y, 2, DD_RED);
        }
    }
}

void addPlotPoint(float x_accel) {
    x_data[plot_index] = x_accel;
    plot_index++;

    if (grid_needs_redraw){
    	initPlot();
    	drawPlotGrid();
    	grid_needs_redraw=false;
    }
    if (plot_index >= MAX_PLOT_POINTS) {
        plot_index = 0;
        plot_buffer_full = false;
        initPlot();
        drawPlotGrid();
    }
}

void drawPlotGrid() {
    Displ_FillArea(PLOT_X, PLOT_Y, PLOT_WIDTH, PLOT_HEIGHT, BLACK);
    Displ_Border(PLOT_X, PLOT_Y, PLOT_WIDTH, PLOT_HEIGHT, 1, WHITE);
    int center_y = PLOT_Y + PLOT_HEIGHT / 2;
    for (int x = PLOT_X; x < PLOT_X + PLOT_WIDTH; x++) {
        Displ_Pixel(x, center_y, WHITE);
    }
    int plus_1g_y = PLOT_Y + PLOT_HEIGHT / 4;
    for (int x = PLOT_X; x < PLOT_X + PLOT_WIDTH; x += 4) {
        Displ_Pixel(x, plus_1g_y, WHITE);
    }
    int minus_1g_y = PLOT_Y + (3 * PLOT_HEIGHT) / 4;
    for (int x = PLOT_X; x < PLOT_X + PLOT_WIDTH; x += 4) {
        Displ_Pixel(x, minus_1g_y, WHITE);
    }
    Displ_WString(PLOT_X - 25, plus_1g_y - 8, "+1g", Font12, 1, WHITE, D_WHITE);
    Displ_WString(PLOT_X - 20, center_y - 8, "0g", Font12, 1, WHITE, D_WHITE);
    Displ_WString(PLOT_X - 25, minus_1g_y - 8, "-1g", Font12, 1, WHITE, D_WHITE);
}

void drawLiveDataWithPlot(I2C_HandleTypeDef hi2c_def) {
        static bool first_run = true;
        float selected_value;
        if (first_run) {
            Displ_CLS(DDDD_WHITE);
            first_run = false;
            drawPlotGrid();
            drawGoBackButton();
        }

        LSM6DS3_AccelData accel_data = LSM6DS3_read_acceleration(hi2c_def);
        switch (current_axis) {
            case AXIS_X: selected_value = accel_data.x; break;
            case AXIS_Y: selected_value = accel_data.y; break;
            case AXIS_Z: selected_value = accel_data.z; break;
        }

        if (accel_data.status == 1) {
            addPlotPoint(selected_value);
            drawPlot();
            int text_y = PLOT_Y + PLOT_HEIGHT + 15;
            Displ_FillArea(0, text_y, GO_BACK_BTN_X, _height - text_y, DDDD_WHITE);


            char msg[64];
            if (current_axis == AXIS_X) {
                sprintf(msg, "X: %.3f g", accel_data.x);
                Displ_WString(10, text_y, msg, Font16, 1, WHITE, DDDD_WHITE);

                sprintf(msg, "Y: %.3f g", accel_data.y);
                Displ_WString(10, text_y + 20, msg, Font12, 1, WHITE, DDDD_WHITE);

                sprintf(msg, "Z: %.3f g", accel_data.z);
                Displ_WString(10, text_y + 35, msg, Font12, 1, WHITE, DDDD_WHITE);

            } else if (current_axis == AXIS_Y) {
                sprintf(msg, "X: %.3f g", accel_data.x);
                Displ_WString(10, text_y, msg, Font12, 1, WHITE, DDDD_WHITE);

                sprintf(msg, "Y: %.3f g", accel_data.y);
                Displ_WString(10, text_y + 20, msg, Font16, 1, WHITE, DDDD_WHITE);

                sprintf(msg, "Z: %.3f g", accel_data.z);
                Displ_WString(10, text_y + 35, msg, Font12, 1, WHITE, DDDD_WHITE);

            } else if (current_axis == AXIS_Z) {
                sprintf(msg, "X: %.3f g", accel_data.x);
                Displ_WString(10, text_y, msg, Font12, 1, WHITE, DDDD_WHITE);

                sprintf(msg, "Y: %.3f g", accel_data.y);
                Displ_WString(10, text_y + 20, msg, Font12, 1, WHITE, DDDD_WHITE);

                sprintf(msg, "Z: %.3f g", accel_data.z);
                Displ_WString(10, text_y + 35, msg, Font16, 1, WHITE, DDDD_WHITE);
            }

        } else {
            Displ_WString(50, PLOT_Y + PLOT_HEIGHT/2, "Sensor Failed!", Font16, 1, DD_RED, D_WHITE);
        }
}

void drawLiveData(I2C_HandleTypeDef hi2c_def) {
    if (HAL_GetTick() - lastUpdateTime >= updateInterval) {
        lastUpdateTime = HAL_GetTick();

        Displ_CLS(DDDD_WHITE);
        Displ_CString(0, 0, _width-1, 20, "LSM6DS3 Live Data:", Font16, 1, WHITE, D_WHITE);

        LSM6DS3_AccelData accel_data = LSM6DS3_read_acceleration(hi2c_def);
        if (accel_data.status == 1) {
            char msg[64];

            sprintf(msg, "Accel X: %.2f g", accel_data.x);
            Displ_CString(0, 30, _width-1, 20, msg, Font16, 1, WHITE, D_WHITE);

            sprintf(msg, "Accel Y: %.2f g", accel_data.y);
            Displ_CString(0, 50, _width-1, 20, msg, Font16, 1, WHITE, D_WHITE);

            sprintf(msg, "Accel Z: %.2f g", accel_data.z);
            Displ_CString(0, 70, _width-1, 20, msg, Font16, 1, WHITE, D_WHITE);

            float total = sqrt(accel_data.x*accel_data.x + accel_data.y*accel_data.y + accel_data.z*accel_data.z);
            sprintf(msg, "Total: %.2f g", total);
            Displ_CString(0, 90, _width-1, 20, msg, Font16, 1, WHITE, D_WHITE);

        } else {
            Displ_CString(0, 30, _width-1, 20, "Sensor Read Failed!", Font16, 1, RED, D_WHITE);
        }
    }
}

void drawGoBackButton() {
    Displ_FillArea(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE, DDDD_WHITE);
    //Displ_Border(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE, 1, WHITE);
    //Displ_FillArea(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE, RED);
    Displ_fillRoundRect(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE/6, RED);
    int mid_y = GO_BACK_BTN_Y + GO_BACK_BTN_SIZE / 2;
    int left_x = GO_BACK_BTN_X + 5;
    int right_x = GO_BACK_BTN_X + GO_BACK_BTN_SIZE - 5;
    int top_y = GO_BACK_BTN_Y + 5;
    int bottom_y = GO_BACK_BTN_Y + GO_BACK_BTN_SIZE - 5;

    for (int y = top_y; y <= bottom_y; y++) {
            int x_start = right_x - (int)((float)(right_x - left_x) * (float)(y - top_y) / (float)(mid_y - top_y));
            int x_end = right_x;
            if (y > mid_y) {
                x_start = right_x - (int)((float)(right_x - left_x) * (float)(bottom_y - y) / (float)(bottom_y - mid_y));
            }
            if (x_start < left_x) x_start = left_x;
            for (int x = x_start; x <= x_end; x++) {
                Displ_Pixel(x, y, WHITE);
            }
        }
        Displ_Line(right_x, top_y, left_x, mid_y, WHITE);
        Displ_Line(left_x, mid_y, right_x, bottom_y, WHITE);
        Displ_Line(right_x, bottom_y, right_x, top_y, WHITE);
}
