/*
 * tft_app.h
 *
 *  Created on: Jun 24, 2025
 *      Author: Marta
 */

#ifndef INC_TFT_APP_H_
#define INC_TFT_APP_H_
#include "fonts.h"
#include "z_displ_ILI9XXX.h"
#include "z_displ_ILI9XXX_test.h"
#include "z_touch_XPT2046.h"
#include "z_touch_XPT2046_test.h"
#include "z_touch_XPT2046_menu.h"
#include "LSM6DS3.h"
#include "stm32f4xx_hal.h"
#include <math.h>
#include <stdbool.h>

extern int16_t _width;
extern int16_t _height;

#define BUTTON_W 120
#define BUTTON_H 64
#define BUTTON_X ((_width - BUTTON_W)/2)
#define BUTTON_Y ((_height - BUTTON_H)/2)
#define PI 3.14159f
#define PLOT_X 10
#define PLOT_Y 30
#define PLOT_WIDTH (_width - 20)
#define PLOT_HEIGHT 120
#define MAX_PLOT_POINTS 100
#define PLOT_SCALE 2.0f
#define GO_BACK_BTN_SIZE 60
#define GO_BACK_BTN_X 250
#define GO_BACK_BTN_Y 160

extern volatile bool grid_needs_redraw;
extern float x_data[MAX_PLOT_POINTS];
extern int plot_index;
extern bool plot_buffer_full;

extern uint32_t lastUpdateTime;
extern uint32_t updateInterval;

typedef enum {
    AXIS_X,
    AXIS_Y,
    AXIS_Z
} AxisCurr;
typedef enum {
    SCREEN_START,
    SCREEN_PLOT,
    SCREEN_LIVE_DATA,
	SCREEN_MENU
} AppScreen;
extern volatile AxisCurr current_axis;
extern AppScreen current_screen;

void drawStartButton(void);
void initPlot(void);
void drawPlot(void);
void drawPlotGrid(void);
void drawGoBackButton(void);
void addPlotPoint(float x_accel);
void drawLiveDataWithPlot(I2C_HandleTypeDef hi2c_def);
void drawLiveData(I2C_HandleTypeDef hi2c_def);

#endif /* INC_TFT_APP_H_ */
