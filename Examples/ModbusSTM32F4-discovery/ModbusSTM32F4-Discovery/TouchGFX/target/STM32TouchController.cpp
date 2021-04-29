/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* USER CODE BEGIN STM32TouchController */
#include <STM32TouchController.hpp>
#include "Components/stmpe811/stmpe811.h"

#define TS_I2C_ADDRESS                      0x82

static TS_DrvTypeDef*     TsDrv;
static uint16_t          TsXBoundary, TsYBoundary;

typedef struct
{
    uint16_t TouchDetected;
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
} TS_StateTypeDef;

typedef enum
{
    TS_OK       = 0x00,
    TS_ERROR    = 0x01,
    TS_TIMEOUT  = 0x02
} TS_StatusTypeDef;

uint8_t BSP_TS_Init(uint16_t XSize, uint16_t YSize);
void    BSP_TS_GetState(TS_StateTypeDef* TsState);

void STM32TouchController::init()
{
    /**
     * Initialize touch controller and driver
     *
     */
    BSP_TS_Init(240, 320);
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    /**
     * By default sampleTouch returns false,
     * return true if a touch has been detected, otherwise false.
     *
     * Coordinates are passed to the caller by reference by x and y.
     *
     * This function is called by the TouchGFX framework.
     * By default sampleTouch is called every tick, this can be adjusted by HAL::setTouchSampleRate(int8_t);
     *
     */
    TS_StateTypeDef state;
    BSP_TS_GetState(&state);
    if (state.TouchDetected)
    {
        x = state.X;
        y = state.Y;
        return true;
    }
    return false;
}

/**
  * @brief  Initializes and configures the touch screen functionalities and
  *         configures all necessary hardware resources (GPIOs, clocks..).
  * @param  XSize: The maximum X size of the TS area on LCD
  * @param  YSize: The maximum Y size of the TS area on LCD
  * @retval TS_OK: if all initializations are OK. Other value if error.
  */
uint8_t BSP_TS_Init(uint16_t XSize, uint16_t YSize)
{
    uint8_t ret = TS_ERROR;

    /* Initialize x and y positions boundaries */
    TsXBoundary = XSize;
    TsYBoundary = YSize;

    /* Read ID and verify if the IO expander is ready */
    if (stmpe811_ts_drv.ReadID(TS_I2C_ADDRESS) == STMPE811_ID)
    {
        /* Initialize the TS driver structure */
        TsDrv = &stmpe811_ts_drv;

        ret = TS_OK;
    }

    if (ret == TS_OK)
    {
        /* Initialize the LL TS Driver */
        TsDrv->Init(TS_I2C_ADDRESS);
        TsDrv->Start(TS_I2C_ADDRESS);
    }

    return ret;
}

/**
  * @brief  Returns status and positions of the touch screen.
  * @param  TsState: Pointer to touch screen current state structure
  */
void BSP_TS_GetState(TS_StateTypeDef* TsState)
{
    static uint32_t _x = 0, _y = 0;
    uint16_t xDiff, yDiff, x, y, xr, yr;

    TsState->TouchDetected = TsDrv->DetectTouch(TS_I2C_ADDRESS);

    if (TsState->TouchDetected)
    {
        TsDrv->GetXY(TS_I2C_ADDRESS, &x, &y);

        /* Y value first correction */
        y -= 360;

        /* Y value second correction */
        yr = y / 11;

        /* Return y position value */
        if (yr <= 0)
        {
            yr = 0;
        }
        else if (yr > TsYBoundary)
        {
            yr = TsYBoundary - 1;
        }
        else
        {}
        y = yr;

        /* X value first correction */
        if (x <= 3000)
        {
            x = 3870 - x;
        }
        else
        {
            x = 3800 - x;
        }

        /* X value second correction */
        xr = x / 15;

        /* Return X position value */
        if (xr <= 0)
        {
            xr = 0;
        }
        else if (xr > TsXBoundary)
        {
            xr = TsXBoundary - 1;
        }
        else
        {}

        x = xr;
        xDiff = x > _x ? (x - _x) : (_x - x);
        yDiff = y > _y ? (y - _y) : (_y - y);

        if (xDiff + yDiff > 5)
        {
            _x = x;
            _y = y;
        }

        /* Update the X position */
        TsState->X = _x;

        /* Update the Y position */
        TsState->Y = _y;
    }
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
