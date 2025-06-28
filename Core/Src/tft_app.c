/*
 * tft_app.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Marta
 */

#include "tft_app.h"
#include "main.h"
#include "stm32f4xx_hal.h"

uint32_t lastUpdateTime = 0;
uint32_t updateInterval = 2000;
volatile bool grid_needs_redraw = true;
int plot_index = 0;
bool plot_buffer_full = false;
float x_data[MAX_PLOT_POINTS] = {0};
volatile AxisCurr current_axis = AXIS_X;
AppScreen current_screen = SCREEN_START;
uint16_t selected_odr = 104;
float selected_fs = 2.0;
int btn_w = 90, btn_h = 40, spacing = 10;

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
        drawPlotGrid();
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
        drawGoBackButton();
    }
    if (grid_needs_redraw || plot_index >= MAX_PLOT_POINTS){
    	initPlot();
    	drawPlotGrid();
        drawGoBackButton();
        if (grid_needs_redraw){
        	grid_needs_redraw=false;
        }
        if (plot_index >= MAX_PLOT_POINTS){
        	plot_index = 0;
        	        plot_buffer_full = false;
        }
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

void drawLiveDataWithPlot(I2C_HandleTypeDef hi2c_def, float selected_fs) {
        static bool first_run = true;
        float selected_value;
        if (first_run) {
            Displ_CLS(DDDD_WHITE);
            first_run = false;
            initPlot();
            drawPlotGrid();
        }

        LSM6DS3_AccelData accel_data = LSM6DS3_read_acceleration(hi2c_def, selected_fs);
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
                sprintf(msg, "X: %.3f g", accel_data.x);
                Displ_WString(10, text_y, msg, Font12, 1, WHITE, DDDD_WHITE);

                sprintf(msg, "Y: %.3f g", accel_data.y);
                Displ_WString(10, text_y + 20, msg, Font12, 1, WHITE, DDDD_WHITE);

                sprintf(msg, "Z: %.3f g", accel_data.z);
                Displ_WString(10, text_y + 35, msg, Font12, 1, WHITE, DDDD_WHITE);

        } else {
            Displ_WString(50, PLOT_Y + PLOT_HEIGHT/2, "Sensor Failed!", Font16, 1, DD_RED, D_WHITE);
        }
}

void drawGoBackButton() {
    Displ_FillArea(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE, DDDD_WHITE);
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

void HandleTouchEvent(I2C_HandleTypeDef hi2c_def) {

    switch (current_screen) {
        case SCREEN_START:
            if (Touch_In_XY_area(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
                Touch_WaitForUntouch(300);
                current_screen = SCREEN_CONFIG1;
                drawODRConfigScreen();
            }
            break;

        case SCREEN_PLOT:
            if (Touch_In_XY_area(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE)) {
                Touch_WaitForUntouch(300);
                current_screen = SCREEN_MENU;
                drawScreenMenu();
            }
            break;
        case SCREEN_CONFIG1:
        	const uint16_t odr_values[] = {104, 208, 416, 833, 1660, 3330};

        	for (int i = 0; i < 6; i++) {
        	     int x = 20 + (i % 3) * (btn_w + spacing);
        	     int y = 50 + (i / 3) * (btn_h + spacing);
        	     if (Touch_In_XY_area(x, y, btn_w, btn_h)) {
        	         Touch_WaitForUntouch(300);
        	         selected_odr = odr_values[i];
        	         current_screen = SCREEN_CONFIG2;
        	         drawFSConfigScreen();
        	         return;
        	     }
        	     if (Touch_In_XY_area(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE)) {
        	                     Touch_WaitForUntouch(300);
        	                     current_screen = SCREEN_START;
        	                     drawStartButton();
        	                 }
        	             }
        	 break;
        case SCREEN_CONFIG2:
        	const float fs_values[] = {2.0, 4.0, 8.0, 16.0};
        	    int cols = 2;
        	    int spacing_fs = 20;
        	    int base_x_fs = (_width - (cols * btn_w + (cols - 1) * spacing_fs)) / 2;
        	    int base_y_fs = 60;

        	    for (int i = 0; i < 4; i++) {
        	        int x = base_x_fs + (i % cols) * (btn_w + spacing_fs);
        	        int y = base_y_fs + (i / cols) * (btn_h + spacing_fs);
        	        if (Touch_In_XY_area(x, y, btn_w, btn_h)) {
        	            Touch_WaitForUntouch(300);
        	            selected_fs = fs_values[i];
        	            LSM6DS3_initialize(hi2c_def, selected_odr, selected_fs);
        	            Displ_CLS(DDDD_WHITE);
        	            current_screen = SCREEN_PLOT;
        	            initPlot();
        	            drawPlotGrid();
        	            return;
        	        }
        	    }
        	    if (Touch_In_XY_area(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE)) {
        	        Touch_WaitForUntouch(300);
        	        current_screen = SCREEN_CONFIG1;
        	        drawODRConfigScreen();
        	    }
        	    break;
        case SCREEN_I2C:
            if (Touch_In_XY_area(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE)) {
                Touch_WaitForUntouch(300);
                current_screen = SCREEN_MENU;
                drawScreenMenu();
            }
            break;
        case SCREEN_DEBUG:
            if (Touch_In_XY_area(GO_BACK_BTN_X, GO_BACK_BTN_Y, GO_BACK_BTN_SIZE, GO_BACK_BTN_SIZE)) {
                Touch_WaitForUntouch(300);
                current_screen = SCREEN_MENU;
                drawScreenMenu();
            }
            break;
        case SCREEN_MENU:
        	 int cols_menu = 2;
        	 int base_x_menu = (_width - (2 * btn_w + 30)) / 2;
        	 int base_y_menu = 60;
        	 for (int i = 0; i < 4; i++) {
        	        int x = base_x_menu + (i % cols_menu) * (btn_w + 30);
        	        int y = base_y_menu + (i / cols_menu) * (btn_h + 30);
        	        int w = btn_w + 10;
        	        int h = btn_h + 10;

        	        if (Touch_In_XY_area(x, y, w, h)) {
        	            Touch_WaitForUntouch(300);
        	            switch (i) {
        	                case 0:
        	                    current_screen = SCREEN_START;
        	                    Displ_CLS(DDDD_WHITE);
        	                    drawStartButton();
        	                    break;
        	                case 1:
        	                    current_screen = SCREEN_PLOT;
        	                    Displ_CLS(DDDD_WHITE);
        	                    initPlot();
        	                    drawPlotGrid();
        	                    drawGoBackButton();
        	                    break;
        	                case 2:
        	                	current_screen = SCREEN_I2C;
        	                	Displ_CLS(DDDD_WHITE);
        	                	I2C_bus_scan(hi2c_def);
        	                    break;
        	                case 3:
        	                	current_screen = SCREEN_DEBUG;
        	                	Displ_CLS(DDDD_WHITE);
        	                	LSM6DS3_read_XL_ODR_debug_display(hi2c_def);
        	                    break;
        	            }
        	            break;
        	        }
        	    }
        	    break;
        default:
            break;
        }
}
void drawODRConfigScreen(void) {
    Displ_CLS(DDDD_WHITE);
    Displ_WString(10, 10, "Select ODR (Hz):", Font20, 1, WHITE, DDDD_WHITE);

    int base_y = 50;
    const uint16_t odr_values[] = {104, 208, 416, 833, 1660, 3330};

    for (int i = 0; i < 6; i++) {
        int x = 20 + (i % 3) * (btn_w + spacing);
        int y = base_y + (i / 3) * (btn_h + spacing);
        Displ_fillRoundRect(x, y, btn_w, btn_h, 6, DD_GREEN);
        char label[10];
        sprintf(label, "%d", odr_values[i]);
        Displ_CString(x + 10, y + 10, x + btn_w - 10, y + btn_h - 10, label, Font16, 1, WHITE, DD_GREEN);
    }

    drawGoBackButton();
}
void drawFSConfigScreen(void) {
    Displ_CLS(DDDD_WHITE);
    Displ_WString(10, 10, "Select FS (g):", Font20, 1, WHITE, DDDD_WHITE);
    const uint8_t fs_values[] = {2, 4, 8, 16};
    int cols = 2;
    int spacing_fs = 20;
    int base_x_fs = (_width - (cols * btn_w + (cols - 1) * spacing_fs)) / 2;
    int base_y_fs = 60;

    for (int i = 0; i < 4; i++) {
         int x = base_x_fs + (i % cols) * (btn_w + spacing_fs);
         int y = base_y_fs + (i / cols) * (btn_h + spacing_fs);
         Displ_fillRoundRect(x, y, btn_w, btn_h, 6, DD_BLUE);
         char label[10];
         sprintf(label, "%dg", fs_values[i]);
         Displ_CString(x + 10, y + 10, x + btn_w - 10, y + btn_h - 10, label, Font16, 1, WHITE, DD_BLUE);
    }

    drawGoBackButton();
}

void drawScreenMenu(void) {
	Displ_CLS(DDDD_WHITE);
	Displ_WString(_width/2 - 70, 20, "Main Menu", Font24, 1, WHITE, DDDD_WHITE);

	const char* labels[4] = {"Restart", "Plot", "I2C Scan", "Debug"};

	int cols = 2;
	int base_x = (_width - (2 * btn_w + 30)) / 2;
	int base_y = 60;

	    for (int i = 0; i < 4; i++) {
	        int x = base_x + (i % cols) * (btn_w + 30);
	        int y = base_y + (i / cols) * (btn_h + 30);

	        Displ_fillRoundRect(x, y, btn_w+10, btn_h+10, 8, DD_GREEN);
	        Displ_CString(x + 10, y + 10, x + btn_w, y + btn_h, labels[i], Font16, 1, WHITE, DD_GREEN);
	    }
}

void I2C_bus_scan(I2C_HandleTypeDef hi2c_def){
	char msg[32];
	uint8_t i2c_address;
	HAL_StatusTypeDef result;
	uint8_t who_am_i = 0;
	Displ_CLS(DDDD_WHITE);
	for (i2c_address = 1; i2c_address < 127; i2c_address++) {
	        result = HAL_I2C_IsDeviceReady(&hi2c_def, (i2c_address << 1), 3, 10);
	        if (result == HAL_OK) {

	        	sprintf(msg, "Device found at 0x%02X", i2c_address);
	            Displ_CString(0, 24, _width - 1, 40, msg, Font16, 1, WHITE, DDDD_WHITE);
	            who_am_i = LSM6DS3_check_who_am_i(hi2c_def);
	            if ( who_am_i == LSM6DS3_WHO_AM_I){
	                char msg2[64];
	                sprintf(msg2, "LSM6DS3 WHO_AM_I OK: 0x%02X", who_am_i);
	                Displ_CString(0, 48, _width-1, 40, msg2, Font16, 1, WHITE, DDDD_WHITE);
	            } else {
	            	Displ_CString(0, 48, _width-1, 40, "WHO_AM_I read failed", Font16, 1, WHITE, DDDD_WHITE);
	            }
	        }
	    }
	    HAL_Delay(1000);
	    Displ_WString(48, 180, "LSM6DS3 Ready!", Font20, 1, WHITE, DDDD_WHITE);
	    //Displ_WString(10, text_y, msg, Font16, 1, WHITE, DDDD_WHITE);
	    drawGoBackButton();
}

float LSM6DS3_read_XL_ODR_debug_display(I2C_HandleTypeDef hi2c_def) {
    char msg[64];
    uint8_t ctrl1_xl_value;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_def, LSM6DS3_ADDRESS << 1, CTRL1_XL,
                                                I2C_MEMADD_SIZE_8BIT, &ctrl1_xl_value, 1, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        sprintf(msg, "ODR Read FAIL: %d", status);
        Displ_CString(0, 24, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);
        return -1.0f;
    }

    sprintf(msg, "CTRL1_XL: 0x%02X", ctrl1_xl_value);
    Displ_CString(0, 24, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

    uint8_t odr_bits = (ctrl1_xl_value >> 4) & 0x0F;
    sprintf(msg, "ODR bits: 0x%X", odr_bits);
    Displ_CString(0, 48,_width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);
    float odr_value = LSM6DS3_read_XL_ODR(hi2c_def);
    if (odr_value <= 0.0f) {
        sprintf(msg, "ODR: Power down or Invalid");
    } else {
        sprintf(msg, "ODR: %.1f Hz", odr_value);
    }
    Displ_CString(0, 72, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

    uint8_t fs_bits = (ctrl1_xl_value >> 2) & 0x03;
    switch (fs_bits) {
            case 0x00: strcpy(msg, "FS: +/-2g"); break;
            case 0x01: strcpy(msg, "FS: +/-16g"); break;
            case 0x02: strcpy(msg, "FS: +/-4g"); break;
            case 0x03: strcpy(msg, "FS: +/-8g"); break;
            default: strcpy(msg, "FS: Invalid"); break;
     }
    Displ_CString(0, 96, _width - 1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

    drawGoBackButton();
    return odr_value;
}
