/*
 * bme280.c
 *
 *  Created on: Jun 3, 2026
 *      Author: Eylül Öztek
 */

#include "bme280.h"

static BME280_Status_t BME280_WaitNVMReady(BME280_Handle_t *dev);
static BME280_Status_t BME280_ValidateConfig(const BME280_Config_t *config);

BME280_Status_t BME280_ReadRegister(BME280_Handle_t *dev,
                                    uint8_t reg,
                                    uint8_t *data,
                                    uint16_t length)
{
    if (dev == NULL || dev->hi2c == NULL || data == NULL || length == 0) {
        return BME280_INVALID_PARAM;
    }

    if (HAL_I2C_Mem_Read(dev->hi2c,
                         dev->address,
                         reg,
                         I2C_MEMADD_SIZE_8BIT,
                         data,
                         length,
                         BME280_TIMEOUT) != HAL_OK) {
        return BME280_ERROR;
    }

    return BME280_OK;
}

BME280_Status_t BME280_WriteRegister(BME280_Handle_t *dev,
                                     uint8_t reg,
                                     uint8_t value)
{
    if (dev == NULL || dev->hi2c == NULL) {
        return BME280_INVALID_PARAM;
    }

    if (HAL_I2C_Mem_Write(dev->hi2c,
                          dev->address,
                          reg,
                          I2C_MEMADD_SIZE_8BIT,
                          &value,
                          1,
                          BME280_TIMEOUT) != HAL_OK) {
        return BME280_ERROR;
    }

    return BME280_OK;
}

BME280_Status_t BME280_IsDeviceReady(BME280_Handle_t *dev)
{
    if (dev == NULL || dev->hi2c == NULL) {
        return BME280_INVALID_PARAM;
    }

    if (HAL_I2C_IsDeviceReady(dev->hi2c,
                              dev->address,
                              3,
                              BME280_TIMEOUT) != HAL_OK) {
        return BME280_ERROR;
    }

    return BME280_OK;
}

BME280_Status_t BME280_ReadDeviceID(BME280_Handle_t *dev, uint8_t *deviceID)
{
    if (dev == NULL || deviceID == NULL) {
        return BME280_INVALID_PARAM;
    }

    return BME280_ReadRegister(dev, BME280_REG_ID, deviceID, 1);
}

BME280_Status_t BME280_SoftReset(BME280_Handle_t *dev)
{
    BME280_Status_t status;

    if (dev == NULL || dev->hi2c == NULL) {
        return BME280_INVALID_PARAM;
    }

    status = BME280_WriteRegister(dev, BME280_REG_RESET, BME280_RESET_VALUE);
    if (status != BME280_OK) {
        return status;
    }

    /*
     * After soft reset, the device reloads calibration data from NVM.
     * A short delay prevents immediate communication problems.
     */
    HAL_Delay(2);

    return BME280_WaitNVMReady(dev);
}

BME280_Status_t BME280_Init(BME280_Handle_t *dev,
                            I2C_HandleTypeDef *hi2c,
                            uint16_t address)
{
    BME280_Config_t defaultConfig = {
        .osrs_h = BME280_OVERSAMPLING_4,
        .osrs_t = BME280_OVERSAMPLING_1,
        .osrs_p = BME280_OVERSAMPLING_4,
        .mode = BME280_NORMAL_MODE,
        .standby_time = BME280_STANDBY_TIME_1000,
        .filter = BME280_FILTER_4
    };

    return BME280_InitWithConfig(dev, hi2c, address, &defaultConfig);
}

BME280_Status_t BME280_InitWithConfig(BME280_Handle_t *dev,
                                      I2C_HandleTypeDef *hi2c,
                                      uint16_t address,
                                      const BME280_Config_t *config)
{
    BME280_Status_t status;
    uint8_t deviceID = 0;

    if (dev == NULL || hi2c == NULL || config == NULL) {
        return BME280_INVALID_PARAM;
    }

    if (address != BME280_I2C_ADDR_GND &&
        address != BME280_I2C_ADDR_VDDIO) {
        return BME280_INVALID_PARAM;
    }

    status = BME280_ValidateConfig(config);
    if (status != BME280_OK) {
        return status;
    }

    dev->hi2c = hi2c;
    dev->address = address;

    status = BME280_IsDeviceReady(dev);
    if (status != BME280_OK) {
        return status;
    }

    status = BME280_SoftReset(dev);
    if (status != BME280_OK) {
        return status;
    }

    status = BME280_ReadDeviceID(dev, &deviceID);
    if (status != BME280_OK) {
        return status;
    }

    if (deviceID != BME280_CHIP_ID) {
        return BME280_INVALID_ID;
    }

    status = BME280_SetConfig(dev, config);
    if (status != BME280_OK) {
        return status;
    }

    return BME280_OK;
}

BME280_Status_t BME280_SetConfig(BME280_Handle_t *dev,
                                 const BME280_Config_t *config)
{
    BME280_Status_t status;
    uint8_t ctrlHum;
    uint8_t ctrlMeas;
    uint8_t configReg;

    if (dev == NULL || dev->hi2c == NULL || config == NULL) {
        return BME280_INVALID_PARAM;
    }

    status = BME280_ValidateConfig(config);
    if (status != BME280_OK) {
        return status;
    }

    /*
     * ctrl_hum register:
     * bit 2:0 -> osrs_h
     */
    ctrlHum = config->osrs_h & 0x07;

    /*
     * config register:
     * bit 7:5 -> t_sb
     * bit 4:2 -> filter
     * bit 0   -> spi3w_en, kept disabled for I2C
     */
    configReg = ((config->standby_time & 0x07) << BME280_CONFIG_T_SB_POS) |
                ((config->filter       & 0x07) << BME280_CONFIG_FILTER_POS);

    /*
     * ctrl_meas register:
     * bit 7:5 -> osrs_t
     * bit 4:2 -> osrs_p
     * bit 1:0 -> mode
     */
    ctrlMeas = ((config->osrs_t & 0x07) << BME280_CTRL_MEAS_OSRS_T_POS) |
               ((config->osrs_p & 0x07) << BME280_CTRL_MEAS_OSRS_P_POS) |
               ((config->mode   & 0x03) << BME280_CTRL_MEAS_MODE_POS);

    /*
     * Important:
     * Humidity oversampling changes become effective after writing ctrl_meas.
     * Therefore, ctrl_hum must be written before ctrl_meas.
     */
    status = BME280_WriteRegister(dev, BME280_REG_CTRL_HUM, ctrlHum);
    if (status != BME280_OK) {
        return status;
    }

    status = BME280_WriteRegister(dev, BME280_REG_CONFIG, configReg);
    if (status != BME280_OK) {
        return status;
    }

    status = BME280_WriteRegister(dev, BME280_REG_CTRL_MEAS, ctrlMeas);
    if (status != BME280_OK) {
        return status;
    }

    return BME280_OK;
}

static BME280_Status_t BME280_WaitNVMReady(BME280_Handle_t *dev)
{
    uint8_t statusReg = 0;

    if (dev == NULL || dev->hi2c == NULL) {
        return BME280_INVALID_PARAM;
    }

    for (uint32_t i = 0; i < 100; i++) {
        if (BME280_ReadRegister(dev, BME280_REG_STATUS, &statusReg, 1) != BME280_OK) {
            return BME280_ERROR;
        }

        if ((statusReg & BME280_STATUS_IM_UPDATE) == 0) {
            return BME280_OK;
        }

        HAL_Delay(1);
    }

    return BME280_TIMEOUT_ERROR;
}

static BME280_Status_t BME280_ValidateConfig(const BME280_Config_t *config)
{
    if (config == NULL) {
        return BME280_INVALID_PARAM;
    }

    if (config->osrs_h > BME280_OVERSAMPLING_16 ||
        config->osrs_t > BME280_OVERSAMPLING_16 ||
        config->osrs_p > BME280_OVERSAMPLING_16) {
        return BME280_INVALID_PARAM;
    }

    if (config->mode != BME280_SLEEP_MODE &&
        config->mode != BME280_FORCED_MODE &&
        config->mode != BME280_NORMAL_MODE) {
        return BME280_INVALID_PARAM;
    }

    if (config->standby_time > BME280_STANDBY_TIME_20) {
        return BME280_INVALID_PARAM;
    }

    if (config->filter > BME280_FILTER_16) {
        return BME280_INVALID_PARAM;
    }

    return BME280_OK;
}
