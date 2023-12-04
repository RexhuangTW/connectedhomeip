/**************************************************************************//**
 * @file     eeprom_ctrl.c
 * @version
 * @brief    eeprom control interface

 ******************************************************************************/

/*******************************************************************************
*   INCLUDES
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "i2c_master.h"
#include "eeprom_ctrl.h"

/******************************************************************************
* DEFINES
******************************************************************************/
#define EEPROM_I2C_BUSY     0xff

#define EEPROM_DEBUG        FALSE

/******************************************************************************
* GLOBAL PARAMETERS
******************************************************************************/
volatile uint32_t   g_eeprom_i2c_status;

/******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************/
static void eeprom_i2c_cmd_done_cb(uint32_t status)
{
    g_eeprom_i2c_status = status;
}

uint32_t eeprom_read(
    eeprom_cfg_t *cfg,
    uint8_t reg_addr,
    uint8_t  *data,
    uint32_t len
)
{
    i2c_slave_data_t  dev_cfg;
    uint32_t   count, status;

    if ((reg_addr + len) > cfg->max_size)
    {
#if (EEPROM_DEBUG == TRUE)
        printf("[ERR] Invalid parameters: 0x%02x, %d\n", reg_addr, len);
#endif

        goto DONE;
    }

    g_eeprom_i2c_status = EEPROM_I2C_BUSY;

    dev_cfg.dev_addr = cfg->dev_addr;
    dev_cfg.reg_addr = reg_addr;
    dev_cfg.bFlag_16bits = 0;
    status = i2c_read( &dev_cfg, data, len, eeprom_i2c_cmd_done_cb);

    if (status != STATUS_SUCCESS)
    {
#if (EEPROM_DEBUG == TRUE)
        printf("[ERR] i2c_read fail: %d\n", status);
#endif
        goto DONE;
    }

    count = 1000000;        // TODO: the count should be calculated by len and clock
    while (g_eeprom_i2c_status == EEPROM_I2C_BUSY)      /*Busy waiting I2C complete*/
    {
        if (--count == 0)
        {
            status = STATUS_TIMEOUT;

            goto DONE;
        }
    }

    if (g_eeprom_i2c_status == I2C_STATUS_ERR_NOACK)
    {
#if (EEPROM_DEBUG == TRUE)
        printf("[ERR] No device: 0x%x\n", cfg->dev_addr);
#endif
        status = STATUS_ERROR;

        goto DONE;
    }

    status = STATUS_SUCCESS;

DONE:
#if (EEPROM_DEBUG == TRUE)
    if (status != STATUS_SUCCESS)
    {
        printf("[ERR] Error status: %d\n", status);
    }
#endif

    return status;
}

uint32_t eeprom_write(
    eeprom_cfg_t *cfg,
    uint8_t reg_addr,
    uint8_t  *data,
    uint32_t len
)
{
    i2c_slave_data_t  dev_cfg;
    uint32_t   count, status;
    uint32_t len_curr;

    if ((reg_addr + len) > cfg->max_size)
    {
#if (EEPROM_DEBUG == TRUE)
        printf("[ERR] Invalid parameters: 0x%02x, %d\n", reg_addr, len);
#endif
        status = STATUS_INVALID_PARAM;

        goto DONE;
    }

    dev_cfg.dev_addr = cfg->dev_addr;
    dev_cfg.bFlag_16bits = 0;

    len_curr = (((reg_addr & cfg->page_size_mask) + len) > cfg->page_size) ? cfg->page_size - (reg_addr & cfg->page_size_mask) : len;
    do
    {
        g_eeprom_i2c_status = EEPROM_I2C_BUSY;

        dev_cfg.reg_addr = reg_addr;
        status = i2c_write( &dev_cfg, data, len_curr, eeprom_i2c_cmd_done_cb);

        if (status != STATUS_SUCCESS)
        {
#if (EEPROM_DEBUG == TRUE)
            printf("[ERR] i2c_write fail: %d\n", status);
#endif
            goto DONE;
        }

        count = 1000000;        // TODO: the count should be calculated by len and clock
        while (g_eeprom_i2c_status == EEPROM_I2C_BUSY)      /*Busy waiting I2C complete*/
        {
            if (--count == 0)
            {
                status = STATUS_TIMEOUT;

                goto DONE;
            }
        }

        if (g_eeprom_i2c_status == I2C_STATUS_ERR_NOACK)
        {
#if (EEPROM_DEBUG == TRUE)
            printf("[ERR] No device: 0x%x\n", cfg->dev_addr);
#endif
            status = STATUS_ERROR;

            goto DONE;
        }

        /* Wait for writing complete */
        Delay_ms(cfg->write_cycle);

        reg_addr += len_curr;
        data += len_curr;
        len -= len_curr;
        len_curr = (len >= cfg->page_size) ? cfg->page_size : len;
    } while (len);

    status = STATUS_SUCCESS;

DONE:
#if (EEPROM_DEBUG == TRUE)
    if (status != STATUS_SUCCESS)
    {
        printf("[ERR] Error status: %d\n", status);
    }
#endif

    return status;
}

uint32_t eeprom_init(
    eeprom_cfg_t *cfg,
    uint8_t dev_addr,
    uint16_t page_size,
    uint16_t max_size,
    uint8_t write_cycle,
    uint32_t scl_pin,
    uint32_t sda_pin,
    uint32_t i2c_speed
)
{
    uint32_t status;

    status = i2c_preinit(scl_pin, sda_pin);
    if (status != STATUS_SUCCESS)
    {
#if (EEPROM_DEBUG == TRUE)
        printf("Wrong pin config: scl(%d) sda(%d)\n", scl_pin, sda_pin);
#endif

        goto DONE;
    }

    i2c_init(i2c_speed);

    cfg->dev_addr = dev_addr;
    cfg->page_size = page_size;
    cfg->page_size_mask = page_size - 1;
    cfg->max_size = max_size;
    cfg->write_cycle = write_cycle;

    status = STATUS_SUCCESS;

DONE:
    return status;
}

