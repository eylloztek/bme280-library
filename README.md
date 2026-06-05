# STM32 BME280 Driver

A lightweight STM32 HAL-based driver for the Bosch BME280 environmental sensor.

This driver communicates with the BME280 over I2C and provides temperature, pressure, humidity, and altitude calculation support. It is designed for STM32 projects using the HAL library. You can access the datasheet from [here.](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf)

## Features

* I2C communication using STM32 HAL
* BME280 chip ID verification
* Soft reset support
* Factory calibration data reading
* Temperature, pressure, and humidity compensation
* Normal mode support
* Forced mode support
* Raw ADC data reading
* Altitude calculation from pressure
* Default configuration helper function
* Basic parameter validation

## Supported Sensor

This driver is intended for the **Bosch BME280** sensor.

The expected chip ID is:

```c
0x60
```

## Files

```text
bme280.h
bme280.c
```

Add both files to your STM32CubeIDE project.

## Hardware Connection

The BME280 can operate over I2C or SPI. This driver supports **I2C only**.

Typical I2C wiring:

| BME280 Pin | STM32 Pin   | Description               |
| ---------- | ----------- | ------------------------- |
| VCC / VIN  | 3.3V        | Power supply              |
| GND        | GND         | Ground                    |
| SCL        | I2C SCL     | I2C clock                 |
| SDA        | I2C SDA     | I2C data                  |
| CSB / CS   | 3.3V        | Must be HIGH for I2C mode |
| SDO        | GND or 3.3V | I2C address selection     |

## I2C Address Selection

The BME280 I2C address depends on the SDO pin:

| SDO Connection | 7-bit Address | STM32 HAL Address |
| -------------- | ------------: | ----------------: |
| SDO → GND      |        `0x76` |            `0xEC` |
| SDO → 3.3V     |        `0x77` |            `0xEE` |

This driver expects **STM32 HAL shifted addresses**.

Use:

```c
BME280_I2C_ADDR_GND
```

or:

```c
BME280_I2C_ADDR_VDDIO
```

Do not pass raw addresses such as `0x76` or `0x77` directly to `BME280_Init()`.

Correct:

```c
BME280_Init(&bme280, &hi2c1, BME280_I2C_ADDR_GND);
```

Incorrect:

```c
BME280_Init(&bme280, &hi2c1, 0x76);
```

## STM32CubeIDE Configuration

Enable an I2C peripheral in STM32CubeMX or STM32CubeIDE.

Recommended initial settings:

| Setting         | Value                                |
| --------------- | ------------------------------------ |
| I2C Speed       | 100 kHz                              |
| GPIO Mode       | Alternate Function Open Drain        |
| Pull-up         | External pull-up or internal pull-up |
| Addressing Mode | 7-bit                                |

If your BME280 breakout board does not include pull-up resistors, add external pull-ups:

```text
SDA -> 4.7kΩ -> 3.3V
SCL -> 4.7kΩ -> 3.3V
```

## Include the Driver

In your `main.c`:

```c
#include "bme280.h"
```

Create a BME280 handle:

```c
BME280_Handle_t bme280;
BME280_Data_t bme280_data;
BME280_Status_t bme280_status;
```

## Basic Usage

Initialize the sensor with default settings:

```c
bme280_status = BME280_Init(&bme280, &hi2c1, BME280_I2C_ADDR_GND);

if (bme280_status != BME280_OK)
{
    Error_Handler();
}
```

Read sensor data:

```c
bme280_status = BME280_ReadSensorData(&bme280, &bme280_data);

if (bme280_status == BME280_OK)
{
    float temperature = bme280_data.temperature;
    float pressure = bme280_data.pressure;
    float humidity = bme280_data.humidity;
}
```

Returned units:

| Field         | Unit |
| ------------- | ---- |
| `temperature` | °C   |
| `pressure`    | hPa  |
| `humidity`    | %RH  |

## Default Configuration

The default configuration is:

| Parameter                | Value       |
| ------------------------ | ----------- |
| Humidity oversampling    | x4          |
| Temperature oversampling | x1          |
| Pressure oversampling    | x4          |
| Mode                     | Normal mode |
| Standby time             | 1000 ms     |
| IIR filter               | 4           |

You can get the default configuration manually:

```c
BME280_Config_t config;

BME280_GetDefaultConfig(&config);
```

## Custom Configuration

Example:

```c
BME280_Config_t config;

BME280_GetDefaultConfig(&config);

config.osrs_h = BME280_OVERSAMPLING_4;
config.osrs_t = BME280_OVERSAMPLING_1;
config.osrs_p = BME280_OVERSAMPLING_4;
config.mode = BME280_NORMAL_MODE;
config.standby_time = BME280_STANDBY_TIME_1000;
config.filter = BME280_FILTER_4;

bme280_status = BME280_InitWithConfig(&bme280,
                                      &hi2c1,
                                      BME280_I2C_ADDR_GND,
                                      &config);

if (bme280_status != BME280_OK)
{
    Error_Handler();
}
```

## Normal Mode Example

In normal mode, the BME280 performs periodic measurements automatically.

```c
BME280_Handle_t bme280;
BME280_Data_t data;
BME280_Status_t status;

status = BME280_Init(&bme280, &hi2c1, BME280_I2C_ADDR_GND);

if (status != BME280_OK)
{
    Error_Handler();
}

while (1)
{
    status = BME280_ReadSensorData(&bme280, &data);

    if (status == BME280_OK)
    {
        float temperature = data.temperature;
        float pressure = data.pressure;
        float humidity = data.humidity;
    }

    HAL_Delay(1000);
}
```

## Forced Mode Example

In forced mode, the sensor performs one measurement and then returns to sleep mode.

This driver automatically triggers a new forced measurement inside `BME280_ReadSensorData()` when the device is configured in forced mode.

```c
BME280_Handle_t bme280;
BME280_Config_t config;
BME280_Data_t data;
BME280_Status_t status;

status = BME280_GetDefaultConfig(&config);

if (status != BME280_OK)
{
    Error_Handler();
}

config.mode = BME280_FORCED_MODE;

status = BME280_InitWithConfig(&bme280,
                              &hi2c1,
                              BME280_I2C_ADDR_GND,
                              &config);

if (status != BME280_OK)
{
    Error_Handler();
}

while (1)
{
    status = BME280_ReadSensorData(&bme280, &data);

    if (status == BME280_OK)
    {
        float temperature = data.temperature;
        float pressure = data.pressure;
        float humidity = data.humidity;
    }

    HAL_Delay(1000);
}
```

## Reading Raw ADC Data

You can also read raw uncompensated ADC values:

```c
BME280_RawData_t raw;

if (BME280_ReadRawData(&bme280, &raw) == BME280_OK)
{
    int32_t raw_temperature = raw.adc_temperature;
    int32_t raw_pressure = raw.adc_pressure;
    int32_t raw_humidity = raw.adc_humidity;
}
```

Raw ADC values are not physical sensor values. Use `BME280_ReadSensorData()` for compensated values.

## Altitude Calculation

The driver provides a helper function for altitude calculation:

```c
float altitude;

if (BME280_CalculateAltitude(data.pressure, 1013.25f, &altitude) == BME280_OK)
{
    float altitude_m = altitude;
}
```

The second parameter is the sea-level reference pressure in hPa.

Standard value:

```c
1013.25f
```

For better accuracy, use the current local sea-level pressure value.

## Public API

### Initialization

```c
BME280_Status_t BME280_Init(BME280_Handle_t *dev,
                            I2C_HandleTypeDef *hi2c,
                            uint16_t address);
```

Initializes the sensor with default configuration.

```c
BME280_Status_t BME280_InitWithConfig(BME280_Handle_t *dev,
                                      I2C_HandleTypeDef *hi2c,
                                      uint16_t address,
                                      const BME280_Config_t *config);
```

Initializes the sensor with a user-defined configuration.

### Configuration

```c
BME280_Status_t BME280_GetDefaultConfig(BME280_Config_t *config);
```

Fills a configuration structure with default settings.

```c
BME280_Status_t BME280_SetConfig(BME280_Handle_t *dev,
                                 const BME280_Config_t *config);
```

Applies a new configuration to the sensor.

```c
BME280_Status_t BME280_Sleep(BME280_Handle_t *dev);
```

Places the sensor into sleep mode.

### Measurement

```c
BME280_Status_t BME280_ReadSensorData(BME280_Handle_t *dev,
                                      BME280_Data_t *data);
```

Reads compensated temperature, pressure, and humidity values.

```c
BME280_Status_t BME280_ReadRawData(BME280_Handle_t *dev,
                                   BME280_RawData_t *raw);
```

Reads raw uncompensated ADC values.

```c
BME280_Status_t BME280_TriggerForcedMeasurement(BME280_Handle_t *dev);
```

Triggers one forced-mode measurement.

```c
BME280_Status_t BME280_WaitMeasurementComplete(BME280_Handle_t *dev);
```

Waits until the current measurement is complete.

### Device Control

```c
BME280_Status_t BME280_ReadDeviceID(BME280_Handle_t *dev,
                                    uint8_t *device_id);
```

Reads the chip ID register.

```c
BME280_Status_t BME280_SoftReset(BME280_Handle_t *dev);
```

Performs a software reset.

### Utility

```c
BME280_Status_t BME280_CalculateAltitude(float pressure_hpa,
                                         float sea_level_hpa,
                                         float *altitude_m);
```

Calculates altitude from pressure.

## Status Codes

| Status                 | Description                                  |
| ---------------------- | -------------------------------------------- |
| `BME280_OK`            | Operation completed successfully             |
| `BME280_ERROR`         | HAL communication or generic driver error    |
| `BME280_INVALID_ID`    | Read chip ID does not match BME280           |
| `BME280_INVALID_PARAM` | Invalid function parameter                   |
| `BME280_TIMEOUT_ERROR` | Device did not become ready in expected time |

## Notes About Oversampling

This driver reads compensated temperature, pressure, and humidity together.

For this reason, the current implementation rejects:

```c
BME280_OVERSAMPLING_SKIPPED
```

for:

```c
osrs_t
osrs_p
osrs_h
```

All three measurement channels must be enabled when using `BME280_ReadSensorData()`.

## Math Library Note

`BME280_CalculateAltitude()` uses `powf()` from `math.h`.

If you get a linker error such as:

```text
undefined reference to `powf`
```

link the math library.

For GCC-based toolchains, add:

```text
-lm
```

In STM32CubeIDE, this can usually be added from:

```text
Project Properties
C/C++ Build
Settings
MCU GCC Linker
Libraries
```

Add:

```text
m
```

## Troubleshooting

### Device is not detected

Check the following:

* VCC is connected to 3.3V
* GND is connected correctly
* SDA and SCL are connected to the correct STM32 I2C pins
* CSB / CS is connected to 3.3V for I2C mode
* SDO is connected to GND or 3.3V
* The correct address is used
* SDA and SCL have pull-up resistors
* I2C peripheral is enabled in STM32CubeIDE
* I2C speed is set to 100 kHz for initial testing

### `BME280_ERROR` during initialization

This usually means I2C communication failed.

Possible causes:

* Wrong I2C address
* Missing pull-up resistors
* Wrong SDA/SCL pins
* CSB pin is floating or LOW
* Poor breadboard or jumper cable contact
* Sensor is not powered correctly

### `BME280_INVALID_ID`

The device responded over I2C, but the chip ID was not `0x60`.

Possible causes:

* Wrong sensor
* Wrong I2C device on the bus
* Communication issue
* BMP280 module instead of BME280

### `BME280_INVALID_PARAM`

Check that:

* The handle pointer is not NULL
* The I2C handle pointer is not NULL
* Output pointers are not NULL
* The selected address is `BME280_I2C_ADDR_GND` or `BME280_I2C_ADDR_VDDIO`
* Configuration values are valid
* Oversampling values are not set to `BME280_OVERSAMPLING_SKIPPED`

## Example Minimal Code

```c
#include "main.h"
#include "bme280.h"

I2C_HandleTypeDef hi2c1;

BME280_Handle_t bme280;
BME280_Data_t data;
BME280_Status_t status;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();

    status = BME280_Init(&bme280, &hi2c1, BME280_I2C_ADDR_GND);

    if (status != BME280_OK)
    {
        Error_Handler();
    }

    while (1)
    {
        status = BME280_ReadSensorData(&bme280, &data);

        if (status == BME280_OK)
        {
            float temperature = data.temperature;
            float pressure = data.pressure;
            float humidity = data.humidity;
        }

        HAL_Delay(1000);
    }
}
```

## License

This project is licensed under the MIT License. See the [LICENSE](https://github.com/eylloztek/bme280-library/blob/master/LICENSE.txt) file for details.
