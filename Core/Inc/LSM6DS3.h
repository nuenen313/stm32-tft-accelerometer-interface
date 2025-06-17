/*
 * LSM6DS3.h
 *
 *  Created on: Jun 4, 2025
 *      Author: Marta
 */
#include "stm32f4xx_hal.h"
#include "z_displ_ILI9XXX.h"
#ifndef INC_LSM6DS3_H_
#define INC_LSM6DS3_H_

#define LSM6DS3_ADDRESS 0x6B
#define LSM6DS3_WHO_AM_I 0x69

#define FUNC_CFG_ACCESS 0x01
#define SENSOR_SYNC_TIME_FRAME 0x04
#define FIFO_CTRL1 0x06
#define FIFO_CTRL2 0x07
#define FIFO_CTRL3 0x07
#define FIFO_CTRL4 0x08
#define FIFO_CTRL5 0x0A
#define ORIENT CFG_G 0x0B
#define INT1_CTRL 0x0D
#define INT2_CTRL 0x0E
#define WHO_AM_I 0x0F
#define CTRL1_XL 0x10
#define CTRL2_G 0x11
#define CTRL3_C 0x12
#define CTRL4_C 0x13
#define CTRL5_C 0x14
#define CTRL6_C 0x15
#define CTRL7_G 0x16
#define CTRL8_XL 0x17
#define CTRL9_XL 0x18
#define CTRL10_C 0x19
#define MASTER_CONFIG 0x1A
#define WAKE_UP_SRC 0x1B
#define TAP_SRC 0x1C
#define D6D_SRC 0x1D
#define STATUS_REG 0x1E
#define OUT_TEMP_L 0x20
#define OUT_TEMP 0x21
#define OUTX_L_G 0x22
#define OUTX_H_G 0x23
#define OUTY_L_G 0x24
#define OUTY_H_G 0x25
#define OUTZ_L_G 0x26
#define OUTZ_H_G 0x27
#define OUTX_L_XL 0x28
#define OUTX_H_XL 0x29
#define OUTY_L_XL 0x2A
#define OUTY_H_XL 0x2B
#define OUTZ_L_XL 0x2C
#define OUTZ_H_XL 0x2D
#define SENSORHUB1_REG 0x2E
#define SENSORHUB2_REG 0x2F
#define SENSORHUB3_REG 0x30
#define SENSORHUB4_REG 0x31
#define SENSORHUB5_REG 0x32
#define SENSORHUB6_REG 0x33
#define SENSORHUB7_REH 0x34
#define SENSORHUB8_REG 0x35
#define SENSORHUB9_REG 0x36
#define SENSORHUB10_REG 0x37
#define SENSORHUB11_REG 0x38
#define SENSORHUB12_REG 0x39
#define FIFO_STATUS1 0x3A
#define FIFO_STATUS2 0x3B
#define FIFO_STATUS3 0x3C
#define FIFO_STATUS4 0x3D
#define FIFO_DATA_OUT_L 0x3E
#define FIFO_DATA_OUT_H 0x3F
#define TIMESTAMP0_REG 0x40
#define TIMESTAMP1_REG 0x41
#define TIMESTAMP2_REG 0x42
#define STEP_TIMESTAMP_L 0x49
#define STEP_TIMESTAMP_H 0x4A
#define STEP_COUNTER_L 0x4B
#define STEP_COUNTER_H 0x4C
#define SENSORHUB13_REG 0x4D
#define SENSORHUB14_REG 0x4E
#define SENSORHUB15_REG 0x4F
#define SENSORHUB16_REG 0x50
#define SENSORHUB17_REG 0x51
#define SENSORHUB18_REG 0x52
#define FUNC_SRC 0x53
#define TAP_CFG 0x58
#define TAP_THS_6D 0x59
#define INT_DUR2 0x5A
#define WAKE_UP_THS 0x5B
#define WAKE_UP_DUR 0x5C
#define FREE_FALL 0x5D
#define MD1_CFG 0x5E
#define MD2_CFG 0x5F
#define OUT_MAG_RAW_X_L 0x66
#define OUT_MAG_RAW_X_H 0x67
#define OUT_MAG_RAW_Y_L 0x68
#define OUT_MAG_RAW_Y_H 0x69
#define OUT_MAG_RAW_Z_L 0x6A
#define OUT_MAG_RAW_Z_H 0x6B

int LSM6DS3_check_who_am_i(I2C_HandleTypeDef hi2c_def);
int LSM6DS3_XL_CONFIG(I2C_HandleTypeDef hi2c_def);
int LSM6DS3_G_CONFIG(I2C_HandleTypeDef hi2c_def);
int LSM6DS3_XL_ODR_CONFIG(I2C_HandleTypeDef hi2c_def);
int LSM6DS3_data_ready(I2C_HandleTypeDef hi2c_def);
typedef struct {
    float x;
    float y;
    float z;
    int status;
} LSM6DS3_AccelData;
LSM6DS3_AccelData LSM6DS3_read_acceleration(I2C_HandleTypeDef hi2c_def);
float LSM6DS3_read_XL_ODR(I2C_HandleTypeDef hi2c_def);
float LSM6DS3_read_XL_ODR_debug_display(I2C_HandleTypeDef hi2c_def);
void LSM6DS3_verify_config_display(I2C_HandleTypeDef hi2c_def);
void I2C_bus_scan_and_config(void);

#endif /* INC_LSM6DS3_H_ */
