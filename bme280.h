/*
 * bme280.h
 *
 *  Created on: Jun 3, 2026
 *      Author: Eylül Öztek
 */

#ifndef BME280_H_
#define BME280_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* BME280 Register Addresses */
#define BME280_REG_ID                  0xD0
#define BME280_REG_RESET               0xE0
#define BME280_REG_CTRL_HUM            0xF2
#define BME280_REG_STATUS              0xF3
#define BME280_REG_CTRL_MEAS           0xF4
#define BME280_REG_CONFIG              0xF5
#define BME280_REG_PRESS_MSB           0xF7
#define BME280_REG_PRESS_LSB           0xF8
#define BME280_REG_PRESS_XLSB          0xF9
#define BME280_REG_TEMP_MSB            0xFA
#define BME280_REG_TEMP_LSB            0xFB
#define BME280_REG_TEMP_XLSB           0xFC
#define BME280_REG_HUM_MSB             0xFD
#define BME280_REG_HUM_LSB             0xFE

/* Calibration Register Start Addresses */
#define BME280_REG_CALIB00             0x88
#define BME280_REG_CALIB26             0xE1

/* Device Constants */
#define BME280_CHIP_ID                 0x60
#define BME280_RESET_VALUE             0xB6
#define BME280_TIMEOUT                 1000U

/*
 * STM32 HAL expects the I2C address shifted left by 1.
 *
 * SDO = GND   -> 7-bit address 0x76 -> HAL address 0xEC
 * SDO = VDDIO -> 7-bit address 0x77 -> HAL address 0xEE
 */
#define BME280_I2C_ADDR_GND            (0x76 << 1)
#define BME280_I2C_ADDR_VDDIO          (0x77 << 1)

/* Status Register Bits */
#define BME280_STATUS_IM_UPDATE        0x01
#define BME280_STATUS_MEASURING        0x08

/* ctrl_hum Register: osrs_h[2:0] */
#define BME280_OVERSAMPLING_SKIPPED    0x00
#define BME280_OVERSAMPLING_1          0x01
#define BME280_OVERSAMPLING_2          0x02
#define BME280_OVERSAMPLING_4          0x03
#define BME280_OVERSAMPLING_8          0x04
#define BME280_OVERSAMPLING_16         0x05

/* ctrl_meas Register: mode[1:0] */
#define BME280_SLEEP_MODE              0x00
#define BME280_FORCED_MODE             0x01
#define BME280_NORMAL_MODE             0x03

/* config Register: filter[4:2] */
#define BME280_FILTER_OFF              0x00
#define BME280_FILTER_2                0x01
#define BME280_FILTER_4                0x02
#define BME280_FILTER_8                0x03
#define BME280_FILTER_16               0x04

/*
 * config Register: t_sb[7:5]
 *
 * Note:
 * For BME280, 0x06 and 0x07 correspond to 10 ms and 20 ms.
 * They are not 2000 ms and 4000 ms. That mapping belongs to BMP280.
 */
#define BME280_STANDBY_TIME_0_5        0x00
#define BME280_STANDBY_TIME_62_5       0x01
#define BME280_STANDBY_TIME_125        0x02
#define BME280_STANDBY_TIME_250        0x03
#define BME280_STANDBY_TIME_500        0x04
#define BME280_STANDBY_TIME_1000       0x05
#define BME280_STANDBY_TIME_10         0x06
#define BME280_STANDBY_TIME_20         0x07

/* Bit Positions */
#define BME280_CTRL_MEAS_OSRS_T_POS    5
#define BME280_CTRL_MEAS_OSRS_P_POS    2
#define BME280_CTRL_MEAS_MODE_POS      0

#define BME280_CONFIG_T_SB_POS         5
#define BME280_CONFIG_FILTER_POS       2
#define BME280_CONFIG_SPI3W_EN_POS     0

typedef enum {
	BME280_OK = 0, /**< Operation completed successfully. */
	BME280_ERROR, /**< HAL communication or generic driver error. */
	BME280_INVALID_ID, /**< Device ID does not match BME280_CHIP_ID. */
	BME280_INVALID_PARAM, /**< Invalid function parameter. */
	BME280_TIMEOUT_ERROR /**< Device did not become ready in expected time. */
} BME280_Status_t;

typedef struct {
	I2C_HandleTypeDef *hi2c; /**< Pointer to STM32 HAL I2C handle. */
	uint16_t address; /**< Shifted I2C device address for STM32 HAL. */
} BME280_Handle_t;

typedef struct {
	uint8_t osrs_h; /**< Humidity oversampling setting. */
	uint8_t osrs_t; /**< Temperature oversampling setting. */
	uint8_t osrs_p; /**< Pressure oversampling setting. */
	uint8_t mode; /**< Sensor mode: sleep, forced or normal. */
	uint8_t standby_time; /**< Standby time for normal mode. */
	uint8_t filter; /**< IIR filter coefficient. */
} BME280_Config_t;

/**
 * @brief Initialize BME280 sensor with default configuration.
 *
 * Default configuration:
 * - Humidity oversampling: x4
 * - Temperature oversampling: x1
 * - Pressure oversampling: x4
 * - Mode: normal mode
 * - Standby time: 1000 ms
 * - IIR filter: coefficient 4
 *
 * @param dev Pointer to BME280 handle.
 * @param hi2c Pointer to STM32 HAL I2C handle.
 * @param address Shifted I2C address for STM32 HAL.
 * @return BME280_Status_t Operation status.
 */
BME280_Status_t BME280_Init(BME280_Handle_t *dev, I2C_HandleTypeDef *hi2c,
		uint16_t address);

/**
 * @brief Initialize BME280 sensor with user-defined configuration.
 *
 * @param dev Pointer to BME280 handle.
 * @param hi2c Pointer to STM32 HAL I2C handle.
 * @param address Shifted I2C address for STM32 HAL.
 * @param config Pointer to BME280 configuration structure.
 * @return BME280_Status_t Operation status.
 */
BME280_Status_t BME280_InitWithConfig(BME280_Handle_t *dev,
		I2C_HandleTypeDef *hi2c, uint16_t address,
		const BME280_Config_t *config);

/**
 * @brief Check if BME280 device is ready on I2C bus.
 *
 * @param dev Pointer to BME280 handle.
 * @return BME280_Status_t Operation status.
 */
BME280_Status_t BME280_IsDeviceReady(BME280_Handle_t *dev);

/**
 * @brief Read BME280 chip ID.
 *
 * @param dev Pointer to BME280 handle.
 * @param deviceID Pointer to variable where device ID will be stored.
 * @return BME280_Status_t Operation status.
 */
BME280_Status_t BME280_ReadDeviceID(BME280_Handle_t *dev, uint8_t *deviceID);

/**
 * @brief Read data from BME280 register.
 *
 * @param dev Pointer to BME280 handle.
 * @param reg Register address.
 * @param data Pointer to data buffer.
 * @param length Number of bytes to read.
 * @return BME280_Status_t Operation status.
 */
BME280_Status_t BME280_ReadRegister(BME280_Handle_t *dev, uint8_t reg,
		uint8_t *data, uint16_t length);

/**
 * @brief Write one byte to BME280 register.
 *
 * @param dev Pointer to BME280 handle.
 * @param reg Register address.
 * @param value Value to write.
 * @return BME280_Status_t Operation status.
 */
BME280_Status_t BME280_WriteRegister(BME280_Handle_t *dev, uint8_t reg,
		uint8_t value);

/**
 * @brief Apply BME280 sensor configuration.
 *
 * @param dev Pointer to BME280 handle.
 * @param config Pointer to BME280 configuration structure.
 * @return BME280_Status_t Operation status.
 */
BME280_Status_t BME280_SetConfig(BME280_Handle_t *dev,
		const BME280_Config_t *config);

/**
 * @brief Perform soft reset.
 *
 * @param dev Pointer to BME280 handle.
 * @return BME280_Status_t Operation status.
 */
BME280_Status_t BME280_SoftReset(BME280_Handle_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* BME280_H_ */
