/** @file fota.c
 *
 * @brief
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "status.h"
#include "fota_define.h"
#include "ble_fota.h"
#include "ble_profile.h"
#include "flashctl.h"

/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/
#define MULTIPLE_OF_32K(Add) ((((Add) & (0x8000-1)) == 0)?1:0)
#define MULTIPLE_OF_64K(Add) ((((Add) & (0x10000-1)) == 0)?1:0)

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
const sys_information_t systeminfo =
{
    {"VerGet"},          /* Specific string for FOTA tool to search system information, in the code base should not appear second string like this */
    {"sys ver 0001"}     /* An example of current system information, able to modify by user */
};

/******************************************************************************
 * Local Variables
 ******************************************************************************/
fota_information_t *p_fota_info              = (fota_information_t *)(FOTA_UPDATE_BANK_INFO_ADDRESS);

fota_state_t       fw_upgrade_state          = OTA_STATE_IDLE;

uint32_t           fotadata_programed_addr   = 0;
uint32_t           fotadata_expectaddr       = 0;
uint32_t           fotadata_currlen          = 0;

uint32_t           fotadata_notify_interval  = 0xFFFFFFFF;
uint32_t           fotadata_last_notify_addr = 0;

fota_timer_t       timer_type                = 0;
uint32_t           expiry_time               = 0;
uint32_t           curr_time                 = 0;

uint8_t            fotadata_buffer[5];               // 1 byte for header
uint8_t            fotacmd_buffer[FW_INFO_LEN + 1];  // 1 byte for header

static uint32_t read_buf[FLASH_PROGRAM_SIZE_PAGE >> 2];
static uint8_t verify_buf[FLASH_PROGRAM_SIZE_PAGE];
/******************************************************************************
 * Private Functions
 ******************************************************************************/
/** This function is used to flash program for .
 *
 * @param[in] addr : Address of the flash location to be programmed.
 * @param[in] data : The data to be programmed.
 *
 */
static void fota_flash_program(uint32_t flash_addr, uint32_t data)
{
    flash_write_byte(flash_addr, data);
    while (flash_check_busy());
}

/** This function is used to flash erase for fota.
 *
 * @param[in] addr : Address of the flash page to be erased.
 *
 */
static uint8_t fota_flash_erase(uint32_t flash_addr)
{
    uint8_t status = BLE_ERR_OK;

    if (flash_erase(FLASH_ERASE_SECTOR, flash_addr) == STATUS_SUCCESS)
    {
        while (flash_check_busy());
    }
    else
    {
        status = STATUS_ERROR;
    }

    return status;
}

static void fota_program(uint32_t flash_addr, uint32_t data, uint32_t data_len)
{
    uint8_t program_idx, data_idx = 0;
    uint32_t pattern;
    uint8_t *p_data;

    for (program_idx = 0; program_idx < data_len; program_idx++)
    {
        p_data = (uint8_t *)&data;
        memcpy((uint8_t *)(&pattern), p_data + data_idx, FLASH_PROGRAM_SIZE);

        fota_flash_program((flash_addr + data_idx), pattern);
        data_idx = data_idx + FLASH_PROGRAM_SIZE;
    }
}

static void ble_fota_system_reboot(void)
{
    NVIC_SystemReset();
}

static uint32_t ble_fota_crc32checksum(uint32_t flash_addr, uint32_t data_len)
{
    uint16_t k;
    uint32_t i;
    uint8_t *p_buf = ((uint8_t *)flash_addr);
    uint32_t chk_sum = ~0, len = data_len;

    for (i = 0; i < len; i ++ )
    {
        chk_sum ^= *p_buf++;
        for (k = 0; k < 8; k ++)
        {
            chk_sum = chk_sum & 1 ? (chk_sum >> 1) ^ 0xedb88320 : chk_sum >> 1;
        }
    }
    return ~chk_sum;
}

/** FOTA timer start to countdown.
 *
 * @note       The purpose for each fota timer were listed as below: \n
 *             OTA_TIMER_OTA_DATA : Send notify when the FOTA data was not send to device in time. \n
 *             OTA_TIMER_OTA_COMPLETE : System reboot directly if disconnection event is missing. \n
 *             OTA_TIMER_OTA_ERASING : Terminated the connection to erase legacy bank FW and information. \n
 *             OTA_TIMER_OTA_DISCONNECT : After valid FW received, terminated the connection and trigger system reboot when disconnection event received. \n
 *
 * @param[in] Timeout : timer timeout value (unit:sec).
 * @param[in] Type : timer type.
 *
 * @return none
 */
static void ble_fota_timerstart(uint8_t timout, fota_timer_t type)
{
    expiry_time = timout + curr_time;
    timer_type = type;
}

/** The actions after FOTA timer expiry.
 *
 * @param[in] host_id : thid links's host id.
 * @param[in] Type : expired timer type.
 *
 * @return none
 */
static void ble_fota_timerexpiry(uint8_t host_id, fota_timer_t type)
{
    ble_err_t status;
    ble_gatt_data_param_t param;
    ble_info_link0_t *p_profile_info = (ble_info_link0_t *)ble_app_link_info[host_id].profile_info;

    switch (type)
    {
    case OTA_TIMER_OTA_DATA:
        fotadata_buffer[0] = OTA_DATA_NOTIFY_TIMEOUT;
        fotadata_buffer[1] = (fotadata_expectaddr & 0xFF);
        fotadata_buffer[2] = ((fotadata_expectaddr >> 8) & 0xFF);
        fotadata_buffer[3] = ((fotadata_expectaddr >> 16) & 0xFF);

        // set parameters
        param.host_id = host_id;
        param.handle_num = p_profile_info->svcs_info_fotas.server_info.handles.hdl_data;
        param.length = (sizeof(uint8_t) * 4);
        param.p_data = fotadata_buffer;
        status = ble_svcs_data_send(TYPE_BLE_GATT_NOTIFICATION, &param);

        if (status != BLE_ERR_OK)
        {
            info_color(LOG_RED, "Notify send fail\n");
        }
        fotadata_last_notify_addr = fotadata_expectaddr;
        break;

    case OTA_TIMER_OTA_COMPLETE:
    {
        ble_fota_system_reboot();
    }
    break;

    case OTA_TIMER_OTA_ERASING:
    {
        ble_cmd_conn_terminate(host_id);
    }
    break;

    case OTA_TIMER_OTA_DISCONNECT:
    {
        ble_fota_timerstart(3, OTA_TIMER_OTA_COMPLETE);
        ble_cmd_conn_terminate(host_id);
    }
    break;

    }
}

/** update Ble FOTA step by input parameter "Action"
 *
 * @note       FOTA step is used to record the bank saving how much upgrading FW roughly,
 *             so that we can continue the transmission if FOTA upgrade process restart unexpectedly. \n
 *             OTA_STEP_INIT : Get current FOTA step \n
 *             OTA_STEP_UPDATE : Stamp next FOTA step \n
 *             OTA_STEP_RESET : Reset current FOTA step to zero \n
 *
 * @param[in] action : the actions for update FOTA step.
 * @param[in] p_expect_add : the next expect address to program to bank.
 *
 * @return none
 */
static void ble_fota_step(fota_step_t action, uint32_t *p_expect_add)
{
    static uint32_t step_size = 0;
    static uint32_t curr_step = 0;
    uint32_t fota_bank_size;
    uint32_t flash_size  = Flash_Size();

    if (flash_size == FLASH_512K)
    {
        fota_bank_size = SIZE_OF_FOTA_BANK_512K;
    }
    else
    {
        fota_bank_size = SIZE_OF_FOTA_BANK_1MB;
    }

    if (action == OTA_STEP_INIT)
    {
        uint8_t *StepPtr = 0;

        /*get suitable step size*/
        while (step_size * OTA_DATA_STEP_TOTAL_NUM < fota_bank_size)
        {
            step_size += FLASH_PROGRAM_SIZE_PAGE;
        }

        /*calculate current step*/
        StepPtr = (uint8_t *)&p_fota_info->expectaddr_initstep;
        while (*(StepPtr + curr_step) == OTA_DATA_STEP_STAMPED)
        {
            curr_step++;
        }

        /*get expect address*/
        *p_expect_add = curr_step * step_size;

    }
    else if (action == OTA_STEP_UPDATE)
    {
        /*stamp new step*/
        if (*p_expect_add >= (curr_step + 1)*step_size)
        {
            uint8_t *p_addr = (uint8_t *)&p_fota_info->expectaddr_initstep;
            fota_program((uint32_t)(p_addr + curr_step), OTA_DATA_STEP_STAMPED, 1);
            curr_step++;
        }
    }
    else if (action == OTA_STEP_RESET)
    {
        curr_step = 0;
        step_size = 0;
        *p_expect_add = 0;
    }
}

static void set_flash_erase(uint32_t flash_addr, uint32_t image_size)
{
    uint32_t ErasedSize = 0;;

    while (image_size > ErasedSize)
    {
        if (((image_size - ErasedSize) > 0x10000) &&
                ( MULTIPLE_OF_64K(flash_addr + ErasedSize) ))
        {
            flash_erase(FLASH_ERASE_64K, flash_addr + ErasedSize);
            ErasedSize += 0x10000;
        }
        else if (((image_size - ErasedSize) > 0x8000) &&
                 ( MULTIPLE_OF_32K(flash_addr + ErasedSize) ))
        {
            flash_erase(FLASH_ERASE_32K, flash_addr + ErasedSize);
            ErasedSize += 0x8000;
        }
        else
        {
            flash_erase(FLASH_ERASE_SECTOR, flash_addr + ErasedSize);
            ErasedSize += SIZE_OF_FLASH_SECTOR_ERASE;
        }
        while (flash_check_busy());
    }
}

static void ble_fota_data_program(uint8_t length, uint8_t *data)
{
    static uint32_t temp_data[FLASH_PROGRAM_SIZE_PAGE >> 2];
    static uint16_t temp_datalen = 0;
    uint32_t fota_update_fw_addr;
    uint32_t flash_size = Flash_Size();

    if (flash_size == FLASH_512K)
    {
        fota_update_fw_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_512K;
    }
    else
    {
        fota_update_fw_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB;
    }

    /*  ------  here buffering OTA data for flash page programming  ------ */
    if (temp_datalen == 0)/* no buffering OTA data */
    {
        memcpy(((uint8_t *)&temp_data[0]), data, length);
        temp_datalen = length;
    }
    else if ((temp_datalen + length) >= FLASH_PROGRAM_SIZE_PAGE) /* buffering OTA data + current OTA data length over than flash page programming size */
    {
        memcpy(((uint8_t *)&temp_data[0]) + temp_datalen, data, (FLASH_PROGRAM_SIZE_PAGE - temp_datalen));
        flash_write_page((uint32_t)temp_data, (uint32_t)(fotadata_programed_addr + fota_update_fw_addr));
        while (flash_check_busy());

        fotadata_programed_addr += FLASH_PROGRAM_SIZE_PAGE;
        ble_fota_step(OTA_STEP_UPDATE, &fotadata_programed_addr);

        memcpy(((uint8_t *)&temp_data[0]), data + (FLASH_PROGRAM_SIZE_PAGE - temp_datalen), length - (FLASH_PROGRAM_SIZE_PAGE - temp_datalen));
        temp_datalen = length - (FLASH_PROGRAM_SIZE_PAGE - temp_datalen);
    }
    else /* buffering OTA data + current OTA data length lower than flash page programming size */
    {
        memcpy(((uint8_t *)&temp_data[0]) + temp_datalen, data, length);
        temp_datalen += length;
    }

    /* check buffer OTA data + current OTA data length over than new FW size */
    if ((fotadata_programed_addr + temp_datalen) >= p_fota_info->fotabank_datalen)
    {
        if (FLASH_PROGRAM_SIZE_PAGE - temp_datalen)
        {
            memset((((uint8_t *)&temp_data[0]) + temp_datalen), 0xFF, (FLASH_PROGRAM_SIZE_PAGE - temp_datalen));
        }
        flash_write_page((uint32_t)temp_data, (uint32_t)(fotadata_programed_addr + fota_update_fw_addr));
        while (flash_check_busy());

        fotadata_programed_addr += temp_datalen;
        ble_fota_step(OTA_STEP_UPDATE, &fotadata_programed_addr);
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/
/** Ble FOTA Timer tick
 *
 *
 * @note       This function should be called once every second, \n
 *             so that the fota related timers can be normally operation. \n
 *
 * @retval RUNNING : FOTA timer is running.
 * @retval EXPIRED : FOTA timer is expired.
 */
fota_timerstate_t ble_fota_timertick(void)
{
    curr_time++;

    if ((expiry_time != 0) && (curr_time > expiry_time))
    {
        expiry_time = 0;
        return EXPIRED;
    }

    return RUNNING;
}

/** Ble FOTA handle FOTA timer expired event
 *
 * @note       This function should be called when FOTA timer is expired.
 *
 * @param[in] host_id : thid links's host id.
 *
 * @return none
 */
void ble_fota_timerexpiry_handler(uint8_t host_id)
{
    ble_fota_timerexpiry(host_id, timer_type);
}

void ble_fota_fw_buffer_flash_check(void)
{
    uint32_t page_idx = 0, fota_bank_size, fota_update_fw_addr;
    uint8_t page_program_cnt_0 = 0, page_program_cnt_1 = 0;
    uint32_t flash_size = Flash_Size();

    if ((p_fota_info->fotabank_ready != FOTA_IMAGE_READY) &&
            (p_fota_info->status != FOTABANK_STATUS_FLASH_PROGRAMMING) &&
            (p_fota_info->status != FOTABANK_STATUS_FLASH_ERASE_FAIL))
    {
        if (flash_size == FLASH_512K)
        {
            fota_bank_size = SIZE_OF_FOTA_BANK_512K;
            fota_update_fw_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_512K;
        }
        else
        {
            fota_bank_size = SIZE_OF_FOTA_BANK_1MB;
            fota_update_fw_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB;
        }

        for (page_idx = 0 ; page_idx < fota_bank_size ; page_idx += FLASH_PROGRAM_SIZE_PAGE)
        {
            if (flash_read_page((uint32_t)read_buf, (fota_update_fw_addr + page_idx)) != STATUS_SUCCESS)
            {
                info_color(LOG_RED, "Read flash failed!\n");
                break;
            }
            while (flash_check_busy());

            memset(verify_buf, 0x00, sizeof(uint8_t) * FLASH_PROGRAM_SIZE_PAGE);
            if (memcmp(read_buf, verify_buf, FLASH_PROGRAM_SIZE_PAGE) != 0)
            {
                page_program_cnt_0++;
            }

            memset(verify_buf, 0xFF, sizeof(uint8_t) * FLASH_PROGRAM_SIZE_PAGE);
            if (memcmp(read_buf, verify_buf, FLASH_PROGRAM_SIZE_PAGE) != 0)
            {
                page_program_cnt_1++;
            }

            if ((page_program_cnt_1 != 0) && (page_program_cnt_0 != 0))
            {
                break;
            }
        }

        if ((page_program_cnt_0 != 0) && (page_program_cnt_1 != 0))
        {
            info_color(LOG_RED, "fw buffer not empty!\n");
            set_flash_erase((uint32_t)fota_update_fw_addr, fota_bank_size);
        }
    }
}

/** Ble FOTA parameters initialization
 *
 *
 * @return none
 */
void ble_fota_init(void)
{
    uint32_t i = 0;

    if (p_fota_info->fotabank_ready == FOTA_IMAGE_READY)
    {
        info_color(LOG_CYAN, "FOTA Result = %d\n", p_fota_info->fota_result);

        set_flash_erase((uint32_t)p_fota_info->fotabank_startaddr, p_fota_info->fotabank_datalen);
        flash_erase(FLASH_ERASE_SECTOR, FOTA_UPDATE_BANK_INFO_ADDRESS);
        while (flash_check_busy());
    }

    ble_fota_step(OTA_STEP_INIT, &fotadata_expectaddr);

    fotadata_programed_addr = fotadata_currlen = fotadata_last_notify_addr = fotadata_expectaddr;
    fotadata_notify_interval = 0xFFFFFFFF;

    info_color(LOG_CYAN, "sysinfo: ");
    for (i = 0; i < FW_INFO_LEN; i++)
    {
        info_color(LOG_CYAN, "%c", systeminfo.sysinfo[i]);
    }
    info_color(LOG_CYAN, "\n");

}

/** The actions related to FOTA after complete the disconnection.
 *
 * @note       perform the action by fw_upgrade_state. \n
 *             OTA_STATE_COMPLETE : System reboot for bootloader to check new FW. \n
 *             OTA_STATE_ERASING : Erase bank FW and bank information and waiting for reconnection. \n
 *
 * @return none
 */
void ble_fota_disconnect(void)
{
    if (fw_upgrade_state == OTA_STATE_COMPLETE)
    {
        ble_fota_system_reboot();
    }
    else if (fw_upgrade_state == OTA_STATE_ERASING)
    {
        uint32_t page_idx = 0, fota_bank_size, fota_update_fw_addr;
        uint32_t flash_size = Flash_Size();

        if (flash_size == FLASH_512K)
        {
            fota_bank_size = SIZE_OF_FOTA_BANK_512K;
            fota_update_fw_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_512K;
        }
        else
        {
            fota_bank_size = SIZE_OF_FOTA_BANK_1MB;
            fota_update_fw_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB;
        }

        fota_flash_erase(FOTA_UPDATE_BANK_INFO_ADDRESS);
        for (page_idx = 0 ; page_idx < fota_bank_size ; page_idx += SIZE_OF_FLASH_SECTOR_ERASE)
        {
            if (fota_flash_erase(fota_update_fw_addr + page_idx) != STATUS_SUCCESS)
            {
                fota_program((uint32_t)(&p_fota_info->status), FOTABANK_STATUS_FLASH_ERASE_FAIL, 4);
                break;
            }
        }
        ble_fota_step(OTA_STEP_RESET, &fotadata_expectaddr);
        ble_fota_init();
    }
}

/** Ble FOTA command processing
 *
 *
 * @param[in] host_id : thid links's host id.
 * @param[in] length : command length.
 * @param[in] p_data : command payload.
 *
 * @note       First byte of command payload contains command ID and each command ID may contain different information behind.
 *             OTA_CMD_QUERY : Get device current system information. \n
 *             OTA_CMD_START : Start FW upgrade, this command contains new FW length and CRC. \n
 *             OTA_CMD_ERASE : Terminated the connection and erasing legacy FW and information. \n
 *             OTA_CMD_APPLY : Apply the new FW if receiving FW length and CRC matched with OTA_CMD_START. \n
 *
 * @return none
 */
void ble_fota_cmd(uint8_t host_id, uint8_t length, uint8_t *p_data)
{
    fota_cmd_t cmd = p_data[0];
    fota_errcode_t errcode = OTA_ERR_CODE_NO_ERR;
    uint8_t ind_len = sizeof(uint8_t); /*first byte of indication always contains error code*/
    uint32_t i;
    ble_info_link0_t *p_profile_info;
    ble_gatt_data_param_t param;
    uint32_t fota_bank_size, fota_update_fw_addr;
    uint32_t flash_size = Flash_Size();

    if (flash_size == FLASH_512K)
    {
        fota_bank_size = SIZE_OF_FOTA_BANK_512K;
        fota_update_fw_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_512K;
    }
    else
    {
        fota_bank_size = SIZE_OF_FOTA_BANK_1MB;
        fota_update_fw_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB;
    }

    switch (cmd)
    {
    case OTA_CMD_QUERY:
    {
        /* FOTA query command format.
            _______________________
            octets    |1          |
            _______________________
            parameter | CommandID |
                      | (0x00)    |
            _______________________ */
        /*Send system information by indication*/
        for (i = 0 ; i < FW_INFO_LEN ; i++)
        {
            fotacmd_buffer[ind_len + i] = systeminfo.sysinfo[i];
        }
        ind_len += FW_INFO_LEN;
    }
    break;

    case OTA_CMD_START:
    {
        uint32_t fw_len;
        uint32_t fw_crc;
        uint8_t cmd_idx = 1;
        /* FOTA start command format.
            _________________________________________________
            octets    |1          |4       |4    |4         |
            _________________________________________________
            parameter | CommandID | FW     | FW  | Notify   |
                      | (0x01)    | length | CRC | interval |
            _________________________________________________ */
        fw_len = ((p_data[3 + cmd_idx] << 24) | (p_data[2 + cmd_idx] << 16) | (p_data[1 + cmd_idx] << 8) | p_data[cmd_idx]) ;
        cmd_idx += sizeof(uint32_t);

        fw_crc = ((p_data[3 + cmd_idx] << 24) | (p_data[2 + cmd_idx] << 16) | (p_data[1 + cmd_idx] << 8) | p_data[cmd_idx]) ;
        cmd_idx += sizeof(uint32_t);

        fotadata_notify_interval = ((p_data[3 + cmd_idx] << 24) | (p_data[2 + cmd_idx] << 16) | (p_data[1 + cmd_idx] << 8) | p_data[cmd_idx]) ;
        cmd_idx += sizeof(uint32_t);

        if ((p_fota_info->status == FOTABANK_STATUS_FLASH_PROGRAMMING) || (fw_upgrade_state == OTA_STATE_START)) /*Check if there were unfinish FOTA data */
        {
            info_color(LOG_CYAN, "ExpectAddr: 0x%08x\n", fotadata_expectaddr);
            fotadata_currlen = fotadata_last_notify_addr = fotadata_expectaddr;
            /*Send legacy FW informations (FW length, FW CRC & next expected FW address) by indication*/
            errcode = OTA_ERR_CODE_ALREADY_START;
            fotacmd_buffer[1] = ((p_fota_info->fotabank_datalen) & 0xFF);
            fotacmd_buffer[2] = ((p_fota_info->fotabank_datalen >> 8) & 0xFF);
            fotacmd_buffer[3] = ((p_fota_info->fotabank_datalen >> 16) & 0xFF);
            fotacmd_buffer[4] = ((p_fota_info->fotabank_datalen >> 24) & 0xFF);
            fotacmd_buffer[5] = ((p_fota_info->fotabank_crc) & 0xFF);
            fotacmd_buffer[6] = ((p_fota_info->fotabank_crc >> 8) & 0xFF);
            fotacmd_buffer[7] = ((p_fota_info->fotabank_crc >> 16) & 0xFF);
            fotacmd_buffer[8] = ((p_fota_info->fotabank_crc >> 24) & 0xFF);
            fotacmd_buffer[9] = (fotadata_expectaddr & 0xFF);
            fotacmd_buffer[10] = ((fotadata_expectaddr >> 8) & 0xFF);
            fotacmd_buffer[11] = ((fotadata_expectaddr >> 16) & 0xFF);

            ind_len += sizeof(uint8_t) * 11;
            fw_upgrade_state = OTA_STATE_START;

        }
        else if (fw_len > fota_bank_size) /*Check if updating FW length larger than bank size */
        {
            /*Send bank size by indication*/
            errcode = OTA_ERR_CODE_OUT_OF_BANK_SIZE;
            fotacmd_buffer[1] = (fota_bank_size & 0xFF);
            fotacmd_buffer[2] = ((fota_bank_size >> 8) & 0xFF);
            fotacmd_buffer[3] = ((fota_bank_size >> 16) & 0xFF);
            fotacmd_buffer[4] = ((fota_bank_size >> 24) & 0xFF);
            ind_len += sizeof(uint8_t) * 4;
        }
        else if (p_fota_info->status == FOTABANK_STATUS_FLASH_ERASE_FAIL) /*Check if bank flash was fail to erase*/
        {
            /*Send bank flash fail to erase by indication*/
            errcode = OTA_ERR_CODE_FLASH_ERASE_ERR;
        }
        else
        {
            /*Record updating FW information into flash and start FOTA update procedure*/
            fw_upgrade_state = OTA_STATE_START;
            fotadata_currlen = fotadata_expectaddr = 0;
            fota_program((uint32_t)(&p_fota_info->status), FOTABANK_STATUS_FLASH_PROGRAMMING, 4);
            fota_program((uint32_t)(&p_fota_info->fotabank_crc), fw_crc, 4);
            fota_program((uint32_t)(&p_fota_info->fotabank_datalen), fw_len, 4);
        }
        info_color(LOG_CYAN, "fota start %d, ExpectAddr: 0x%08x interval %d\n", errcode, fotadata_expectaddr, fotadata_notify_interval);

    }
    break;

    case OTA_CMD_ERASE:
    {
        /* FOTA erase command format.
            _______________________
            octets    |1          |
            _______________________
            parameter | CommandID |
                      | (0x02)    |
            _______________________ */
        /*Erasing the legacy updating FW and FW information*/
        fw_upgrade_state = OTA_STATE_ERASING;
        ble_fota_timerstart(1, OTA_TIMER_OTA_ERASING);
        info_color(LOG_CYAN, "fota erase, disconnect to clean legacy FW\n");
    }
    break;

    case OTA_CMD_APPLY:
    {
        uint32_t fw_update_add;
        uint32_t chk_sum;
        uint8_t cmd_idx = 1;

        /* FOTA apply command format.
            _________________________________
            octets    |1          |4        |
            _________________________________
            parameter | CommandID | Bank0   |
                      | (0x03)    | address |
            _________________________________  */

        if (fw_upgrade_state != OTA_STATE_START) /*Check if FOTA procedure has started */
        {
            errcode = OTA_ERR_CODE_UPDATE_NOT_START;
        }
        else
        {
            fw_update_add = ((p_data[3 + cmd_idx] << 24) | (p_data[2 + cmd_idx] << 16) | (p_data[1 + cmd_idx] << 8) | p_data[cmd_idx]) ;
            cmd_idx += sizeof(uint32_t);
            fw_upgrade_state = OTA_STATE_ERASING;

            chk_sum = ble_fota_crc32checksum((uint32_t)fota_update_fw_addr, p_fota_info->fotabank_datalen);
            info_color(LOG_CYAN, "ChkSum 0x%08x 0x%08x\n", chk_sum,  p_fota_info->fotabank_crc);
            if (fotadata_currlen != p_fota_info->fotabank_datalen) /*Check if receiving FW length matched FOTA start command*/
            {
                errcode = OTA_ERR_CODE_FW_LEN_ERR;
            }
            else if (chk_sum != p_fota_info->fotabank_crc) /*Check if receiving FW CRC matched FOTA start command*/
            {
                info_color(LOG_CYAN, "ChkSum 0x%08x 0x%08x\n", chk_sum,  p_fota_info->fotabank_crc);
                errcode = OTA_ERR_CODE_FW_CRC_ERR;
            }
            else
            {
                /*Set FOTA informations into flash for bootloader*/
                fota_program((uint32_t)(&p_fota_info->fotabank_ready), FOTA_IMAGE_READY, 4);
                fota_program((uint32_t)(&p_fota_info->fotabank_startaddr), (uint32_t)fota_update_fw_addr, 4);
                fota_program((uint32_t)(&p_fota_info->target_startaddr), fw_update_add, 4);

                fw_upgrade_state = OTA_STATE_COMPLETE;

            }
            ble_fota_timerstart(1, OTA_TIMER_OTA_DISCONNECT);
        }
        info_color(LOG_RED, "fota apply %d\n", errcode);
    }
    break;

    default:
        info_color(LOG_RED, "not supported %d\n", cmd);
        errcode = OTA_ERR_CODE_CMD_ERR;
        break;
    }

    p_profile_info = (ble_info_link0_t *)ble_app_link_info[host_id].profile_info;
    fotacmd_buffer[0] = errcode;

    // set parameters
    param.host_id = host_id;
    param.handle_num = p_profile_info->svcs_info_fotas.server_info.handles.hdl_command;
    param.length = ind_len;
    param.p_data = fotacmd_buffer;

    ble_svcs_data_send(TYPE_BLE_GATT_INDICATION, &param);
}


/** Ble FOTA data processing
 *
 *
 * @param[in] host_id : thid links's host id.
 * @param[in] length : data length.
 * @param[in] p_data : data payload.
 *
 * @note       First 4 bytes of data payload is data header which contains the FOTA data address (3 bytes) and length (1 byte),
 *             if there were invalid data header, send notification to response it. \n
 *
 * @return none
 */
void ble_fota_data(uint8_t host_id, uint8_t length, uint8_t *p_data)
{
    ble_err_t status;
    uint32_t data_addr = 0;
    uint32_t ota_data_idx = 0;
    fota_notify_t notify = OTA_DATA_NOTIFY_NONE;
    uint8_t data_len;
    uint8_t notify_len = 0;
    uint32_t fota_bank_size;
    uint32_t flash_size = Flash_Size();

    if (flash_size == FLASH_512K)
    {
        fota_bank_size = SIZE_OF_FOTA_BANK_512K;
    }
    else
    {
        fota_bank_size = SIZE_OF_FOTA_BANK_1MB;
    }

    /* FOTA data format.
    _____________________________________
    octets    |3        |1       | Var  |
    _____________________________________
    parameter | Data    | Data   | Data |
              | address | length |      |
    _____________________________________  */
    data_addr = ((p_data[2] << 16) | (p_data[1] << 8) | p_data[0]) ;
    data_len = p_data[3];
    ota_data_idx += sizeof(uint32_t);
    if (fw_upgrade_state != OTA_STATE_START) /*Check if FOTA procedure has started*/
    {
        notify = OTA_DATA_NOTIFY_NOT_START;
    }
    else if (data_addr > fota_bank_size) /*Check if FOTA data's address is larger than bank size*/
    {
        notify = OTA_DATA_NOTIFY_ADDRESS_ERR;

    }
    else if (data_addr != fotadata_expectaddr) /*Check if FOTA data's address is matched with expecting*/
    {
        notify = OTA_DATA_NOTIFY_ADDRESS_UNEXPECTED;

        fotadata_buffer[1] = (fotadata_expectaddr & 0xFF);
        fotadata_buffer[2] = ((fotadata_expectaddr >> 8) & 0xFF);
        fotadata_buffer[3] = ((fotadata_expectaddr >> 16) & 0xFF);
        notify_len += (sizeof(uint8_t) * 3);
        fotadata_last_notify_addr = fotadata_expectaddr;
        info_color(LOG_CYAN, "Add unexp %d %d\n", data_addr, fotadata_expectaddr);
    }
    else if ((data_len == 0) || (data_len > length)) /*Check if FOTA data length vaild*/
    {
        notify = OTA_DATA_NOTIFY_LEN_ERROR;

        fotadata_buffer[1] = data_len;
        notify_len += sizeof(uint8_t);
        info_color(LOG_CYAN, "data len %d length %d\n", data_len, length);
    }
    else if ((data_len + fotadata_currlen) >  fota_bank_size)/*Check if total received FOTA data length larger than bank size*/
    {
        notify = OTA_DATA_NOTIFY_TOTAL_LEN_ERR;

        fotadata_buffer[1] = ((fotadata_currlen + data_len) & 0xFF);
        fotadata_buffer[2] = (((fotadata_currlen + data_len) >> 8) & 0xFF);
        fotadata_buffer[3] = (((fotadata_currlen + data_len) >> 16) & 0xFF);
        fotadata_buffer[4] = (((fotadata_currlen + data_len) >> 24) & 0xFF);
        notify_len += (sizeof(uint8_t) * 4);
    }

    if (notify != OTA_DATA_NOTIFY_NONE)/*Check if notification needs to send*/
    {
        ble_gatt_data_param_t param;
        ble_info_link0_t *p_profile_info = (ble_info_link0_t *)ble_app_link_info[host_id].profile_info;

        fotadata_buffer[0] = notify;
        notify_len += sizeof(uint8_t);

        // set parameters
        param.host_id = host_id;
        param.handle_num = p_profile_info->svcs_info_fotas.server_info.handles.hdl_data;
        param.length = notify_len;
        param.p_data = fotadata_buffer;

        status = ble_svcs_data_send(TYPE_BLE_GATT_NOTIFICATION, &param);
        if (status != BLE_ERR_OK)
        {
            info_color(LOG_RED, "Notify send failed, status = %d\n", notify);
        }
    }
    else
    {
        /* programming OTA data into flash ota bank*/
        ble_fota_data_program(data_len, p_data + ota_data_idx);

        /* update how much OTA data is receiving */
        fotadata_expectaddr += data_len;

        /*Check if periodic notification interval reached*/
        if (fotadata_expectaddr >= (fotadata_notify_interval + fotadata_last_notify_addr))
        {
            ble_gatt_data_param_t param;
            ble_info_link0_t *p_profile_info = (ble_info_link0_t *)ble_app_link_info[host_id].profile_info;

            fotadata_last_notify_addr = fotadata_expectaddr;
            fotadata_buffer[0] = OTA_DATA_NOTIFY_PERIODIC;
            fotadata_buffer[1] = (fotadata_expectaddr & 0xFF);
            fotadata_buffer[2] = ((fotadata_expectaddr >> 8) & 0xFF);
            fotadata_buffer[3] = ((fotadata_expectaddr >> 16) & 0xFF);

            // set parameters
            param.host_id = host_id;
            param.handle_num = p_profile_info->svcs_info_fotas.server_info.handles.hdl_data;
            param.length = (sizeof(uint8_t) * 4);
            param.p_data = fotadata_buffer;

            status = ble_svcs_data_send(TYPE_BLE_GATT_NOTIFICATION, &param);
            if (status != BLE_ERR_OK)
            {
                info_color(LOG_RED, "Notify periodic send fail\n");
            }
            info_color(LOG_CYAN, "Notify int 0x%04x %d\n", fotadata_expectaddr, status);
        }

        fotadata_currlen += data_len;
        ble_fota_timerstart(1, OTA_TIMER_OTA_DATA);
    }
}
