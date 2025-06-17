/*
 * LSM6DS3.c
 *
 *  Created on: Jun 5, 2025
 *      Author: Marta
 */
#include "LSM6DS3.h"
#include "stm32f4xx_hal.h"
#include <math.h>

int LSM6DS3_check_who_am_i(I2C_HandleTypeDef hi2c_def){
	static uint8_t who_am_i;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_def, LSM6DS3_ADDRESS << 1, WHO_AM_I,
			I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, HAL_MAX_DELAY);
	if (status == HAL_OK & who_am_i == LSM6DS3_WHO_AM_I){
		return who_am_i;
	}
	else{
		return 0;
	}
}

int LSM6DS3_XL_CONFIG(I2C_HandleTypeDef hi2c_def){
	static uint8_t data = 0x69;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c_def, LSM6DS3_ADDRESS << 1, CTRL1_XL,
			I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
	if (status != HAL_OK){
		return 1;
	}
	else {
		return 0;
	}
}

int LSM6DS3_G_CONFIG(I2C_HandleTypeDef hi2c_def){
	static uint8_t data = 0x00;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c_def, LSM6DS3_ADDRESS << 1, CTRL2_G,
			I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY); //wylaczony
	if (status != HAL_OK){
		return 1;
	}
	else {
		return 0;
	}
}

int LSM6DS3_XL_ODR_CONFIG(I2C_HandleTypeDef hi2c_def){
	static uint8_t data = 0x09;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c_def, LSM6DS3_ADDRESS << 1, CTRL8_XL,
			I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY); //ODR/4
	if (status != HAL_OK){
		return 1;
	}
	else{
		return 0;
	}
}

float LSM6DS3_read_XL_ODR(I2C_HandleTypeDef hi2c_def) {
    uint8_t ctrl1_xl_value;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_def, LSM6DS3_ADDRESS << 1, CTRL1_XL,
                                                I2C_MEMADD_SIZE_8BIT, &ctrl1_xl_value, 1, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        return 0.0f;
    }
    uint8_t odr_bits = (ctrl1_xl_value >> 4) & 0x0F;
    switch (odr_bits) {
        case 0x0: return 0.0f;
        case 0x1: return 12.5f;
        case 0x2: return 26.0f;
        case 0x3: return 52.0f;
        case 0x4: return 104.0f;
        case 0x5: return 208.0f;
        case 0x6: return 416.0f;
        case 0x7: return 833.0f;
        case 0x8: return 1666.0f;
        case 0x9: return 3333.0f;
        case 0xA: return 6666.0f;
        default: return 0.0f;
    }
}

int LSM6DS3_data_ready(I2C_HandleTypeDef hi2c_def){
	uint8_t status_val;
	static uint8_t xl_data_ready = 0x01;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_def, LSM6DS3_ADDRESS << 1, STATUS_REG,
			I2C_MEMADD_SIZE_8BIT, &status_val, 1, HAL_MAX_DELAY);
	if (status == HAL_OK & status_val & xl_data_ready){
		return 1;
	}
	else{
		return 0;
	}
}

LSM6DS3_AccelData LSM6DS3_read_acceleration(I2C_HandleTypeDef hi2c_def){
	LSM6DS3_AccelData result;
	uint8_t raw_data[6];
	int16_t accel_data[3];
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_def, LSM6DS3_ADDRESS << 1, OUTX_L_XL,
	                                                I2C_MEMADD_SIZE_8BIT, raw_data, 6, HAL_MAX_DELAY);
	if (status != HAL_OK) {
	        result.x = NAN;
	        result.y = NAN;
	        result.z = NAN;
	        result.status = 0;
	        return result;
	    }
	accel_data[0] = (int16_t)((uint16_t)raw_data[1] << 8 | raw_data[0]);
	accel_data[1] = (int16_t)((uint16_t)raw_data[3] << 8 | raw_data[2]);
	accel_data[2] = (int16_t)((uint16_t)raw_data[5] << 8 | raw_data[4]);

	 result.x = accel_data[0] * 4.0 / 32768.0;
	 result.y = accel_data[1] * 4.0 / 32768.0;
	 result.z = accel_data[2] * 4.0 / 32768.0;
	 result.status = 1;

	 return result;
 }
