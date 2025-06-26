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

int LSM6DS3_XL_CONFIG(I2C_HandleTypeDef hi2c_def, float odr_hz, float fs_g){
	uint8_t odr_bits, fs_bits;
	uint8_t data;
	if (odr_hz <= 104.0f){ odr_bits = 0x04;}
	else if (odr_hz <= 208.0f){ odr_bits = 0x05;}
	else if (odr_hz <= 416.0f){ odr_bits = 0x06;}
	else if (odr_hz <= 833.0f){ odr_bits = 0x07;}
	else if (odr_hz <= 1660.0f) { odr_bits = 0x08;}
	else if (odr_hz <= 3330.0f){ odr_bits = 0x09;}
	else { odr_bits = 0x0A;}

	if (fs_g <= 2.0f){ fs_bits = 0x00;}
	else if (fs_g <= 4.0f){ fs_bits = 0x02;}
	else if (fs_g <= 8.0f){ fs_bits = 0x04;}
	else { fs_bits = 0x03;}

	data = ( odr_bits << 4 ) | ( fs_bits << 2);
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

int LSM6DS3_XL_ODR_CONFIG(I2C_HandleTypeDef hi2c_def){//ustawienia filtra
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

float LSM6DS3_read_XL_FS(I2C_HandleTypeDef hi2c_def) {
    uint8_t ctrl1_xl_fs_value;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_def, LSM6DS3_ADDRESS << 1, CTRL1_XL,
                                                I2C_MEMADD_SIZE_8BIT, &ctrl1_xl_fs_value, 1, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        return 0.0f;
    }
    uint8_t odr_bits = (ctrl1_xl_fs_value >> 2) & 0x03;
    switch (odr_bits) {
    case 0x00: return 2.0f;   //2g
            case 0x01: return 16.0f;  //16g
            case 0x02: return 4.0f;   //4g
            case 0x03: return 8.0f;   // 8g
            default:   return 0.0f;
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

LSM6DS3_AccelData LSM6DS3_read_acceleration(I2C_HandleTypeDef hi2c_def, float fs_g){
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
	float sensitivity_mg_per_lsb;

	    if (fs_g <= 2.0f) {
	        sensitivity_mg_per_lsb = 0.061f;
	    } else if (fs_g <= 4.0f) {
	        sensitivity_mg_per_lsb = 0.122f;
	    } else if (fs_g <= 8.0f) {
	        sensitivity_mg_per_lsb = 0.244f;
	    } else {
	        sensitivity_mg_per_lsb = 0.488f;
	    }
	    float sensitivity_g_per_lsb = sensitivity_mg_per_lsb/1000.0f;
	    accel_data[0] = (int16_t)((uint16_t)raw_data[1] << 8 | raw_data[0]);
	    accel_data[1] = (int16_t)((uint16_t)raw_data[3] << 8 | raw_data[2]);
	    accel_data[2] = (int16_t)((uint16_t)raw_data[5] << 8 | raw_data[4]);

	    result.x = accel_data[0] * sensitivity_g_per_lsb;
	    result.y = accel_data[1] * sensitivity_g_per_lsb;
	    result.z = accel_data[2] * sensitivity_g_per_lsb;


	 result.status = 1;

	 return result;
 }

int LSM6DS3_enable_INT1_DRDY_XL(I2C_HandleTypeDef hi2c_def){
	uint8_t int1_ctrl_val;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_def, LSM6DS3_ADDRESS << 1, INT1_CTRL,
													I2C_MEMADD_SIZE_8BIT, &int1_ctrl_val, 1, HAL_MAX_DELAY);
	if (status != HAL_OK) {
	        return 1;
	    }
	int1_ctrl_val |= (1 << 5);
	status = HAL_I2C_Mem_Write(&hi2c_def, LSM6DS3_ADDRESS << 1, INT1_CTRL,
												   I2C_MEMADD_SIZE_8BIT, &int1_ctrl_val, 1, HAL_MAX_DELAY);
	    if (status != HAL_OK) {
	        return 2;
	    }

	    return 0;
}

int LSM6DS3_initialize(I2C_HandleTypeDef hi2c_def, float odr_hz, float fs_g) {
    int status = 0;
    if (LSM6DS3_check_who_am_i(hi2c_def) != LSM6DS3_WHO_AM_I) {
        return 1;
    }
    status |= LSM6DS3_XL_CONFIG(hi2c_def, odr_hz, fs_g);
    status |= LSM6DS3_G_CONFIG(hi2c_def);
    status |= LSM6DS3_XL_ODR_CONFIG(hi2c_def);
    status |= LSM6DS3_enable_INT1_DRDY_XL(hi2c_def);

    return status;
}
