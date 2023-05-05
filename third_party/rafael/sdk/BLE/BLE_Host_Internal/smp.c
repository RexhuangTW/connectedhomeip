/*******************************************************************
 *
 * File Name  : SMP.C
 * Description:
 *
 *******************************************************************
 *
 *      Copyright (c) 2020, All Right Reserved
 *      Rafael Microelectronics Co. Ltd.
 *      Taiwan, R.O.C.
 *
 *******************************************************************/

#include "smp.h"
#include "ble_printf.h"
#include "cm3_mcu.h"
#include "host_management.h"
#include "l2cap.h"
#include "sys_arch.h"
#include <stdint.h>
#include <string.h>

#include "ble_api.h"
#include "ble_bonding.h"
#include "ble_event_module.h"
#include "ble_gap_api.h"
#include "ble_hci.h"
#include "ble_host_cmd.h"
#include "ble_memory.h"
#include "hci_cmd_security.h"
#include "lib_config.h"
#include "task_ble_app.h"

#define TASK_DELAY_MS (2) /* task delay time (ms) */
l2cap_smp_pairing_req_rsp_t app_prefer_smp_format;
const l2cap_smp_pairing_req_rsp_t default_pairing_response_format = {
    0x0007,                              // PDU Length
    0x0006,                              // Attribute Protocol : L2CAP_CID_SECURITY_MANAGER_PROTOCOL
    CODE_SMP_PAIRING_RESPONSE,           // code : CODE_SMP_PAIRING_RESPONSE
    IO_CAPABILITY_NOINPUT_NOOUTPUT,      // IO Capabilities, User define
    OOB_AUTHENTICATION_DATA_NOT_PRESENT, // OOB data flag: OOB Authentication data not present
    {                                    // AuthReq
      AUTHREQ_BONDING_FLAGS_BONDING | MITM_PROTECTION_NO | 0x00 },
    SIZE_MINIMUM_ENCRYPTION_KEY, // Maximum Encryption Key Size: 16 Octets
    (                            // Initiator Key Distribution
        KEY_DISTRIBUTION_ENCKEY_1 | KEY_DISTRIBUTION_IDKEY_1 |
        // KEY_DISTRIBUTION_SIGN_1 |
        0x00),
    ( // Responder Key Distribution
        KEY_DISTRIBUTION_ENCKEY_1 | KEY_DISTRIBUTION_IDKEY_1 |
        // KEY_DISTRIBUTION_SIGN_1 |
        0x00),
};

const uint8_t SEL_STK_GEN_MTHD[5][5] = {
    // DisplayOnly,                  Display YesNo,              Keyboard-Only,                      NoInputNoOutput,
    // KeyboardDisplay
    {
        STK_GEN_MTHD_JUST_WORKS, STK_GEN_MTHD_JUST_WORKS, STK_GEN_MTHD_PASSKEY_ENTRY_DISP, STK_GEN_MTHD_JUST_WORKS,
        STK_GEN_MTHD_PASSKEY_ENTRY_DISP // DisplayOnly
    },
    {
        STK_GEN_MTHD_JUST_WORKS, STK_GEN_MTHD_JUST_WORKS, STK_GEN_MTHD_PASSKEY_ENTRY_DISP, STK_GEN_MTHD_JUST_WORKS,
        STK_GEN_MTHD_PASSKEY_ENTRY_DISP // Display YesNo
    },
    {
        STK_GEN_MTHD_PASSKEY_ENTRY, STK_GEN_MTHD_PASSKEY_ENTRY, STK_GEN_MTHD_PASSKEY_ENTRY, STK_GEN_MTHD_JUST_WORKS,
        STK_GEN_MTHD_PASSKEY_ENTRY // Keyboard-Only
    },
    {
        STK_GEN_MTHD_JUST_WORKS, STK_GEN_MTHD_JUST_WORKS, STK_GEN_MTHD_JUST_WORKS, STK_GEN_MTHD_JUST_WORKS,
        STK_GEN_MTHD_JUST_WORKS // NoInputNoOutput
    },
    {
        STK_GEN_MTHD_PASSKEY_ENTRY, STK_GEN_MTHD_PASSKEY_ENTRY, STK_GEN_MTHD_PASSKEY_ENTRY_DISP, STK_GEN_MTHD_JUST_WORKS,
        STK_GEN_MTHD_PASSKEY_ENTRY // KeyboardDisplay
    }
};

const uint8_t IRK_FIXED[][16] = { { // 0xB5, 0xB7, 0xE1, 0x48,
                                    // 0xAC, 0x7E, 0x3B, 0x91,
                                    // 0x8A, 0xA6, 0xD7, 0x1C,
                                    // 0x45, 0x33, 0xE5, 0x0B
                                    IRK_PREDEF_FIXED } };

uint8_t resolvable_comp_list[10][3];
uint8_t identity_resolving_seed[16];
ble_irk_rpa_addr_t * param_rpa;

const uint8_t CSRK_FIXED[][16] = { { 0xE5, 0x8B, 0xE8, 0x8D, 0x78, 0x61, 0x75, 0xA7, 0xE3, 0x4B, 0x01, 0x80, 0xA0, 0xFB, 0xF9,
                                     0x82 } };

const uint8_t EDIV_FIXED[SIZE_SMP_EDIV] = // Reference spec. vol.6 P82
    { 0x67, 0x75 };

const uint8_t r_AES_INI[16] = { 0x64, 0xe8, 0xb6, 0x77, 0x1b, 0x49, 0x83, 0x23, 0x07, 0x00, 0x4f, 0xf6, 0xc9, 0x16, 0x2e, 0x1d };

const uint8_t LTK_FIXED[16] = // Reference spec. vol.6 P82
    { 0xE2, 0x48, 0x5A, 0x7A, 0xD8, 0xD9, 0x7C, 0x22, 0x30, 0x57, 0xFA, 0x42, 0xDB, 0x59, 0x8B, 0x25 };

int8_t Prcss_BLE_CODE_SMP_Reserved(void);
int8_t Prcss_BLE_CODE_SMP_Pairing_Request(void);
int8_t Prcss_BLE_CODE_SMP_Pairing_Response(void);
int8_t Prcss_BLE_CODE_SMP_Pairing_Confirm(void);
int8_t Prcss_BLE_CODE_SMP_Pairing_Random(void);
int8_t Prcss_BLE_CODE_SMP_Pairing_Failed(void);
int8_t Prcss_BLE_CODE_SMP_Encryption_Information(void);
int8_t Prcss_BLE_CODE_SMP_Master_Identification(void);
int8_t Prcss_BLE_CODE_SMP_Identity_Information(void);
int8_t Prcss_BLE_CODE_SMP_Identity_Address_Information(void);
int8_t Prcss_BLE_CODE_SMP_Signing_Information(void);
int8_t Prcss_BLE_CODE_SMP_Security_Request(void);
int8_t Prcss_BLE_CODE_SMP_Pairing_Public_Key(void);
int8_t Prcss_BLE_CODE_SMP_Pairing_DHKEY_Check(void);
int8_t Prcss_BLE_CODE_SMP_Pairing_Keypress_Notification(void);

void memcpy_inv(uint8_t * pDst, uint8_t * pSrc, uint16_t len)
{
    uint16_t i;

    if (len)
    {
        i = 0;
        do
        {
            len--;
            *(pDst + len) = *(pSrc + i);
            i++;
        } while (len != 0);
    }
}

void setBLE_ConnTxData_SMP(uint16_t connID, uint8_t L2CAP_Code_SMP, uint8_t * L2CAP_RspDataSMP, uint8_t Length)
{
    ACL_Data * ACL_data;
    ble_hci_tx_acl_data_hdr_t * p_hci_msg;
    MBLK * mblk;

    p_hci_msg = mem_malloc(sizeof(ble_hci_tx_acl_data_hdr_t));
    while (p_hci_msg == NULL)
    {
        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
        sys_task_delay(TASK_DELAY_MS);
        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
        p_hci_msg = mem_malloc(sizeof(ble_hci_tx_acl_data_hdr_t));
    }
    p_hci_msg->transport_id = BLE_TRANSPORT_HCI_ACL_DATA;
    p_hci_msg->handle       = connID;
    p_hci_msg->pb_flag      = 0;
    p_hci_msg->bc_flag      = 0;
    p_hci_msg->length       = SIZE_BASIC_L2CAP_HEADER + Length + 1;

    ACL_data                                = (ACL_Data *) &acl_data_tx[0];
    ACL_data->Length_PDU                    = Length + 1;
    ACL_data->ChannelID                     = L2CAP_CID_SECURITY_MANAGER_PROTOCOL;
    ACL_data->PDU.smp_op_generic.SmprOpcode = L2CAP_Code_SMP;
    memcpy_inv(ACL_data->PDU.smp_op_generic.data, L2CAP_RspDataSMP, Length);

    mblk = get_msgblks_L1(p_hci_msg->length);
    while (mblk == NULL)
    {
        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
        sys_task_delay(TASK_DELAY_MS);
        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
        mblk = get_msgblks_L1(p_hci_msg->length);
    }
    acl_data2msgblk(mblk, (uint8_t *) ACL_data, p_hci_msg->length);
    p_hci_msg->p_data = mblk;

    task_host_queue_send(TASK_HOST_QUEUE_TX_ACL_DATA, p_hci_msg);
}

int8_t send_le_encrypt(uint8_t * key, uint8_t * plaintext_data)
{
    ble_hci_le_encrypt_param_t param;

    memcpy_inv(&param.key[0], key, SIZE_AES_KEY);
    memcpy_inv(&param.plaintext_data[0], plaintext_data, SIZE_AES_KEY);

    return hci_le_encrypt_cmd(&param);
}

int8_t confirm_value_generation_c1_p1(uint8_t host_id, uint8_t * k, uint8_t * r)
{
    encrypt_queue_t encrypt_msg;
    LE_Host_Para * p_host_current;
    uint8_t i, p_AES[16];
    int8_t err_code;

    err_code       = ERR_OK;
    p_host_current = &LE_Host[host_id];
    if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
    {
        memcpy_inv(p_AES, &p_host_current->Remote_IOCapab, 6);
        p_AES[6] = CODE_SMP_PAIRING_RESPONSE;
        memcpy_inv(&p_AES[7], &app_prefer_smp_format.para.pairing_request.IOCapability, 6);
        p_AES[13] = CODE_SMP_PAIRING_REQUEST;
        p_AES[14] = p_host_current->PeerAddrType; // HCI_ADDR_TYPE_PUBLIC or HCI_ADDR_TYPE_RANDOM, 0x00 or 0x01
        p_AES[15] = p_host_current->OwnAddrType;  // PUBLIC_ADDR or RANDOM_STATIC_ADDR, 0x00 or 0x01
    }
    else
    {
        memcpy_inv(p_AES, &app_prefer_smp_format.para.pairing_response.IOCapability, 6);
        p_AES[6]  = CODE_SMP_PAIRING_RESPONSE;
        p_AES[7]  = p_host_current->Remote_ResponserKeyDistrib;
        p_AES[8]  = p_host_current->Remote_InitiatorKeyDistrib;
        p_AES[9]  = p_host_current->Remote_MaxEncKeySize;
        p_AES[10] = p_host_current->Remote_AuthReq;
        p_AES[11] = p_host_current->Remote_OOBDataFlag;
        p_AES[12] = p_host_current->Remote_IOCapab;
        p_AES[13] = CODE_SMP_PAIRING_REQUEST;
        p_AES[14] = p_host_current->OwnAddrType;  // PUBLIC_ADDR or RANDOM_STATIC_ADDR, 0x00 or 0x01
        p_AES[15] = p_host_current->PeerAddrType; // HCI_ADDR_TYPE_PUBLIC or HCI_ADDR_TYPE_RANDOM, 0x00 or 0x01
    }

    for (i = 0; i < 16; i++)
    {
        p_AES[i] = r[i] ^ p_AES[i]; // p_AES: P1
    }

    vPortEnterCritical();
    if (send_le_encrypt(k, p_AES) == ERR_OK)
    {
        // send encrypt queue
        encrypt_msg.host_id       = host_id;
        encrypt_msg.encrypt_state = STATE_CONFIRM_VALUE_GEN_C1_P1;
        sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
    }
    else
    {
        err_code = ERR_MEM;
    }
    vPortExitCritical();
    return err_code;
}

int8_t confirm_value_generation_c1_p2(uint8_t host_id, uint8_t * encrypt_out)
{
    encrypt_queue_t encrypt_msg;
    LE_Host_Para * p_host_current;
    uint8_t p_AES[16], i;
    int8_t err_code;

    err_code       = ERR_OK;
    p_host_current = &LE_Host[host_id];
    memset(p_AES, 0, SIZE_AES_KEY);
    if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
    {
        memcpy_inv(&p_AES[10], p_host_current->PeerAddr, 6);
        memcpy_inv(&p_AES[4], p_host_current->OwnAddr, 6);
    }
    else
    {
        memcpy_inv(&p_AES[4], p_host_current->PeerAddr, 6);
        memcpy_inv(&p_AES[10], p_host_current->OwnAddr, 6);
    }
    for (i = 0; i < 16; i++)
    {
        p_AES[i] = encrypt_out[15 - i] ^ p_AES[i]; // p_AES: P2
    }

    if (send_le_encrypt(p_host_current->PeerLTK_Passkey, p_AES) == ERR_OK)
    {
        // send encrypt queue
        encrypt_msg.host_id       = host_id;
        encrypt_msg.encrypt_state = STATE_CONFIRM_VALUE_GEN_C1_P2;
        sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

void send_pairing_confirm(uint8_t host_id, uint8_t * encrypt_out)
{
    LE_Host_Para * p_host_current;
    uint8_t temp[16];

    p_host_current = &LE_Host[host_id];
    memcpy_inv(temp, encrypt_out, SIZE_AES_KEY);
    setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_PAIRING_CONFIRM, temp, SIZE_AES_KEY);
}

void verify_pairing_random_valid_or_not(uint8_t host_id, uint8_t * encrypt_out)
{
    encrypt_queue_t encrypt_msg;
    LE_Host_Para * p_host_current;
    uint8_t temp_aes[16], i;

    p_host_current = &LE_Host[host_id];
    memcpy_inv(temp_aes, encrypt_out, SIZE_AES_KEY);
    if (memcmp(temp_aes, p_host_current->Confirm_STK, SIZE_AES_KEY) == 0)
    {
        p_host_current->KeyType = SMP_KEY_USE_STK;
        if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
        {
            memcpy(temp_aes, &p_host_current->PeerIRK_M_S_RAND[8], 8);
            memcpy(&temp_aes[8], &p_host_current->RandomValue[8], 8);
            send_le_encrypt(p_host_current->PeerLTK_Passkey, temp_aes);
        }
        else if (p_host_current->Role == BLE_GAP_ROLE_PERIPHERAL)
        {
            memcpy(temp_aes, &p_host_current->RandomValue[8], 8);
            memcpy(&temp_aes[8], &p_host_current->PeerIRK_M_S_RAND[8], 8);
            send_le_encrypt(p_host_current->PeerLTK_Passkey, temp_aes);
        }

        memset(p_host_current->PeerIRK_M_S_RAND, 0xFF, SIZE_SMP_IRK);
        // send encrypt queue
        encrypt_msg.host_id       = host_id;
        encrypt_msg.encrypt_state = STATE_GEN_STK_S1;
        sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
    }
    else
    {
        MBLK * p_mblk;
        ble_evt_param_t * p_evt_param;
        ble_host_to_app_evt_t * p_queue_param;

        // failed, generate error response
        i = ERR_CODE_SMP_CONFIRM_VALUE_FAILED;
        setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_PAIRING_FAILED, &i, 1);

        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        while (p_queue_param == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        }
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

        p_evt_param                                                       = (ble_evt_param_t *) p_queue_param->parameter;
        p_evt_param->event                                                = BLE_SM_EVT_AUTH_STATUS;
        p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.host_id = host_id;
        p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.status  = ERR_CODE_SMP_CONFIRM_VALUE_FAILED;
        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

        p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
        while (p_mblk == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
        }
        p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S253;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

        bhc_timer_clear(host_id, TIMER_EVENT_AUTH_STATUS);
    }
}

int8_t random_address_hash_ah(uint8_t * irk, uint8_t * prand)
{
    uint8_t r[16];
    int8_t err_code;

    memset(r, 0, 13);
    memcpy_inv(&r[13], prand, 3);

    if (send_le_encrypt(irk, r) == ERR_OK)
    {
        err_code = ERR_OK;
    }
    else
    {
        err_code = ERR_MEM;
    }
    return err_code;
}

int8_t private_address_construct_by_id(uint8_t host_id, private_addr_gen_t * private_addr_param)
{
    encrypt_queue_t encrypt_msg;
    int8_t err_code;

    vPortEnterCritical();
    if (random_address_hash_ah(private_addr_param->irk, private_addr_param->private_addr24) == ERR_OK)
    {
        // send encrypt queue
        encrypt_msg.host_id       = host_id;
        encrypt_msg.encrypt_state = STATE_RANDOM_ADDRESS_HASH_AH_GEN;
        sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
        err_code = ERR_OK;
    }
    else
    {
        err_code = ERR_MEM;
    }
    vPortExitCritical();
    return err_code;
}

int8_t private_address_resolve_by_id(uint8_t host_id, uint8_t * private_addr24)
{
    encrypt_queue_t encrypt_msg;
    LE_Host_Para * p_host_current;
    int8_t err_code;

    p_host_current = &LE_Host[host_id];

    if (random_address_hash_ah(p_host_current->PeerIRK_M_S_RAND, private_addr24) == ERR_OK)
    {
        // send encrypt queue
        encrypt_msg.host_id       = host_id;
        encrypt_msg.encrypt_state = STATE_RANDOM_ADDRESS_HASH_AH_COMP;
        sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
        err_code = ERR_OK;
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** The BLE host generate a random value for security.
 *
 * @param[in]  data : encrpyt queue message data.
 * @param[in]  encrypt_out : pointer of the le encrypt event data.
 *
 * @retval  ERR_OK : successful.
 * @retval  ERR_MEM: the queue is full or memory not enough.
 */
int8_t bhc_prcss_le_encrypt_event(encrypt_queue_t data, uint8_t * encrypt_out)
{
    encrypt_queue_t encrypt_msg;
    LE_Host_Para * p_host_current;
    uint8_t host_id;
    int8_t err_code;

    err_code       = ERR_OK;
    host_id        = data.host_id;
    p_host_current = &LE_Host[host_id];
    switch (data.encrypt_state)
    {
    case STATE_GEN_RANDOM_VALUE: {
        memcpy_inv(p_host_current->RandomValue, encrypt_out, SIZE_AES_KEY);
        memcpy(p_host_current->OwnLTK, &p_host_current->RandomValue[(p_host_current->RandomValue[0] & 0x03)], 10);
        memcpy(p_host_current->OwnRAND, &p_host_current->RandomValue[(p_host_current->RandomValue[0] & 0x03)], 10);
        memcpy(&p_host_current->OwnLTK[10], &p_host_current[(p_host_current->OwnLTK[0] & 0x03)], 5);
        p_host_current->OwnLTK[15] = EDIV_FIXED[(p_host_current->OwnLTK[0] & 0x01)];

        if (send_le_encrypt(p_host_current->OwnLTK, (uint8_t *) LTK_FIXED) == ERR_OK)
        {
            // send encrypt queue
            encrypt_msg.host_id       = host_id;
            encrypt_msg.encrypt_state = STATE_GEN_LTK;
            sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
        }
        else
        {
            err_code = ERR_MEM;
        }
    }
    break;

    case STATE_GEN_LTK:
        memcpy_inv(p_host_current->OwnLTK, encrypt_out, SIZE_AES_KEY);
        break;

    case STATE_CONFIRM_VALUE_GEN_C1_P1:
        err_code = confirm_value_generation_c1_p2(host_id, encrypt_out);
        break;

    case STATE_CONFIRM_VALUE_GEN_C1_P2:
        if (p_host_current->state_pairing == CODE_SMP_PAIRING_CONFIRM)
        {
            send_pairing_confirm(host_id, encrypt_out);
        }
        else if (p_host_current->state_pairing == CODE_SMP_PAIRING_RANDOM)
        {
            verify_pairing_random_valid_or_not(host_id, encrypt_out);
        }
        break;

    case STATE_GEN_STK_S1:
        if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
        {
            ble_hci_le_enable_encrypt_param_t param;

            memcpy_inv(p_host_current->Confirm_STK, encrypt_out, SIZE_AES_KEY);
            memset(p_host_current->Confirm_STK, 0, (SIZE_AES_KEY - p_host_current->MaxEncKeySize));

            param.conn_handle = p_host_current->LL_ConnID;
            memset(&param.random_number[0], 0, SIZE_SMP_RAND);
            memset(&param.encrypted_diversifier[0], 0, SIZE_SMP_EDIV);
            memcpy_inv(&param.long_term_key[0], p_host_current->Confirm_STK, SIZE_AES_KEY);

            err_code = hci_le_enable_encryption_cmd(&param);
        }
        else
        {
            memcpy_inv(p_host_current->Confirm_STK, encrypt_out, SIZE_AES_KEY);
            memset(p_host_current->Confirm_STK, 0, (SIZE_AES_KEY - p_host_current->MaxEncKeySize));
            setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_PAIRING_RANDOM, p_host_current->RandomValue, SIZE_AES_KEY);
        }
        break;

    case STATE_RANDOM_ADDRESS_HASH_AH_GEN:
        memcpy(param_rpa[host_id].rpa_addr, encrypt_out, 3);
        break;

    case STATE_RANDOM_ADDRESS_HASH_AH_COMP:
        do
        {
            uint16_t conn_id;
            static uint8_t cnt = 0;

            memcpy(resolvable_comp_list[cnt++], encrypt_out, 3);
            if (cnt == 10)
            {
                cnt = 0;
            }
            if (bhc_host_id_is_valid_check(host_id) == FALSE)
            {
                break;
            }

            // check connection exist or not
            if (bhc_host_id_is_connected_check(host_id, &conn_id) == FALSE)
            {
                break;
            }
            else
            {
                memcpy(p_host_current->private_addr24, encrypt_out, 3);
                if (memcmp(encrypt_out, p_host_current->PeerAddr, 3) != 0)
                {
                    ble_cmd_conn_terminate(host_id);
                }
                else
                {
                    chkATT_DB(BLE_HOSTID_RESERVED, p_host_current->LL_ConnID);
                }
            }
        } while (0);
        break;

    case STATE_GEN_LOCAL_IRK: {
        uint32_t sys_time_cnt;
        private_addr_gen_t addr_gen_param;

        memcpy(param_rpa[host_id].irk, encrypt_out, SIZE_SMP_IRK);
        addr_gen_param.irk = param_rpa[host_id].irk;
        sys_time_cnt       = sys_now() & 0x003FFFFF;
        sys_time_cnt |= 0x400000;
        addr_gen_param.private_addr24 = (uint8_t *) &sys_time_cnt;
        err_code                      = private_address_construct_by_id(host_id, &addr_gen_param);
        if (err_code == ERR_OK)
        {
            memcpy(&param_rpa[host_id].rpa_addr[3], (uint8_t *) &sys_time_cnt, 3);
        }
    }
    break;

    case STATE_GEN_LOCAL_IRK_BY_SECURITY:
        memcpy(LE_Host[host_id].OwnIRK, encrypt_out, SIZE_SMP_IRK);
        memcpy(param_rpa[host_id].irk, encrypt_out, SIZE_SMP_IRK);
        break;

    default:
        break;
    }

    return err_code;
}

/** Set security pass key
 */
int8_t bhc_sm_passkey_set(uint32_t hex_passkey, uint16_t conn_id)
{
    LE_Host_Para * p_host_current;
    uint8_t host_id;

    host_id        = bhc_query_host_id_by_conn_id(conn_id);
    p_host_current = &LE_Host[host_id];

    memcpy_inv(&p_host_current->PeerLTK_Passkey[12], (uint8_t *) &hex_passkey, 4);

    p_host_current->state_pairing = CODE_SMP_PAIRING_CONFIRM;

    return confirm_value_generation_c1_p1(host_id, p_host_current->PeerLTK_Passkey, p_host_current->RandomValue);
}

int8_t (*const Prcss_BLE_CODE_SMP[])(void) = {
    Prcss_BLE_CODE_SMP_Reserved,
    Prcss_BLE_CODE_SMP_Pairing_Request,
    Prcss_BLE_CODE_SMP_Pairing_Response,
    Prcss_BLE_CODE_SMP_Pairing_Confirm,
    Prcss_BLE_CODE_SMP_Pairing_Random,
    Prcss_BLE_CODE_SMP_Pairing_Failed,
    Prcss_BLE_CODE_SMP_Encryption_Information,
    Prcss_BLE_CODE_SMP_Master_Identification,
    Prcss_BLE_CODE_SMP_Identity_Information,
    Prcss_BLE_CODE_SMP_Identity_Address_Information,
    Prcss_BLE_CODE_SMP_Signing_Information,
    Prcss_BLE_CODE_SMP_Security_Request,
    Prcss_BLE_CODE_SMP_Pairing_Public_Key,
    Prcss_BLE_CODE_SMP_Pairing_DHKEY_Check,
    Prcss_BLE_CODE_SMP_Pairing_Keypress_Notification,
};

int8_t Prcss_BLE_CODE_SMP_Reserved(void)
{
    return ERR_OK;
}

int8_t Prcss_BLE_CODE_SMP_Pairing_Request(void)
{
    ble_host_to_app_evt_t * p_queue_param;
    ble_evt_param_t * p_evt_param;
    MBLK * p_mblk;
    LE_Host_Para * p_host_current;
    ACL_Data * ACL_data;
    uint8_t host_id, i;

    ACL_data       = (ACL_Data *) g_l2cap_buffer.data;
    host_id        = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];

    memcpy(&p_host_current->Remote_IOCapab, &ACL_data->PDU.smp_op_pairing_response.IOCapability, 6);
    if ((ACL_data->PDU.smp_op_pairing_response.AuthReq.MITM |
         (app_prefer_smp_format.para.pairing_response.AuthReq.MITM & MITM_PROTECTION_YES)) == 0)
    {
        p_host_current->SMP_STKGenMethod = STK_GEN_MTHD_JUST_WORKS;
    }
    else
    {
        p_host_current->SMP_STKGenMethod =
            SEL_STK_GEN_MTHD[app_prefer_smp_format.para.pairing_response.IOCapability][p_host_current->Remote_IOCapab];
    }

    if ((ACL_data->PDU.smp_op_pairing_request.MaximumEncryptionKeySize > 6) &&
        (ACL_data->PDU.smp_op_pairing_request.MaximumEncryptionKeySize < 17))
    {
        p_host_current->MaxEncKeySize = app_prefer_smp_format.para.pairing_response.MaximumEncryptionKeySize >=
                ACL_data->PDU.smp_op_pairing_request.MaximumEncryptionKeySize
            ? ACL_data->PDU.smp_op_pairing_request.MaximumEncryptionKeySize
            : app_prefer_smp_format.para.pairing_response.MaximumEncryptionKeySize;

        memset(p_host_current->PeerLTK_Passkey, 0, SIZE_AES_KEY);

        if (setBLE_ConnTxData(p_host_current->LL_ConnID, (uint8_t *) &app_prefer_smp_format,
                              (app_prefer_smp_format.length + SIZE_BASIC_L2CAP_HEADER)) == ERR_OK)
        {
            p_host_current->Smp_Phase = 1;

            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
            while (p_queue_param == NULL)
            {
                BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                sys_task_delay(TASK_DELAY_MS);
                Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
            }
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

            p_evt_param                                                          = (ble_evt_param_t *) p_queue_param->parameter;
            p_evt_param->event                                                   = BLE_SM_EVT_STK_GENERATION_METHOD;
            p_evt_param->event_param.ble_evt_sm.param.evt_stk_gen_method.host_id = host_id;
            p_evt_param->event_param.ble_evt_sm.param.evt_stk_gen_method.key_gen_method = LE_Host[host_id].SMP_STKGenMethod;
            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

            i = p_host_current->Remote_InitiatorKeyDistrib & app_prefer_smp_format.para.pairing_request.InitiatorKeyDistribution;
            if ((i & KEY_DISTRIBUTION_IDKEY_1))
            {
                if (bhc_host_gen_new_irk(host_id) == ERR_MEM)
                {
                    BLE_PRINTF(BLE_DEBUG_ERR, "Gen new irk fail.\n");
                }
            }
            else
            {
                memset(p_host_current->OwnIRK, 0xFF, SIZE_SMP_IRK);
            }
            bhc_timer_set(host_id, TIMER_EVENT_AUTH_STATUS, 3000); // 30s
        }
    }
    else
    {
        i = ERR_CODE_SMP_ENCRYPTION_KEY_SIZE;

        setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_PAIRING_FAILED, &i, 1);

        p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
        while (p_mblk == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
        }
        p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S253;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        while (p_queue_param == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        }
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

        p_evt_param                                                       = (ble_evt_param_t *) p_queue_param->parameter;
        p_evt_param->event                                                = BLE_SM_EVT_AUTH_STATUS;
        p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.host_id = host_id;
        p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.status  = ERR_CODE_SMP_ENCRYPTION_KEY_SIZE;
        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

        bhc_timer_clear(host_id, TIMER_EVENT_AUTH_STATUS);
    }

    return ERR_OK;
}

int8_t Prcss_BLE_CODE_SMP_Pairing_Response(void)
{
    ble_host_to_app_evt_t * p_queue_param;
    ble_evt_param_t * p_evt_param;
    MBLK * p_mblk;
    LE_Host_Para * p_host_current;
    ACL_Data * ACL_data;
    uint8_t host_id, i;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data       = (ACL_Data *) g_l2cap_buffer.data;
        host_id        = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];

        memcpy(&p_host_current->Remote_IOCapab, &ACL_data->PDU.smp_op_pairing_response.IOCapability, 6);
        if ((ACL_data->PDU.smp_op_pairing_response.AuthReq.MITM |
             (app_prefer_smp_format.para.pairing_request.AuthReq.MITM & MITM_PROTECTION_YES)) == 0)
        {
            p_host_current->SMP_STKGenMethod = STK_GEN_MTHD_JUST_WORKS;
        }
        else
        {
            p_host_current->SMP_STKGenMethod =
                SEL_STK_GEN_MTHD[app_prefer_smp_format.para.pairing_request.IOCapability][p_host_current->Remote_IOCapab];
        }

        if ((ACL_data->PDU.smp_op_pairing_response.MaximumEncryptionKeySize > 6) &&
            (ACL_data->PDU.smp_op_pairing_response.MaximumEncryptionKeySize < 17))
        {
            p_host_current->MaxEncKeySize = app_prefer_smp_format.para.pairing_request.MaximumEncryptionKeySize >=
                    ACL_data->PDU.smp_op_pairing_response.MaximumEncryptionKeySize
                ? ACL_data->PDU.smp_op_pairing_response.MaximumEncryptionKeySize
                : app_prefer_smp_format.para.pairing_request.MaximumEncryptionKeySize;

            memset(p_host_current->PeerLTK_Passkey, 0, SIZE_AES_KEY);
            switch (p_host_current->SMP_STKGenMethod)
            {
            case STK_GEN_MTHD_PASSKEY_ENTRY_DISP:
            case STK_GEN_MTHD_PASSKEY_ENTRY:
                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
                if (p_queue_param == NULL)
                {
                    err_code = ERR_MEM;
                    break;
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

                p_evt_param        = (ble_evt_param_t *) p_queue_param->parameter;
                p_evt_param->event = BLE_SM_EVT_PASSKEY_CONFIRM;
                p_evt_param->event_param.ble_evt_sm.param.evt_passkey_confirm_param.host_id = host_id;
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                p_host_current->Smp_Phase = 2;
                break;

            default: // STK_GEN_MTHD_JUST_WORKS
                if (bhc_sm_passkey_set(0, p_host_current->LL_ConnID) == ERR_MEM)
                {
                    err_code = ERR_MEM;
                    break;
                }
                p_host_current->Smp_Phase = 2;
                break;
            }
            if (err_code == ERR_OK)
            {
                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
                while (p_queue_param == NULL)
                {
                    BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                    sys_task_delay(TASK_DELAY_MS);
                    Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

                p_evt_param                                                          = (ble_evt_param_t *) p_queue_param->parameter;
                p_evt_param->event                                                   = BLE_SM_EVT_STK_GENERATION_METHOD;
                p_evt_param->event_param.ble_evt_sm.param.evt_stk_gen_method.host_id = host_id;
                p_evt_param->event_param.ble_evt_sm.param.evt_stk_gen_method.key_gen_method = LE_Host[host_id].SMP_STKGenMethod;
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                i = p_host_current->Remote_InitiatorKeyDistrib &
                    app_prefer_smp_format.para.pairing_request.InitiatorKeyDistribution;
                if ((i & KEY_DISTRIBUTION_IDKEY_1))
                {
                    if (bhc_host_gen_new_irk(host_id) == ERR_MEM)
                    {
                        BLE_PRINTF(BLE_DEBUG_ERR, "Gen new irk fail.\n");
                    }
                }
                else
                {
                    memset(p_host_current->OwnIRK, 0xFF, SIZE_SMP_IRK);
                }

                bhc_timer_set(host_id, TIMER_EVENT_AUTH_STATUS, 3000); // 30s
            }
        }
        else
        {
            i = ERR_CODE_SMP_ENCRYPTION_KEY_SIZE;

            setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_PAIRING_FAILED, &i, 1);

            p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
            while (p_mblk == NULL)
            {
                BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                sys_task_delay(TASK_DELAY_MS);
                Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
            }
            p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S253;
            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
            while (p_queue_param == NULL)
            {
                BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                sys_task_delay(TASK_DELAY_MS);
                Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
            }
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

            p_evt_param                                                       = (ble_evt_param_t *) p_queue_param->parameter;
            p_evt_param->event                                                = BLE_SM_EVT_AUTH_STATUS;
            p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.host_id = host_id;
            p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.status  = ERR_CODE_SMP_ENCRYPTION_KEY_SIZE;
            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

            bhc_timer_clear(host_id, TIMER_EVENT_AUTH_STATUS);
        }
    } while (0);

    return err_code;
}

int8_t Prcss_BLE_CODE_SMP_Pairing_Confirm(void)
{
    ble_host_to_app_evt_t * p_queue_param;
    ble_evt_param_t * p_evt_param;
    LE_Host_Para * p_host_current;
    ACL_Data * ACL_data;
    uint8_t host_id;
    int8_t err_code;

    err_code       = ERR_OK;
    ACL_data       = (ACL_Data *) g_l2cap_buffer.data;
    host_id        = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];

    memcpy_inv(&p_host_current->Confirm_STK[0], &ACL_data->PDU.smp_op_pairing_confirm.ConfirmValue[0], SIZE_AES_KEY);
    if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
    {
        setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_PAIRING_RANDOM, p_host_current->RandomValue, SIZE_AES_KEY);
    }
    else
    {
        switch (p_host_current->SMP_STKGenMethod)
        {
        case STK_GEN_MTHD_PASSKEY_ENTRY_DISP:
        case STK_GEN_MTHD_PASSKEY_ENTRY:
            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
            if (p_queue_param == NULL)
            {
                err_code = ERR_MEM;
                break;
            }
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

            p_evt_param        = (ble_evt_param_t *) p_queue_param->parameter;
            p_evt_param->event = BLE_SM_EVT_PASSKEY_CONFIRM;
            p_evt_param->event_param.ble_evt_sm.param.evt_passkey_confirm_param.host_id = host_id;
            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

            p_host_current->Smp_Phase = 2;
            break;

        default: // STK_GEN_MTHD_JUST_WORKS
            if (bhc_sm_passkey_set(0, p_host_current->LL_ConnID) == ERR_MEM)
            {
                err_code = ERR_MEM;
                break;
            }
            p_host_current->Smp_Phase = 2;
            break;
        }
    }

    return err_code;
}

int8_t Prcss_BLE_CODE_SMP_Pairing_Random(void)
{
    LE_Host_Para * p_host_current;
    ACL_Data * ACL_data;
    uint8_t host_id;

    ACL_data       = (ACL_Data *) g_l2cap_buffer.data;
    host_id        = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];
    memcpy_inv(p_host_current->PeerIRK_M_S_RAND, ACL_data->PDU.smp_op_pairing_random.RandomValue, SIZE_AES_KEY);

    p_host_current->state_pairing = CODE_SMP_PAIRING_RANDOM;
    return confirm_value_generation_c1_p1(host_id, p_host_current->PeerLTK_Passkey, p_host_current->PeerIRK_M_S_RAND);
}

int8_t Prcss_BLE_CODE_SMP_Pairing_Failed(void)
{
    ble_host_to_app_evt_t * p_queue_param;
    ble_evt_param_t * p_evt_param;
    ACL_Data * ACL_data;
    MBLK * p_mblk;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *) g_l2cap_buffer.data;
        host_id  = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);

        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param == NULL)
        {
            err_code = ERR_MEM;
            break;
        }
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

        p_evt_param                                                       = (ble_evt_param_t *) p_queue_param->parameter;
        p_evt_param->event                                                = BLE_SM_EVT_AUTH_STATUS;
        p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.host_id = host_id;
        p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.status  = ACL_data->PDU.smp_op_pairing_failed.Reason;
        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

        p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
        while (p_mblk == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
        }
        p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S253;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

        bhc_timer_clear(host_id, TIMER_EVENT_AUTH_STATUS);
    } while (0);

    return err_code;
}

int8_t Prcss_BLE_CODE_SMP_Encryption_Information(void)
{
    LE_Host_Para * p_host_current;
    uint8_t host_id;
    ACL_Data * ACL_data;

    ACL_data       = (ACL_Data *) g_l2cap_buffer.data;
    host_id        = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];

    memcpy_inv(p_host_current->PeerLTK_Passkey, ACL_data->PDU.smp_op_encryption_information.LongTermKey, SIZE_AES_KEY);
    return ERR_OK;
}

int8_t Prcss_BLE_CODE_SMP_Master_Identification(void)
{
    LE_Host_Para * p_host_current;
    uint8_t host_id, i;
    MBLK * p_mblk;
    ACL_Data * ACL_data;
    uint8_t inverse_addr[7];
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data       = (ACL_Data *) g_l2cap_buffer.data;
        host_id        = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];
        memcpy(p_host_current->PeerEDIV, ACL_data->PDU.smp_op_master_identification.EDIV, SIZE_SMP_EDIV);
        memcpy(p_host_current->PeerRAND, ACL_data->PDU.smp_op_master_identification.RAND, SIZE_SMP_RAND);
        if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
        {
            if (((p_host_current->Remote_ResponserKeyDistrib &
                  app_prefer_smp_format.para.pairing_request.ResponderKeyDistribution) &
                 (KEY_DISTRIBUTION_IDKEY_1 | KEY_DISTRIBUTION_SIGN_1)) == 0)
            {
                i = p_host_current->Remote_InitiatorKeyDistrib &
                    app_prefer_smp_format.para.pairing_request.InitiatorKeyDistribution;
                if ((i & KEY_DISTRIBUTION_ENCKEY_1))
                {
                    memset(p_host_current->OwnLTK, 0, (SIZE_AES_KEY - p_host_current->MaxEncKeySize));
                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_ENCRYPTION_INFORMATION,
                                          (uint8_t *) p_host_current->OwnLTK, SIZE_AES_KEY);
                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_MASTER_IDENTIFICATION,
                                          (uint8_t *) p_host_current->OwnRAND, (SIZE_SMP_EDIV + SIZE_SMP_RAND));
                    p_host_current->KeyType = SMP_KEY_USE_LTK;
                }
                if ((i & KEY_DISTRIBUTION_IDKEY_1))
                {
                    ble_gap_addr_t identity_addr;

                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_IDENTITY_INFORMATION,
                                          (uint8_t *) p_host_current->OwnIRK, SIZE_SMP_IRK);
                    ble_gap_device_identity_address_get(&identity_addr);
                    inverse_addr[6] = identity_addr.addr_type;
                    memcpy_inv((uint8_t *) &inverse_addr[0], (uint8_t *) &identity_addr.addr[0], BLE_ADDR_LEN);
                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_IDENTITY_ADDRESS_INFORMATION, &inverse_addr[0],
                                          (1 + BLE_ADDR_LEN));
                }
                if ((i & KEY_DISTRIBUTION_SIGN_1))
                {
                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_SIGNING_INFORMATION, (uint8_t *) CSRK_FIXED,
                                          SIZE_SMP_CSRK);
                }

                p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
                while (p_mblk == NULL)
                {
                    BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                    sys_task_delay(TASK_DELAY_MS);
                    Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                    p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
                }
                p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
                task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

                Chk_BOND_StorageOrNot(host_id);
            }
        }
        else
        {
            if (((p_host_current->Remote_InitiatorKeyDistrib &
                  app_prefer_smp_format.para.pairing_response.InitiatorKeyDistribution) &
                 (KEY_DISTRIBUTION_IDKEY_1 | KEY_DISTRIBUTION_SIGN_1)) == 0)
            {
                p_mblk = get_msgblks_L1(sizeof(MHS_ATT_Authe_Carry_On_Para));
                if (p_mblk == NULL)
                {
                    err_code = ERR_MEM;
                    break;
                }
                p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
                task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

                Chk_BOND_StorageOrNot(host_id);
            }
        }
    } while (0);

    return err_code;
}

int8_t Prcss_BLE_CODE_SMP_Identity_Information(void)
{
    LE_Host_Para * p_host_current;
    ACL_Data * ACL_data;
    uint8_t host_id;

    ACL_data       = (ACL_Data *) g_l2cap_buffer.data;
    host_id        = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];

    memcpy_inv(p_host_current->PeerIRK_M_S_RAND, ACL_data->PDU.smp_op_identity_information.IdentityResolvingKey, SIZE_SMP_IRK);
    return ERR_OK;
}

int8_t Prcss_BLE_CODE_SMP_Identity_Address_Information(void)
{
    LE_Host_Para * p_host_current;
    ACL_Data * ACL_data;
    uint8_t host_id, i;
    MBLK * p_mblk;
    uint8_t inverse_addr[7];
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        host_id                    = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current             = &LE_Host[host_id];
        ACL_data                   = (ACL_Data *) g_l2cap_buffer.data;
        p_host_current->IDAddrType = ACL_data->PDU.smp_op_identity_addr_information.AddrType;
        memcpy_inv(p_host_current->IDAddr, ACL_data->PDU.smp_op_identity_addr_information.BDADDR, BLE_ADDR_LEN);

        if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
        {
            if (((p_host_current->Remote_ResponserKeyDistrib &
                  app_prefer_smp_format.para.pairing_request.ResponderKeyDistribution) &
                 KEY_DISTRIBUTION_SIGN_1) == 0)
            {
                i = p_host_current->Remote_InitiatorKeyDistrib &
                    app_prefer_smp_format.para.pairing_request.InitiatorKeyDistribution;
                if ((i & KEY_DISTRIBUTION_ENCKEY_1))
                {
                    memset(p_host_current->OwnLTK, 0, (SIZE_AES_KEY - p_host_current->MaxEncKeySize));
                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_ENCRYPTION_INFORMATION,
                                          (uint8_t *) p_host_current->OwnLTK, SIZE_AES_KEY);
                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_MASTER_IDENTIFICATION,
                                          (uint8_t *) p_host_current->OwnRAND, (SIZE_SMP_EDIV + SIZE_SMP_RAND));
                    p_host_current->KeyType = SMP_KEY_USE_LTK;
                }
                if ((i & KEY_DISTRIBUTION_IDKEY_1))
                {
                    ble_gap_addr_t identity_addr;

                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_IDENTITY_INFORMATION,
                                          (uint8_t *) p_host_current->OwnIRK, SIZE_SMP_IRK);
                    ble_gap_device_identity_address_get(&identity_addr);
                    inverse_addr[6] = identity_addr.addr_type;
                    memcpy_inv((uint8_t *) &inverse_addr[0], (uint8_t *) &identity_addr.addr[0], BLE_ADDR_LEN);
                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_IDENTITY_ADDRESS_INFORMATION, &inverse_addr[0],
                                          (1 + BLE_ADDR_LEN));
                }
                if ((i & KEY_DISTRIBUTION_SIGN_1))
                {
                    setBLE_ConnTxData_SMP(g_l2cap_buffer.conn_handle, CODE_SMP_SIGNING_INFORMATION, (uint8_t *) CSRK_FIXED,
                                          SIZE_SMP_CSRK);
                }

                p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
                while (p_mblk == NULL)
                {
                    BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                    sys_task_delay(TASK_DELAY_MS);
                    Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                    p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
                }
                p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
                task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

                Chk_BOND_StorageOrNot(host_id);
            }
        }
        else
        {
            if (((p_host_current->Remote_InitiatorKeyDistrib &
                  app_prefer_smp_format.para.pairing_response.InitiatorKeyDistribution) &
                 (KEY_DISTRIBUTION_SIGN_1)) == 0)
            {
                p_mblk = get_msgblks_L1(sizeof(MHS_ATT_Authe_Carry_On_Para));
                if (p_mblk == NULL)
                {
                    err_code = ERR_MEM;
                    break;
                }
                p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
                task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

                Chk_BOND_StorageOrNot(host_id);
            }
        }
    } while (0);

    return err_code;
}

int8_t Prcss_BLE_CODE_SMP_Signing_Information(void)
{
    MBLK * p_mblk;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    host_id  = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_mblk   = get_msgblks_L1(sizeof(MHS_ATT_Authe_Carry_On_Para));
    if (p_mblk != NULL)
    {
        p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

        Chk_BOND_StorageOrNot(host_id);
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

int8_t Prcss_BLE_CODE_SMP_Security_Request(void)
{
    LE_Host_Para * p_host_current;
    MBLK * p_mblk;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        host_id        = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];

        if ((p_host_current->IsEncryption == DISABLE) && (p_host_current->Smp_Phase == 0) &&
            (p_host_current->state_authe < HS_ATE_S2))
        {
            p_mblk = get_msgblks_L1(sizeof(MHS_ATT_Authe_Carry_On_Para));
            if (p_mblk == NULL)
            {
                err_code = ERR_MEM;
                break;
            }
            p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S4;
            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
        }
    } while (0);

    return err_code;
}

int8_t Prcss_BLE_CODE_SMP_Pairing_Public_Key(void)
{
    return ERR_OK;
}

int8_t Prcss_BLE_CODE_SMP_Pairing_DHKEY_Check(void)
{
    return ERR_OK;
}

int8_t Prcss_BLE_CODE_SMP_Pairing_Keypress_Notification(void)
{
    return ERR_OK;
}

/** The BLE host processing long term key request event.
 *
 * @param[in]  p_data : pointer of the long term key request event data.
 *
 * @retval  ERR_OK : successful.
 * @retval  ERR_MEM: the queue is full or memory not enough.
 */
int8_t bhc_smp_long_term_key_req(ble_hci_le_meta_evt_param_long_term_key_req_t * p_data)
{
#if (_CONN_SUPPORT_ == 1)
    LE_Host_Para * p_host_current;
    uint8_t host_id, zero_array[SIZE_SMP_RAND + SIZE_SMP_EDIV];
    uint8_t *k, i;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        host_id        = bhc_query_host_id_by_conn_id(p_data->conn_handle);
        p_host_current = &LE_Host[host_id];

        if (p_host_current->KeyType != SMP_KEY_USE_STK)
        {
            smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;
            k = ble_flashbond_cmd(CMD_FB_GET_EXIST_PID_BY_HOST_ID, (uint8_t *) smp_Para_Bond_tmp);
            if (*k == FLH_BND_ERR_CODE_NO_ERR)
            {
                k = ble_flashbond_cmd(CMD_FB_GET_KEY_FLASHBOND_PARA_BOND, (uint8_t *) smp_Para_Bond_tmp);
                if (*k == FLH_BND_ERR_CODE_NO_ERR)
                {
                    for (i = 0; i < BLE_ADDR_LEN; i++)
                    {}
                    if (i == BLE_ADDR_LEN)
                    {
                        i = 7;
                        while (i < SMP_PARA_BOND_SIZE)
                        {
                            *((&p_host_current->BOND_Role) + (i - 7)) = *(k + TAB_PARA_DATA_BOND_ROLE + (i - 7));
                            i++;
                        }
                    }
                }
            }
        }

        if (p_host_current->KeyType == SMP_KEY_USE_STK)
        {
            memset(zero_array, 0, SIZE_SMP_RAND + SIZE_SMP_EDIV);
            if (memcmp(p_data->random_num, zero_array, (SIZE_SMP_RAND + SIZE_SMP_EDIV)) == 0)
            {
                ble_hci_le_long_term_key_req_reply_param_t param;

                param.conn_handle = p_data->conn_handle;
                memcpy_inv(&param.long_term_key[0], p_host_current->Confirm_STK, SIZE_AES_KEY);
                err_code = hci_le_long_term_key_req_reply_cmd(&param);
                break;
            }
            else
            {
                // should sent LL_REJECT_IND
                ble_hci_le_long_term_key_neg_reply_param_t param;

                param.conn_handle = p_data->conn_handle;
                err_code          = hci_le_long_term_key_req_neg_reply_cmd(&param);
                break;
            }
        }
        else // if (p_host_current->KeyType == SMP_KEY_USE_STK)
        {
            if (p_host_current->BOND_Role == BLE_GAP_ROLE_CENTRAL)
            {
                if (memcmp(p_data->random_num, p_host_current->PeerRAND, (SIZE_SMP_RAND + SIZE_SMP_EDIV)) == 0)
                {
                    ble_hci_le_long_term_key_req_reply_param_t param;

                    param.conn_handle = p_data->conn_handle;
                    memcpy_inv(&param.long_term_key[0], p_host_current->PeerLTK_Passkey, SIZE_AES_KEY);
                    err_code = hci_le_long_term_key_req_reply_cmd(&param);
                    break;
                }
                else
                {
                    // should sent LL_REJECT_IND
                    ble_hci_le_long_term_key_neg_reply_param_t param;

                    param.conn_handle = p_data->conn_handle;
                    err_code          = hci_le_long_term_key_req_neg_reply_cmd(&param);
                    break;
                }
            }
            else
            {
                if (memcmp(p_data->random_num, p_host_current->OwnRAND, (SIZE_SMP_RAND + SIZE_SMP_EDIV)) == 0)
                {
                    ble_hci_le_long_term_key_req_reply_param_t param;

                    param.conn_handle = p_data->conn_handle;
                    memcpy_inv(&param.long_term_key[0], p_host_current->OwnLTK, SIZE_AES_KEY);
                    err_code = hci_le_long_term_key_req_reply_cmd(&param);
                    break;
                }
                else
                {
                    // should sent LL_REJECT_IND
                    ble_hci_le_long_term_key_neg_reply_param_t param;

                    param.conn_handle = p_data->conn_handle;
                    err_code          = hci_le_long_term_key_req_neg_reply_cmd(&param);
                    break;
                }
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_CONN_SUPPORT_ == 1))
}

/** The BLE host processing encryption change event.
 *
 * @param[in]  p_data : pointer of the encryption change event data.
 *
 */
void bhc_prcss_encrpytion_change(ble_hci_evt_param_encrypt_change_t * p_data)
{
    LE_Host_Para * p_host_current;
    MBLK * p_mblk;
    uint8_t host_id, i, inverse_addr[7];

    host_id        = bhc_query_host_id_by_conn_id(p_data->conn_handle);
    p_host_current = &LE_Host[host_id];

    if (p_data->encrypt_enabled == HCI_ENCRPT_EN_AES_CCM_LE)
    {
        if (p_host_current->KeyType == SMP_KEY_USE_STK)
        {
            if (p_host_current->Role == BLE_GAP_ROLE_PERIPHERAL)
            {
                i = p_host_current->Remote_ResponserKeyDistrib &
                    app_prefer_smp_format.para.pairing_response.ResponderKeyDistribution;
                if ((i & KEY_DISTRIBUTION_ENCKEY_1))
                {
                    memset(p_host_current->OwnLTK, 0, (SIZE_AES_KEY - p_host_current->MaxEncKeySize));
                    setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_ENCRYPTION_INFORMATION,
                                          (uint8_t *) p_host_current->OwnLTK, SIZE_AES_KEY);
                    setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_MASTER_IDENTIFICATION,
                                          (uint8_t *) p_host_current->OwnRAND, (SIZE_SMP_EDIV + SIZE_SMP_RAND));
                    p_host_current->KeyType = SMP_KEY_USE_LTK;
                }
                if ((i & KEY_DISTRIBUTION_IDKEY_1))
                {
                    ble_gap_addr_t identity_addr;

                    setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_IDENTITY_INFORMATION,
                                          (uint8_t *) p_host_current->OwnIRK, SIZE_SMP_IRK);
                    ble_gap_device_identity_address_get(&identity_addr);
                    inverse_addr[6] = identity_addr.addr_type;
                    memcpy_inv((uint8_t *) &inverse_addr[0], (uint8_t *) &identity_addr.addr[0], BLE_ADDR_LEN);
                    setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_IDENTITY_ADDRESS_INFORMATION, &inverse_addr[0],
                                          (1 + BLE_ADDR_LEN));
                }
                if ((i & KEY_DISTRIBUTION_SIGN_1))
                {
                    setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_SIGNING_INFORMATION, (uint8_t *) CSRK_FIXED,
                                          SIZE_SMP_CSRK);
                }
                i = p_host_current->Remote_InitiatorKeyDistrib &
                    app_prefer_smp_format.para.pairing_response.InitiatorKeyDistribution;
                if (i == 0)
                {
                    p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
                    while (p_mblk == NULL)
                    {
                        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                        sys_task_delay(TASK_DELAY_MS);
                        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                        p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
                    }
                    p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
                    task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

                    Chk_BOND_StorageOrNot(host_id);
                }
            }
            else //(p_host_current->Role == HCI_ROLE_SLAVE)
            {
                i = p_host_current->Remote_ResponserKeyDistrib &
                    app_prefer_smp_format.para.pairing_request.ResponderKeyDistribution;
                if (i == 0)
                {
                    i = p_host_current->Remote_InitiatorKeyDistrib &
                        app_prefer_smp_format.para.pairing_request.InitiatorKeyDistribution;
                    if ((i & KEY_DISTRIBUTION_ENCKEY_1))
                    {
                        setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_ENCRYPTION_INFORMATION,
                                              (uint8_t *) p_host_current->OwnLTK, SIZE_AES_KEY);
                        setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_MASTER_IDENTIFICATION,
                                              (uint8_t *) p_host_current->OwnRAND, (SIZE_SMP_EDIV + SIZE_SMP_RAND));
                        p_host_current->KeyType = SMP_KEY_USE_LTK;
                    }
                    if ((i & KEY_DISTRIBUTION_IDKEY_1))
                    {
                        ble_gap_addr_t identity_addr;

                        setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_IDENTITY_INFORMATION,
                                              (uint8_t *) p_host_current->OwnIRK, SIZE_SMP_IRK);
                        ble_gap_device_identity_address_get(&identity_addr);
                        inverse_addr[6] = identity_addr.addr_type;
                        memcpy_inv((uint8_t *) &inverse_addr[0], (uint8_t *) &identity_addr.addr[0], BLE_ADDR_LEN);
                        setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_IDENTITY_ADDRESS_INFORMATION, &inverse_addr[0],
                                              (1 + BLE_ADDR_LEN));
                    }
                    if ((i & KEY_DISTRIBUTION_SIGN_1))
                    {
                        setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_SIGNING_INFORMATION, (uint8_t *) CSRK_FIXED,
                                              SIZE_SMP_CSRK);
                    }

                    p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
                    while (p_mblk == NULL)
                    {
                        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                        sys_task_delay(TASK_DELAY_MS);
                        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                        p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
                    }
                    p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
                    task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

                    Chk_BOND_StorageOrNot(host_id);
                }
            }
        }
        else
        {
            p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
            while (p_mblk == NULL)
            {
                BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                sys_task_delay(TASK_DELAY_MS);
                Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
            }
            p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = host_id;
            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

            Chk_BOND_StorageOrNot(host_id);
        }
        if (p_host_current->SMP_STKGenMethod !=
            STK_GEN_MTHD_JUST_WORKS) // STK_GEN_MTHD_PASSKEY_ENTRY or STK_GEN_MTHD_PASSKEY_ENTRY_DISP
        {
            p_host_current->SecurityMode = SMP_SECURITY_MODE_1_AUTHEN_PAIRING_W_ENCYPT;
        }
        else
        {
            p_host_current->SecurityMode = SMP_SECURITY_MODE_1_UNAUTHEN_PAIRING_W_ENCYPT;
        }
        p_host_current->IsEncryption = TRUE;
        p_host_current->Smp_Phase    = 0;
    }
    else
    {
        p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
        while (p_mblk == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
        }
        p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = bhc_query_host_id_by_conn_id(p_data->conn_handle);
        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S253;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
        p_host_current->Smp_Phase = 0;
    }
}

/** The BLE host processing encrypt key refresh event.
 *
 * @param[in]  p_data : pointer of the encrypt key refresh event data.
 *
 * @retval  true : successful.
 * @retval  false: the queue is full or memory not enough.
 */
int8_t bhc_prcss_encrpyt_key_refresh(ble_hci_evt_param_key_refresh_complete_t * p_data)
{
#if (_CONN_SUPPORT_ == 1)
    LE_Host_Para * p_host_current;
    MBLK * p_mblk;
    uint8_t host_id;
    int8_t err_code;

    err_code       = ERR_OK;
    host_id        = bhc_query_host_id_by_conn_id(p_data->conn_handle);
    p_host_current = &LE_Host[host_id];

    if (p_host_current->SMP_STKGenMethod != STK_GEN_MTHD_JUST_WORKS) // STK_GEN_MTHD_PASSKEY_ENTRY or
                                                                     // STK_GEN_MTHD_PASSKEY_ENTRY_DISP
    {
        p_host_current->SecurityMode = SMP_SECURITY_MODE_1_AUTHEN_PAIRING_W_ENCYPT;
    }
    else
    {
        p_host_current->SecurityMode = SMP_SECURITY_MODE_1_UNAUTHEN_PAIRING_W_ENCYPT;
    }
    p_host_current->IsEncryption = TRUE;
    p_host_current->Smp_Phase    = 0;

    p_mblk = get_msgblks(sizeof(MHS_ATT_Authe_Carry_On_Para));
    if (p_mblk != NULL)
    {
        p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id       = bhc_query_host_id_by_conn_id(p_data->conn_handle);
        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S0;
        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = TRUE;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

        Chk_BOND_StorageOrNot(host_id);
        ble_flashbond_cmd(CMD_FB_PSH_BACKUP_DATA_FLASH_PARA_BOND, (uint8_t *) smp_Para_Bond_tmp);
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
#endif // #if (_CONN_SUPPORT_ == 1)
}

/** The BLE host generate a random value for security.
 *
 * @param[in]  p_data : pointer of the random event data.
 * @param[in]  msg : encrpyt queue message.
 *
 */
int8_t bhc_gen_random_value(ble_hci_return_param_rand_t * p_data, encrypt_queue_t msg)
{
    ble_hci_le_encrypt_param_t param;
    int8_t err_code;

    err_code = ERR_OK;

    switch (msg.encrypt_state)
    {
    case STATE_GEN_RANDOM_NUMBER:
        memcpy(&param.key[0], p_data->random_number, 8);
        memcpy(&param.plaintext_data[0], r_AES_INI, SIZE_AES_KEY);

        if (hci_le_encrypt_cmd(&param) == ERR_OK)
        {
            msg.encrypt_state = STATE_GEN_RANDOM_VALUE;
            sys_queue_send(&g_host_encrypt_handle, &msg);
        }
        else
        {
            err_code = ERR_MEM;
        }
        break;

    case STATE_GEN_LOCAL_IRK: {
        encrypt_queue_t encrypt_msg;

        memcpy(param.key, &identity_resolving_seed[0], SIZE_SMP_IRK);
        memcpy(param.plaintext_data, p_data->random_number, SIZE_SMP_IRK);
        if (hci_le_encrypt_cmd(&param) == ERR_OK)
        {
            // send encrypt queue
            encrypt_msg.host_id       = msg.host_id;
            encrypt_msg.encrypt_state = STATE_GEN_LOCAL_IRK;
            sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
        }
        else
        {
            err_code = ERR_MEM;
        }
    }
    break;

    default:
        break;
    }
    return err_code;
}

/**
 * host send security request / pairing request
 *
 * @param[in]  host_id : the link's host id.
 * @param[in]  p_param : a pointer to connection parameter.
 *
 * @retval BLE_ERR_INVALID_STATE  : Invalid BLE state, usually happens in \n
 *                                               - BLE stack has not initialized or \n
 *                                               - there is no connection established with the host id \n
 *                                               - the link is already encrpted \n
 *                                               - there is the same procedure has been processed.
 * @retval BLE_BUSY     : The Security command timer is busy. Meaning there are still unfinished security events.
 * @retval BLE_BUSY     : Message queue buffer full.
 * @retval BLE_ERR_OK    : Setting success.
 *
 */
ble_err_t bhc_sm_security_request(uint8_t host_id)
{
    LE_Host_Para * p_le_current;
    ble_err_t err_code;

    err_code     = BLE_ERR_OK;
    p_le_current = &LE_Host[host_id];
    if ((p_le_current->IsEncryption == DISABLE) &&
        ((p_le_current->state_authe == HS_ATE_S0) || (p_le_current->state_authe == HS_ATE_S253)))
    {
        if (bhc_timer_evt_get(host_id, TIMER_EVENT_AUTH_STATUS) == TIMER_EVENT_NULL)
        {
            MBLK * p_mblk;

            p_mblk = get_msgblks_L1(sizeof(MHS_ATT_Authe_Carry_On_Para));
            if (p_mblk != NULL)
            {
                p_mblk->primitive = HOST_MSG_AUTHE_EVENT;

                p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id = host_id;
                if (p_le_current->state_authe == HS_ATE_S0)
                {
                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S4;
                }
                else
                {
                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S2;
                }
                task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);

                bhc_timer_set(host_id, TIMER_EVENT_AUTH_STATUS, 3000); // 30s/10ms = 3000
            }
            else
            {
                err_code = BLE_BUSY;
            }
        }
        else
        {
            err_code = BLE_BUSY;
        }
    }
    else
    {
        if (p_le_current->IsEncryption == ENABLE)
        {
            // the process has been done and state is in "IsEncryption"
        }
        else // if(LE_Host_Current->state_authe != HS_ATE_S0)
        {
            //  issue this command again if the same process has NOT been done
        }

        err_code = BLE_ERR_INVALID_STATE;
    }

    return err_code;
}

/**
 * set security parameters - io capabilities.
 *
 * @param[in]  p_param : a pointer of the io capability parameter.
 *
 */
void bhc_sm_io_caps_set(ble_evt_sm_io_cap_t * p_param)
{
    app_prefer_smp_format.para.pairing_request.IOCapability = p_param->io_caps_param;

    if (p_param->io_caps_param == IO_CAPABILITY_NOINPUT_NOOUTPUT)
    {
        app_prefer_smp_format.para.pairing_request.AuthReq.MITM = 0;
    }
    else
    {
        app_prefer_smp_format.para.pairing_request.AuthReq.MITM = 1;
    }
}

/**
 * set security parameters - bonding flag
 *
 * @param[in]  p_param : a pointer of the bonding flag parameter.
 *
 */
void bhc_sm_bonding_flag_set(ble_evt_sm_bonding_flag_t * p_param)
{
    switch (p_param->bonding_flag)
    {
    case NO_BONDING:
        app_prefer_smp_format.para.pairing_request.AuthReq.BondingFlags = 0;
        break;

    case BONDING:
        app_prefer_smp_format.para.pairing_request.AuthReq.BondingFlags = 1;
        break;

    default:
        break;
    }
}
