/**************************************************************************//**
 * @file     eeprom_ctrl.h
 * @version
 * @brief    eeprom control interface

 ******************************************************************************/
#ifndef _EEPROM_CTRL_H
#define _EEPROM_CTRL_H

/*******************************************************************************
*   INCLUDES
*******************************************************************************/

/*******************************************************************************
*   CONSTANT AND DEFINE
*******************************************************************************/
typedef struct
{
    uint8_t     dev_addr;
    uint8_t     write_cycle;
    uint16_t    page_size;
    uint16_t    page_size_mask;
    uint32_t    max_size;
} eeprom_cfg_t;

/******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************/
uint32_t eeprom_read( eeprom_cfg_t *cfg, uint8_t reg_addr, uint8_t  *data, uint32_t len);
uint32_t eeprom_write( eeprom_cfg_t *cfg, uint8_t reg_addr, uint8_t  *data, uint32_t len);
uint32_t eeprom_init( eeprom_cfg_t *cfg, uint8_t dev_addr, uint16_t page_size, uint16_t max_size, uint8_t write_cycle,
                      uint32_t scl_pin, uint32_t sda_pin, uint32_t i2c_speed);

#endif  /* _EEPROM_CTRL_H */
