/*******************************************************************
 *
 * File Name  : host_manager.c
 * Description:
 *
 *******************************************************************/
#include "host_management.h"
#include "att.h"
#include "ble_api.h"
#include "ble_bonding.h"
#include "ble_event_module.h"
#include "ble_gap_api.h"
#include "ble_hci.h"
#include "ble_host_cmd.h"
#include "ble_memory.h"
#include "ble_printf.h"
#include "ble_privacy_api.h"
#include "ble_profile.h"
#include "cm3_mcu.h"
#include "hci_cmd_connect.h"
#include "hci_cmd_security.h"
#include "l2cap.h"
#include "lib_config.h"
#include "smp.h"
#include "sys_arch.h"
#include "task_ble_app.h"
#include <stdint.h>
#include <string.h>

#define TASK_DELAY_MS (2) /* task delay time (ms) */

LE_Host_Para * LE_Host;
LE_Host_Exist LE_Host_Active;
uint8_t smp_Para_Bond_tmp[128];
uint8_t acl_data_tx[251];
ble_host_timer_msg_t * host_timer = (ble_host_timer_msg_t *) &param_host_timer[0][0];

const LE_Host_Para LE_HOST_PARA_DEFAULT = {

    BLE_CONNID_RESERVED,  // LL_ConnID,
    0,                    // Role,
    0,                    // PeerAddrType,
    { 0, 0, 0, 0, 0, 0 }, // PeerAddr[LEN_BD_ADDR],
    0,                    // ConnInterval,
    0,                    // ConnLatency,
    0,                    // SvisionTimeout,
    0x0017,               // att_MTU, default 23
    0x0017,               // att_MTU_planed, default 23
    HS_ATE_S255,          // state_authe,
    HS_ATO_S255,          // state_autho,
    HS_DBP_S255,          // state_DB_parse,
    0,                    // state_att_MTU_opr
    0,                    // numHdl_fc_cl
    0,                    // numHdl_fc_sr
    (MBLK *) 0,           //*mblk_Param_L0
    (MBLK *) 0,           //*mblk_Param_L1
    (MBLK *) 0,           //*mblk_Q_4_Wr
    0,                    // state_pairing;
    { 0, 0, 0 },          // private_addr24[3];
    0,                    // Smp_Phase
    0,                    // IsEncryption
    0,                    // SecurityMode
    0,                    // Remote_IOCapab
    0,                    // Remote_OOBDataFlag
    0,                    // Remote_AuthReq
    0,                    // Remote_MaxEncKeySize
    0,                    // Remote_InitiatorKeyDistrib
    0,                    // Remote_ResponserKeyDistrib
    {
        0,
        0,
        0,
        0, // RandomValue[16]
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    {
        0,
        0,
        0,
        0, // Confirm_STK[16]
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    0xAA, // BOND_ROLE
    0,    // SMP_STKGenMethod
    0,    // MaxEncKeySize
    0xAA, // KeyType
    {
        0,
        0,
        0,
        0, // PeerRAND[8]
        0,
        0,
        0,
        0,
    },
    { 0, 0 }, // PeerEDIV[2]
    {
        0,
        0,
        0,
        0, // PeerLTK[16]
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    {
        0,
        0,
        0,
        0, // PeerIRK[16]
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    0,                    // IDAddrType,
    { 0, 0, 0, 0, 0, 0 }, // IDAddr[6],
    0,                    // OwnAddrType,
    { 0, 0, 0, 0, 0, 0 }, // OwnAddr[6],
    {
        0,
        0,
        0,
        0, // OwnRAND[8]
        0,
        0,
        0,
        0,
    },
    { 0, 0 }, // OwnEDIV[2]
    {
        0,
        0,
        0,
        0, // OwnLTK[16]
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    { IRK_PREDEF_FIXED },
};

const ble_att_handle_param_t ATTRIBUTE_BLE_HDL_PARA_DEFAULT = {
    0, // numHDL;
    0, // propertyValue;
    0, // value;
};

/**
 * SET BLE HOST TIMER
 *
 * @param[in]  host_id : the link's host id.
 * @param[in]  event : select the host timer event to set.
 * @param[in]  timeout_base : timeout counter (10ms).
 *
 * @retval  TRUE : setting success.
 * @retval  FALSE: event timer busy.
 *
 */
uint8_t bhc_timer_set(uint8_t host_id, host_timer_evt event, uint32_t timeout_base)
{
    uint8_t err_code;

    err_code = FALSE;
    if ((host_timer + event)[host_id * TOTAL_TIMER_EVENT].event == TIMER_EVENT_NULL)
    {
        (host_timer + event)[host_id * TOTAL_TIMER_EVENT].event        = event;
        (host_timer + event)[host_id * TOTAL_TIMER_EVENT].current_time = (uint32_t) pvTimerGetTimerID(g_ble_host_timer);
        (host_timer + event)[host_id * TOTAL_TIMER_EVENT].timeout_base = timeout_base;

        if (xTimerIsTimerActive(g_ble_host_timer) == pdFALSE)
        {
            if (xTimerStart(g_ble_host_timer, 0) != pdPASS)
            {
                BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] ble_host_timer start fail\n");
            }
        }
        err_code = TRUE;
    }

    return err_code;
}

/**
 * BLE HOST TIMER CLEAR
 *
 * @param[in]  host_id : the link's host id.
 * @param[in]  event : select the host timer event to clear.
 *
 */
void bhc_timer_clear(uint8_t host_id, host_timer_evt event)
{
    uint8_t i;

    if (event == TIMER_EVENT_NULL)
    {
        for (i = 0; i < TOTAL_TIMER_EVENT; i++)
        {
            (host_timer + i)[host_id * TOTAL_TIMER_EVENT].event = TIMER_EVENT_NULL; // close
        }
    }
    else if (event < TOTAL_TIMER_EVENT)
    {
        (host_timer + event)[host_id * TOTAL_TIMER_EVENT].event = TIMER_EVENT_NULL; // close
    }
}

/** Get BLE HOST TIMER EVENT STATUS
 *
 * @param[in]  host_id : the link's host id.
 * @param[in]  event : select the host timer event.
 *
 * @retval  host timer status.
 */
uint8_t bhc_timer_evt_get(uint8_t host_id, host_timer_evt event)
{
    return (host_timer + event)[host_id * TOTAL_TIMER_EVENT].event;
}

/*************************************************************************
 *
 * InitCmdTimer -
 * Description:
 *
 ************************************************************************/
void bhc_timer_init(void)
{
    uint8_t i;

    for (i = 0; i < MAX_CONN_NO_APP; i++)
    {
        bhc_timer_clear(i, TIMER_EVENT_NULL);
    }
}
/** host parameter initial
 *
 * @param[in]  host_id : host id of the link.
 *
 */
void bhc_host_param_init(uint8_t host_id)
{
    extern uint8_t * ble_flashbond_restore_key(uint8_t host_id);
    uint8_t i;

    LE_Host[host_id] = LE_HOST_PARA_DEFAULT;

    for (i = 0; i < att_db_mapping_size[host_id].size_map_client_db; i++)
    {
        att_db_mapping[host_id].map_client_db[i] = ATTRIBUTE_BLE_HDL_PARA_DEFAULT;
    }
    bhc_timer_clear(host_id, TIMER_EVENT_NULL); // Turn off the cmdTimer beacuse of disconnect.
    ble_flashbond_cmd(CMD_FB_CHK_IF_FLASHBOND_NEED_TO_ERASE_PAGE, (uint8_t *) smp_Para_Bond_tmp);
    ble_flashbond_restore_key(host_id);
}

/** query the host id by connection handle value
 *
 * @param[in]  conn_id : connection handle value.
 *
 * @return host id.
 */
uint8_t bhc_query_host_id_by_conn_id(uint16_t conn_id)
{
    uint8_t i;

    for (i = 0; i < max_num_conn_host; i++)
    {
        if (LE_Host[i].LL_ConnID)
        {
            if (LE_Host[i].LL_ConnID == conn_id)
            {
                break;
            }
        }
    }
    if (i == max_num_conn_host)
    {
        i = BLE_HOSTID_RESERVED;
    }

    return i;
}

void HS_Msg_ATT_DB_PARSE_CARRY_ON(uint8_t host_id)
{
    MBLK * p_mblk;

    p_mblk = get_msgblks(sizeof(MHS_ATT_DB_Parse_Carry_On_Para));
    while (p_mblk == NULL)
    {
        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
        sys_task_delay(TASK_DELAY_MS);
        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
        p_mblk = get_msgblks(sizeof(MHS_ATT_DB_Parse_Carry_On_Para));
    }
    p_mblk->primitive = HOST_MSG_DB_PARSING_EVENT;

    p_mblk->para.MHS_ATT_DB_Parse_Carry_On_Para.host_id = host_id;
    task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
}

void rls_mblk_resource_HS(MBLK * mblk)
{
    if (mblk != (MBLK *) NULL)
    {
        mem_free((void *) mblk);
    }
}

/** release the att queue write resource.
 *
 * @param[in]  host_id : host id of the link.
 *
 */
void bhc_att_queue_write_resource(uint8_t host_id)
{
    if (LE_Host[host_id].mblk_Q_4_Wr != NULL)
    {
        mem_free(LE_Host[host_id].mblk_Q_4_Wr);
        LE_Host[host_id].mblk_Q_4_Wr = (MBLK *) 0;
    }
}

/** release the db parsing resource.
 *
 * @param[in]  host_id : host id of the link.
 *
 */
void bhc_host_release_db_parsing_resource(uint8_t host_id)
{
    rls_mblk_resource_HS(LE_Host[host_id].mblk_Param_L0);
    rls_mblk_resource_HS(LE_Host[host_id].mblk_Param_L1);
    LE_Host[host_id].mblk_Param_L0 = (MBLK *) NULL;
    LE_Host[host_id].mblk_Param_L1 = (MBLK *) NULL;
}

uint8_t chkATT_DB_Param_Mem4Prcss_Rdy(uint8_t host_id, uint16_t conn_id)
{
    uint8_t err_code;

    err_code = TRUE;
    if (host_id == BLE_HOSTID_RESERVED)
    {
        host_id = bhc_query_host_id_by_conn_id(conn_id);
    }
    else
    {
        conn_id = LE_Host[host_id].LL_ConnID;
    }

    if (((LE_Host[host_id].mblk_Param_L0 == (MBLK *) 0) || (LE_Host[host_id].mblk_Param_L1 == (MBLK *) 0)))
    {
        err_code = FALSE;
    }

    return err_code;
}

void bhc_db_parsing_evt_handle(void * pMBlk)
{
    MBLK * mblk;
    LE_Host_Para * p_host_current;
    uint8_t host_id;
    uint8_t evtGen;

    mblk           = (MBLK *) pMBlk;
    host_id        = mblk->para.MHS_ATT_DB_Parse_Carry_On_Para.host_id;
    p_host_current = &LE_Host[host_id];

    evtGen = 0;
    switch (p_host_current->state_DB_parse)
    {
    case HS_DBP_S0:
        break;

    case HS_DBP_S1:
        if (chkATT_DB_Param_Mem4Prcss_Rdy(host_id, 0) == TRUE)
        {
            if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_READ_BY_GROUP_TYPE_REQUEST,
                            (uint8_t *) &p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting, 6) != ERR_OK)
            {
                evtGen = 1;
            }
        }
        else
        {
            mblk = mem_malloc(sizeof(MBLK));
            if (mblk != (MBLK *) 0)
            {
                if (p_host_current->mblk_Param_L0 == (MBLK *) 0)
                {
                    p_host_current->mblk_Param_L0                                               = mblk;
                    p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = 0x0001;
                    p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Ending   = 0xFFFF;
                    p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.typeUUID     = GATT_DECL_PRIMARY_SERVICE;
                }
                else if (p_host_current->mblk_Param_L1 == (MBLK *) 0)
                {
                    p_host_current->mblk_Param_L1                                               = mblk;
                    p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = 0x0001;
                    p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Ending   = 0xFFFF;
                    p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.typeUUID     = GATT_DECL_CHARACTERISTIC;
                }
                else
                {
                    mem_free(mblk);
                }
            }
            evtGen = 1;
        }
        break;

    case HS_DBP_S2:
        if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_READ_BY_TYPE_REQUEST,
                        (uint8_t *) &p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting, 6) != ERR_OK)
        {
            evtGen = 1;
        }
        break;

    case HS_DBP_S3:
        if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_FIND_INFORMATION_REQUEST,
                        (uint8_t *) &p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB, 4) != ERR_OK)
        {
            evtGen = 1;
        }
        break;

    case HS_DBP_S252:
        break;

    case HS_DBP_S253:
        break;

    case HS_DBP_S254:
        break;

    default:
        evtGen = 1;
        break;
    }

    if (evtGen)
    {
        HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
    }
}

void bhc_authentication_evt_handle(void * pMBlk)
{
#if (_CONN_CENTRAL_ROLE_ == 1)
    uint8_t * tmp;
#endif //(#if (_CONN_CENTRAL_ROLE_ == 1))
    MBLK * p_mblk;
    LE_Host_Para * p_host_current;
    uint8_t host_id;

    p_mblk         = (MBLK *) pMBlk;
    host_id        = p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id;
    p_host_current = &LE_Host[host_id];

    switch (p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State)
    {
    case HS_ATE_S0:
        if (p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status == TRUE)
        {
            ble_host_to_app_evt_t * p_queue_param;
            ble_evt_param_t * p_evt_param;

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
            p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.status  = 0;
            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

            bhc_timer_clear(host_id, TIMER_EVENT_AUTH_STATUS);
        }
        p_host_current->state_authe = HS_ATE_S0;

        p_mblk = get_msgblks(sizeof(MHS_ATT_Autho_Carry_On_Para));
        while (p_mblk == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_mblk = get_msgblks(sizeof(MHS_ATT_Autho_Carry_On_Para));
        }
        p_mblk->primitive = HOST_MSG_AUTHO_EVENT;

        p_mblk->para.MHS_ATT_Autho_Carry_On_Para.host_id = host_id;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
        break;

    case HS_ATE_S1:
        if (p_host_current->state_authe == HS_ATE_S1)
        {
            if ((p_host_current->Smp_Phase == 0) && (p_host_current->IsEncryption == 0))
            {
                if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
                {
#if (_CONN_CENTRAL_ROLE_ == 1)
                    smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;
                    tmp = ble_flashbond_cmd(CMD_FB_GET_EXIST_PID_BY_HOST_ID, (uint8_t *) smp_Para_Bond_tmp);
                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                    {
                        tmp = ble_flashbond_cmd(CMD_FB_GET_KEY_FLASHBOND_PARA_BOND, (uint8_t *) smp_Para_Bond_tmp);
                        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                        {
                            uint8_t addrSubType;
                            uint8_t peerAddr[7], cmp0[16], cmpFF[16];

                            memset(cmp0, 0, SIZE_SMP_IRK);
                            memset(cmpFF, 0xFF, SIZE_SMP_IRK);
                            if ((memcmp(p_host_current->OwnIRK, param_rpa[host_id].irk, SIZE_SMP_IRK) == 0) ||
                                (memcmp(p_host_current->OwnIRK, cmp0, SIZE_SMP_IRK) == 0) ||
                                (memcmp(p_host_current->OwnIRK, cmpFF, SIZE_SMP_IRK) == 0))
                            {
                                memcpy_inv(peerAddr, &p_host_current->PeerAddrType, BLE_ADDR_LEN + 1);
                                addrSubType = peerAddr[0] & BLE_ADDR_SUB_TYPE_FLD_RANDOM;

                                if ((memcmp(&p_host_current->PeerAddrType, (tmp + TAB_PARA_DATA_INI_ADDR), 7) == 0) ||
                                    (memcmp(&p_host_current->IDAddrType, &peerAddr[0], BLE_ADDR_LEN) == 0) ||
                                    ((addrSubType == BLE_ADDR_SUB_TYPE_RANDOM_RESOLVABLE) &&
                                     (memcmp(p_host_current->PeerAddr, p_host_current->private_addr24, 3) == 0)))
                                {
                                    ble_hci_le_enable_encrypt_param_t param;

                                    memcpy(&p_host_current->BOND_Role, (tmp + TAB_PARA_DATA_BOND_ROLE), SMP_PARA_BOND_SIZE - 7);

                                    param.conn_handle = p_host_current->LL_ConnID;
                                    if (p_host_current->BOND_Role == BLE_GAP_ROLE_CENTRAL)
                                    {
                                        memcpy(&param.random_number[0], &p_host_current->PeerRAND[0],
                                               (SIZE_SMP_RAND + SIZE_SMP_EDIV));
                                        memcpy_inv(&param.long_term_key[0], &p_host_current->PeerLTK_Passkey[0], SIZE_AES_KEY);
                                    }
                                    else
                                    {
                                        memcpy(&param.random_number[0], &p_host_current->OwnRAND[0],
                                               (SIZE_SMP_RAND + SIZE_SMP_EDIV));
                                        memcpy_inv(&param.long_term_key[0], &p_host_current->OwnLTK[0], SIZE_AES_KEY);
                                    }
                                    if (hci_le_enable_encryption_cmd(&param) == ERR_OK)
                                    {
                                        p_host_current->Smp_Phase   = 2;
                                        p_host_current->state_authe = HS_ATE_S3;
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

                                        p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id = host_id;
                                        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
                                    }
                                    break;
                                }
                            }
                        }
                    }
#endif //(#if (_CONN_CENTRAL_ROLE_ == 1))
                }
            }
            else
            {
                p_host_current->state_authe = HS_ATE_S3;
                break;
            }
            p_host_current->state_authe = HS_ATE_S0;

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
            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.report_status = FALSE;
            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
        }
        break;

    case HS_ATE_S2:
        if ((p_host_current->Smp_Phase == 0) && (p_host_current->IsEncryption == 0))
        {
            if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
            {
#if (_CONN_CENTRAL_ROLE_ == 1)
                memcpy_inv(&p_host_current->Confirm_STK[0], &app_prefer_smp_format.para.pairing_request.IOCapability, 6);
                setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_PAIRING_REQUEST, p_host_current->Confirm_STK, 6);
                p_host_current->state_authe = HS_ATE_S3;
#endif //(#if (_CONN_CENTRAL_ROLE_ == 1))
            }
            else
            {
#if (_CONN_PHERIPHERAL_ROLE_ == 1)
                setBLE_ConnTxData_SMP(p_host_current->LL_ConnID, CODE_SMP_SECURITY_REQUEST,
                                      (uint8_t *) &app_prefer_smp_format.para.pairing_response.AuthReq, 1);
                p_host_current->state_authe = HS_ATE_S3;
#endif //(#if (_CONN_PHERIPHERAL_ROLE_ == 1))
            }
        }
        else
        {
            p_host_current->state_authe = HS_ATE_S3;
        }
        break;

    case HS_ATE_S4:
        if (p_host_current->Smp_Phase == 0)
        {
            if (p_host_current->Role == BLE_GAP_ROLE_CENTRAL)
            {
#if (_CONN_CENTRAL_ROLE_ == 1)
                smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;
                tmp = ble_flashbond_cmd(CMD_FB_GET_EXIST_PID_BY_HOST_ID, (uint8_t *) smp_Para_Bond_tmp);
                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                {
                    tmp = ble_flashbond_cmd(CMD_FB_GET_KEY_FLASHBOND_PARA_BOND, (uint8_t *) smp_Para_Bond_tmp);
                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                    {
                        uint8_t addrSubType;
                        uint8_t peerAddr[7], cmp0[16], cmpFF[16];

                        memset(cmp0, 0, SIZE_SMP_IRK);
                        memset(cmpFF, 0, SIZE_SMP_IRK);
                        if ((memcmp(p_host_current->OwnIRK, param_rpa[host_id].irk, SIZE_SMP_IRK) == 0) ||
                            (memcmp(p_host_current->OwnIRK, cmp0, SIZE_SMP_IRK) == 0) ||
                            (memcmp(p_host_current->OwnIRK, cmpFF, SIZE_SMP_IRK) == 0))
                        {
                            memcpy_inv(peerAddr, &p_host_current->PeerAddrType, BLE_ADDR_LEN + 1);
                            addrSubType = peerAddr[0] & BLE_ADDR_SUB_TYPE_FLD_RANDOM;

                            if ((memcmp(&p_host_current->PeerAddrType, (tmp + TAB_PARA_DATA_INI_ADDR), 7) == 0) ||
                                (memcmp(&p_host_current->IDAddrType, &peerAddr[0], BLE_ADDR_LEN) == 0) ||
                                ((addrSubType == BLE_ADDR_SUB_TYPE_RANDOM_RESOLVABLE) &&
                                 (memcmp(p_host_current->PeerAddr, p_host_current->private_addr24, 3) == 0)))
                            {
                                ble_hci_le_enable_encrypt_param_t param;

                                memcpy(&p_host_current->BOND_Role, (tmp + TAB_PARA_DATA_BOND_ROLE), SMP_PARA_BOND_SIZE - 7);

                                param.conn_handle = p_host_current->LL_ConnID;

                                if (p_host_current->BOND_Role == BLE_GAP_ROLE_CENTRAL)
                                {
                                    memcpy(&param.random_number[0], &p_host_current->PeerRAND[0], (SIZE_SMP_RAND + SIZE_SMP_EDIV));
                                    memcpy_inv(&param.long_term_key[0], &p_host_current->PeerLTK_Passkey[0], SIZE_AES_KEY);
                                }
                                else
                                {
                                    memcpy(&param.random_number[0], &p_host_current->OwnRAND[0], (SIZE_SMP_RAND + SIZE_SMP_EDIV));
                                    memcpy_inv(&param.long_term_key[0], &p_host_current->OwnLTK[0], SIZE_AES_KEY);
                                }
                                if (hci_le_enable_encryption_cmd(&param) == ERR_OK)
                                {
                                    p_host_current->Smp_Phase   = 2;
                                    p_host_current->state_authe = HS_ATE_S3;
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

                                    p_mblk->para.MHS_ATT_Authe_Carry_On_Para.host_id = host_id;
                                    task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
                                }
                                break;
                            }
                        }
                    }
                }
#endif //(#if (_CONN_CENTRAL_ROLE_ == 1))
            }
            p_host_current->state_authe = HS_ATE_S2;

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
            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S2;
            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
        }
        else
        {
            p_host_current->state_authe = HS_ATE_S3;
        }
        break;

    case HS_ATE_S253:
        p_host_current->state_authe = HS_ATE_S253;
        HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
        p_host_current->Smp_Phase = 0;
        break;

    default:
        break;
    }
}

void bhc_authorization_evt_handle(void * pMBlk)
{
    MBLK * p_mblk;
    LE_Host_Para * p_host_current;
    uint8_t host_id;

    p_mblk         = (MBLK *) pMBlk;
    host_id        = p_mblk->para.MHS_ATT_Autho_Carry_On_Para.host_id;
    p_host_current = &LE_Host[host_id];

    switch (p_host_current->state_autho)
    {
    case HS_ATO_S0: // ready
        HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
        break;

    case HS_ATO_S1:                              // Authorization_data restore
        p_host_current->state_autho = HS_ATO_S0; // There is no Authorization support now.
        HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
        break;

    case HS_ATO_S2: // authorization procedure begins if no previous authorization record
        break;

    case HS_ATO_S252: // authorization procedure failed
        break;

    default:
        p_mblk = get_msgblks(sizeof(MHS_ATT_Autho_Carry_On_Para));
        while (p_mblk == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_mblk = get_msgblks(sizeof(MHS_ATT_Autho_Carry_On_Para));
        }
        p_mblk->primitive = HOST_MSG_AUTHO_EVENT;

        p_mblk->para.MHS_ATT_Autho_Carry_On_Para.host_id = host_id;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
        break;
    }
}

uint16_t queryHdlNumArrayPos(uint8_t host_id, uint16_t conn_id, ble_gatt_role_t role, ble_att_param_t * ATTRIBUTE_element)
{
    uint16_t i;
    uint16_t sizeDB;
    ble_att_param_t ** ATT_SERVER_content;
    uint16_t result;

    result = 0;
    do
    {
        if (host_id == BLE_HOSTID_RESERVED)
        {
            host_id = bhc_query_host_id_by_conn_id(conn_id);
        }
        if (host_id >= max_num_conn_host)
        {
            break;
        }
        if (role == BLE_GATT_ROLE_CLIENT)
        {
            if (att_db_link[host_id].p_client_db == (const ble_att_param_t **) 0)
            {
                break;
            }
            ATT_SERVER_content = (ble_att_param_t **) (att_db_link[host_id].p_client_db);
            sizeDB             = att_db_mapping_size[host_id].size_map_client_db;
        }
        else
        {
            if (att_db_link[host_id].p_server_db == (const ble_att_param_t **) 0)
            {
                break;
            }
            ATT_SERVER_content = (ble_att_param_t **) (att_db_link[host_id].p_server_db);
            sizeDB             = att_db_mapping_size[host_id].size_map_server_db;
        }
        for (i = 0; i < sizeDB; i++)
        {
            if (ATTRIBUTE_element == (ble_att_param_t *) (*(ATT_SERVER_content + i)))
            {
                break;
            }
        }
        if (i != sizeDB)
        {
            result = i;
        }
    } while (0);

    return result;
}

uint16_t queryArrayPos_By_HdlNum_Client(uint8_t host_id, uint16_t hdlNum)
{
    uint16_t i;
    uint16_t sizeDB;
    uint16_t result;

    result = 0;
    do
    {
        if (host_id >= max_num_conn_host)
        {
            break;
        }
        if (att_db_link[host_id].p_client_db == (const ble_att_param_t **) 0)
        {
            break;
        }
        sizeDB = att_db_mapping_size[host_id].size_map_client_db;
        for (i = 0; i < sizeDB; i++)
        {
            if (att_db_mapping[host_id].map_client_db[i].handle_num == hdlNum)
            {
                break;
            }
        }
        if (i != sizeDB)
        {
            result = i;
        }
    } while (0);

    return result;
}

uint16_t queryConnID_by_HSID(uint8_t host_id)
{
    return (LE_Host[host_id].LL_ConnID);
}

uint8_t chkConnIdExist_by_ConnID(uint16_t connID)
{
    return (connID == BLE_CONNID_RESERVED ? FALSE : TRUE);
}

uint8_t chkConnIdExist_by_HSID(uint8_t host_id)
{
    uint16_t conn_handle;

    conn_handle = queryConnID_by_HSID(host_id);

    return (conn_handle == BLE_CONNID_RESERVED ? FALSE : TRUE);
}

void bhc_att_param_init(void)
{
    uint8_t i;
    extern uint8_t * param_rsv_host[][(SIZE_LE_HOST_PARA >> 2)];
    extern l2cap_smp_pairing_req_rsp_t app_prefer_smp_format;
    extern const l2cap_smp_pairing_req_rsp_t default_pairing_response_format;
    extern uint8_t * ble_flashbond_restore_key(uint8_t host_id);

    param_rpa             = (ble_irk_rpa_addr_t *) &param_host_identity[0][0];
    LE_Host               = (LE_Host_Para *) &param_rsv_host[0];
    app_prefer_smp_format = default_pairing_response_format;
    memcpy(&identity_resolving_seed[0], (uint8_t *) IRK_FIXED, sizeof(param_rpa->irk));
    for (i = 0; i < max_num_conn_host; i++)
    {
        bhc_host_param_init(i);
        ble_flashbond_restore_key(i);
    }
    LE_Host_Active.Host_central    = (LE_Host_Para *) 0;
    LE_Host_Active.Host_Peripheral = (LE_Host_Para *) 0;

    bhc_timer_init();
}

uint8_t chkATT_DB(uint8_t host_id, uint16_t conn_id)
{
    uint8_t state;
    MBLK * p_mblk;
    LE_Host_Para * p_host_current;

    if (host_id == BLE_HOSTID_RESERVED)
    {
        host_id = bhc_query_host_id_by_conn_id(conn_id);
    }
    else
    {
        conn_id = LE_Host[host_id].LL_ConnID;
    }
    p_host_current = &LE_Host[host_id];

    state = TRUE;
    if (p_host_current->state_DB_parse == HS_DBP_S255)
    {
        if (att_db_link[host_id].p_client_db == (const ble_att_param_t **) 0)
        {
            p_host_current->state_authe    = HS_ATE_S0;
            p_host_current->state_autho    = HS_ATO_S0;
            p_host_current->state_DB_parse = HS_DBP_S0;
        }
        else
        {
            p_host_current->state_DB_parse = HS_DBP_S1;
            p_host_current->state_authe    = HS_ATE_S1;
            p_host_current->state_autho    = HS_ATO_S1;

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
            p_mblk->para.MHS_ATT_Authe_Carry_On_Para.Command_State = HS_ATE_S1;
            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
        }
    }
    else
    {
        if ((p_host_current->state_DB_parse == HS_DBP_S0) && (att_db_link[host_id].p_client_db != (const ble_att_param_t **) 0))
        {
            p_host_current->state_DB_parse = HS_DBP_S1;
            if ((p_host_current->state_authe == HS_ATE_S0) && (p_host_current->state_autho == HS_ATO_S0))
            {
                HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
            }
        }
        else
        {
            state = FALSE;
        }
    }

    return state;
}

uint8_t chkPermission_ATT_DataTansfer(uint8_t host_id, uint16_t conn_id)
{
    if (host_id == BLE_HOSTID_RESERVED)
    {
        host_id = bhc_query_host_id_by_conn_id(conn_id);
    }

    if ((LE_Host[host_id].state_DB_parse != HS_DBP_S0) ||
        ((LE_Host[host_id].state_authe != HS_ATE_S0) && (LE_Host[host_id].state_authe != HS_ATE_S253)) ||
        (LE_Host[host_id].state_autho != HS_ATO_S0))
    {
        return FALSE;
    }

    return TRUE;
}

void host_id_set_active_peripheral(uint8_t host_id)
{
    if (host_id != BLE_HOSTID_RESERVED)
    {
        LE_Host_Active.Host_Peripheral = &LE_Host[host_id];
    }
}

void host_id_set_active_central(uint8_t host_id)
{
    if (host_id != BLE_HOSTID_RESERVED)
    {
        LE_Host_Active.Host_central = &LE_Host[host_id];
    }
}

void bhc_bonding_storage_init(void)
{
    uint8_t * tmp;

    tmp = ble_flashbond_cmd(CMD_FB_CHK_IF_FLASH_INITED, (uint8_t *) smp_Para_Bond_tmp);
    if (*(tmp + 0) == FLH_BND_ERR_CODE_FLASH_NOT_INI)
    {
        tmp = ble_flashbond_cmd(CMD_FB_INIT_INFO_FLASHBOND, (uint8_t *) smp_Para_Bond_tmp);
    }
    else
    {
        ble_flashbond_cmd(CMD_FB_CHK_IF_FLASHBOND_NEED_TO_ERASE_PAGE, (uint8_t *) smp_Para_Bond_tmp);
    }
}

void Chk_BOND_StorageOrNot(uint8_t host_id)
{
    uint8_t * tmp;
    LE_Host_Para * p_host_current;

    p_host_current = &LE_Host[host_id];
    if (((p_host_current->Remote_AuthReq & app_prefer_smp_format.para.pairing_response.AuthReq.BondingFlags) &
         AUTHREQ_BONDING_FLAGS_BONDING) == AUTHREQ_BONDING_FLAGS_BONDING)
    {
        p_host_current->BOND_Role               = p_host_current->Role;
        smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;
        tmp                                     = ble_flashbond_cmd(CMD_FB_GET_EXIST_PID_BY_HOST_ID, (uint8_t *) smp_Para_Bond_tmp);
        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
        {
            tmp = ble_flashbond_cmd(CMD_FB_GET_KEY_FLASHBOND_PARA_BOND, (uint8_t *) smp_Para_Bond_tmp);
            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
            {
                if (memcmp(&p_host_current->BOND_Role, (tmp + TAB_PARA_DATA_BOND_ROLE), (SMP_PARA_BOND_SIZE - 7)) != 0)
                {
                    memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_INI_ADDR], &p_host_current->PeerAddrType, 7);
                    memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_BOND_ROLE], &p_host_current->BOND_Role, (SMP_PARA_BOND_SIZE - 7));
                    memcpy_inv(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_RAND], &p_host_current->OwnRAND[0], SIZE_SMP_RAND);
                    memcpy_inv(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_EDIV], &p_host_current->OwnEDIV[0], SIZE_SMP_EDIV);
                    memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_IRK], &p_host_current->OwnIRK[0], SIZE_SMP_IRK);
                    ble_flashbond_fillwithPID((uint8_t *) smp_Para_Bond_tmp);
                }
            }
            else
            {
                memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_INI_ADDR], &p_host_current->PeerAddrType, 7);
                memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_BOND_ROLE], &p_host_current->BOND_Role, (SMP_PARA_BOND_SIZE - 7));
                memcpy_inv(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_RAND], &p_host_current->OwnRAND[0], SIZE_SMP_RAND);
                memcpy_inv(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_EDIV], &p_host_current->OwnEDIV[0], SIZE_SMP_EDIV);
                memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_IRK], &p_host_current->OwnIRK[0], SIZE_SMP_IRK);
                ble_flashbond_fillwithPID((uint8_t *) smp_Para_Bond_tmp);
            }
        }
        else
        {
            memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_INI_ADDR], &p_host_current->PeerAddrType, 7);
            memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_BOND_ROLE], &p_host_current->BOND_Role, (SMP_PARA_BOND_SIZE - 7));
            memcpy_inv(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_RAND], &p_host_current->OwnRAND[0], SIZE_SMP_RAND);
            memcpy_inv(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_EDIV], &p_host_current->OwnEDIV[0], SIZE_SMP_EDIV);
            memcpy(&smp_Para_Bond_tmp[TAB_PARA_DATA_OWN_IRK], &p_host_current->OwnIRK[0], SIZE_SMP_IRK);
            ble_flashbond_fillwithPID((uint8_t *) smp_Para_Bond_tmp);
        }
    }
}

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/** Check host id is valid or not.
 */
bool bhc_host_id_is_valid_check(uint8_t host_id)
{
    if ((host_id == BLE_HOSTID_RESERVED) || (host_id >= max_num_conn_host))
    {
        return FALSE;
    }

    return TRUE;
}

/** Check host id is connected to the device or not.
 */
bool bhc_host_id_is_connected_check(uint8_t host_id, uint16_t * conn_id)
{
    *conn_id = 0xFF;

    // check connection exist or not
    (*conn_id) = queryConnID_by_HSID(host_id);

    if (chkConnIdExist_by_ConnID((*conn_id)) == FALSE)
    {
        return FALSE;
    }

    return TRUE;
}

/** Check host DB parsing or encryption process is finished or not.
 */
bool bhc_host_parsing_process_is_finished_check(uint8_t host_id)
{
    if (chkPermission_ATT_DataTansfer(host_id, 0) == FALSE)
    {
        return FALSE;
    }
    return TRUE;
}

/** Check the host is in encryption process or not.
 */
bool bhc_host_is_in_encryption_check(uint8_t host_id)
{
    LE_Host_Para * p_current_host;

    p_current_host = &LE_Host[host_id];
    if (p_current_host->Smp_Phase)
    {
        return TRUE;
    }

    return FALSE;
}

/** Check the GATT response is received or not.
 */
bool bhc_host_is_wating_gatt_rsp_check(uint8_t host_id)
{
    LE_Host_Para * p_current_host;

    p_current_host = &LE_Host[host_id];
    if (p_current_host->numHdl_fc_cl != 0)
    {
        return TRUE;
    }

    return FALSE;
}

/** Check the BLE Client Characteristic Properties Definition is defined or not.
 */
bool bhc_client_property_value_is_match_check(uint8_t host_id, uint16_t handle_num, uint8_t property)
{
    uint16_t idx;
    ble_att_handle_param_t * p_att_handle_param;

    idx                = queryArrayPos_By_HdlNum_Client(host_id, handle_num);
    p_att_handle_param = &att_db_mapping[host_id].map_client_db[idx];

    // check GATT role
    if ((att_db_link[host_id].p_client_db == (const ble_att_param_t **) 0) ||
        ((p_att_handle_param->property_value & property) == 0))
    {
        return FALSE;
    }

    return TRUE;
}

/** Check the BLE Server Characteristic Properties Definition is defined or not.
 */
bool bhc_server_property_value_is_match_check(uint8_t host_id, uint16_t handle_num, uint8_t property)
{
    // check GATT role
    if (att_db_link[host_id].p_server_db == (const ble_att_param_t **) 0)
    {
        return FALSE;
    }

    if ((property == GATT_DECLARATIONS_PROPERTIES_NOTIFY) &&
        (att_db_link[host_id].p_server_db[handle_num]->property_value & GATT_DECLARATIONS_PROPERTIES_NOTIFY) == 0)
    {
        return FALSE;
    }

    if ((property == GATT_DECLARATIONS_PROPERTIES_INDICATE) &&
        (att_db_link[host_id].p_server_db[handle_num]->property_value & GATT_DECLARATIONS_PROPERTIES_INDICATE) == 0)
    {
        return FALSE;
    }

    return TRUE;
}

/** Set GATT preferred ATT_MTU size.
 */
bool bhc_gatt_preferred_mtu_set(uint8_t host_id, uint16_t preferred_mtu)
{
    bool statue = TRUE;

    vPortEnterCritical();
    if (LE_Host[host_id].LL_ConnID == 0)
    {
        LE_Host[host_id].att_MTU_planed = preferred_mtu;
    }
    else
    {
        statue = FALSE;
    }
    vPortExitCritical();

    return statue;
}

/** Get BLE GATT Attribute Handles Mapping Table.
 */
bool bhc_gatt_att_handle_mapping_get(uint8_t host_id, ble_gatt_role_t role, ble_att_param_t * p_att_element,
                                     void * p_handle_num_addr)
{
    uint16_t db_size;
    uint16_t index;
    uint16_t * addrTgt;
    uint16_t idxTgt;
    ble_att_param_t ** p_att_param;

    if (bhc_host_id_is_valid_check(host_id) == FALSE)
    {
        return FALSE;
    }

    // check handle
    index = queryHdlNumArrayPos(host_id, 0, role, p_att_element);
    if (index == 0)
    {
        return FALSE;
    }

    idxTgt  = 0;
    addrTgt = (uint16_t *) p_handle_num_addr;
    if (role == BLE_GATT_ROLE_CLIENT)
    {
        db_size     = att_db_mapping_size[host_id].size_map_client_db;
        p_att_param = (ble_att_param_t **) (att_db_link[host_id].p_client_db);
    }
    else
    {
        db_size     = att_db_mapping_size[host_id].size_map_server_db;
        p_att_param = (ble_att_param_t **) (att_db_link[host_id].p_server_db);
    }

    while (1)
    {
        index++;
        if (index == db_size)
        {
            break;
        }
        switch (*((uint16_t *) (*(p_att_param + index))->p_uuid_type))
        {
        case GATT_DECL_PRIMARY_SERVICE:
            index = db_size - 1;
        case GATT_DECL_CHARACTERISTIC:
        case GATT_DECL_SECONDARY_SERVICE:
        case GATT_DECL_INCLUDE:
            break;

        default:
            if (role == BLE_GATT_ROLE_CLIENT)
            {
                *(addrTgt + idxTgt) = att_db_mapping[host_id].map_client_db[index].handle_num;
            }
            else
            {
                *(addrTgt + idxTgt) = index;
            }
            idxTgt++;
            break;
        }
    }

    return TRUE;
}

/** Get the number of BLE connected links..
 */
uint8_t bhc_host_connected_link_number_get(void)
{
    uint8_t i;
    uint8_t count;

    count = 0;
    for (i = 0; i < max_num_conn_host; i++)
    {
        if (chkConnIdExist_by_HSID(i) == TRUE)
        {
            count++;
        }
    }
    return count;
}

/** get the active host id.
 */
uint8_t bhc_host_id_state_active_get(uint8_t role)
{
    uint8_t i;
    uint8_t host_id;

    host_id = BLE_HOSTID_RESERVED;
    switch (role)
    {
    case BLE_GAP_ROLE_CENTRAL:
        for (i = 0; i < max_num_conn_host; i++)
        {
            if (LE_Host_Active.Host_central == &LE_Host[i])
            {
                host_id = i;
                break;
            }
        }
        break;

    case BLE_GAP_ROLE_PERIPHERAL:
        for (i = 0; i < max_num_conn_host; i++)
        {
            if (LE_Host_Active.Host_Peripheral == &LE_Host[i])
            {
                host_id = i;
                break;
            }
        }
        break;

    default:
        break;
    }

    return host_id;
}

/** The BLE host id is set to active mode.
 */
void bhc_host_id_state_active_set(uint8_t host_id, uint8_t role)
{
    if (host_id != BLE_HOSTID_RESERVED)
    {
        switch (role)
        {
        case BLE_GAP_ROLE_CENTRAL:
            LE_Host_Active.Host_central = &LE_Host[host_id];
            break;
        case BLE_GAP_ROLE_PERIPHERAL:
            LE_Host_Active.Host_Peripheral = &LE_Host[host_id];
            break;

        default:
            break;
        }
    }
}

/** The BLE host id is set to idle mode.
 */
void bhc_host_id_state_active_release(uint8_t role)
{
    switch (role)
    {
    case BLE_GAP_ROLE_CENTRAL:
        LE_Host_Active.Host_central = (LE_Host_Para *) 0;
        break;
    case BLE_GAP_ROLE_PERIPHERAL:
        LE_Host_Active.Host_Peripheral = (LE_Host_Para *) 0;
        break;

    default:
        break;
    }
}

/** The BLE host processing connection completion event.
 */
void bhc_connection_complete_handle(ble_hci_le_meta_evt_param_conn_complete_t * p_data)
{
    LE_Host_Para * p_host_current;
    encrypt_queue_t encrypt_msg;
    uint8_t host_id;
    uint32_t timeout_value;

    /* handle connection completed. */
    if (p_data->role == BLE_GAP_ROLE_CENTRAL)
    {
        p_host_current              = LE_Host_Active.Host_central;
        LE_Host_Active.Host_central = (LE_Host_Para *) 0;
    }
    else
    {
        p_host_current                 = LE_Host_Active.Host_Peripheral;
        LE_Host_Active.Host_Peripheral = (LE_Host_Para *) 0;
    }

    p_host_current->LL_ConnID = p_data->conn_handle;
    memcpy((uint8_t *) &p_host_current->Role, &p_data->role, 14);

    ble_gap_device_address_get((ble_gap_addr_t *) &p_host_current->OwnAddrType);
    host_id = bhc_query_host_id_by_conn_id(p_host_current->LL_ConnID);

    do
    {
        uint8_t addrSubType;
        uint8_t peerAddr[6];

        if ((ble_privacy_parameter_enable_check() == FALSE) || (ble_privacy_host_id_get() != host_id))
        {
            chkATT_DB(BLE_HOSTID_RESERVED, p_host_current->LL_ConnID);
            break;
        }
        memcpy_inv(peerAddr, p_host_current->PeerAddr, BLE_ADDR_LEN);
        addrSubType = peerAddr[0] & BLE_ADDR_SUB_TYPE_FLD_RANDOM;

        if (ble_privacy_parameter_LL_privacy_on_check() == FALSE)
        {
            if (p_host_current->PeerAddrType != PUBLIC_ADDR)
            {
                if (addrSubType == BLE_ADDR_SUB_TYPE_RANDOM_RESOLVABLE)
                {
                    memcpy(peerAddr, &p_host_current->PeerAddr[3], 3);
                    private_address_resolve_by_id(host_id, peerAddr);
                    break;
                }
            }
            if (ble_privacy_parameter_device_mode_check() == TRUE)
            {
                if (p_host_current->PeerAddrType == PUBLIC_ADDR)
                {
                    if (p_host_current->IDAddrType == PUBLIC_ADDR)
                    {
                        if (memcmp(p_host_current->IDAddr, peerAddr, BLE_ADDR_LEN) == 0)
                        {
                            chkATT_DB(BLE_HOSTID_RESERVED, p_host_current->LL_ConnID);
                            break;
                        }
                    }
                }
                else
                {
                    if (addrSubType == BLE_ADDR_SUB_TYPE_RANDOM_STATIC)
                    {
                        if (memcmp(p_host_current->IDAddr, peerAddr, BLE_ADDR_LEN) == 0)
                        {
                            chkATT_DB(BLE_HOSTID_RESERVED, p_host_current->LL_ConnID);
                            break;
                        }
                    }
                }
            }
        }
        ble_cmd_conn_terminate(host_id);
    } while (0);

    // gen random
    if (hci_le_random_cmd() == ERR_OK)
    {
        // send encrypt queue
        encrypt_msg.host_id       = host_id;
        encrypt_msg.encrypt_state = STATE_GEN_RANDOM_NUMBER;
        sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
    }

    if (att_db_link[host_id].p_client_db != (const ble_att_param_t **) 0)
    {
        if ((p_host_current->ConnInterval > 0x8C) || (MAX_CONN_NO_APP != 1)) // 175ms interval
        {
            timeout_value = 18000; // 180s
        }
        else
        {
            timeout_value = (p_host_current->ConnInterval) << 6; //(ConnInterval*128)*10 ms
        }
        bhc_timer_set(host_id, TIMER_EVENT_CLIENT_DB_PARSING, timeout_value);
    }
}

/** The BLE host processing connection update completion event.
 */
void bhc_connection_update_handle(ble_hci_le_meta_evt_param_conn_update_t * p_data)
{
    uint8_t host_id = queryConnID_by_HSID(p_data->conn_handle);
    memcpy((void *) &LE_Host[host_id].ConnInterval, (void *) &p_data->conn_interval, 6);
}

/**
 * host transmit the connection parameter update request
 *
 * @param[in]  conn_id : the link's host id.
 * @param[in]  p_param : a pointer to connection parameter.
 *
 * @retval  true : setting success.
 * @retval  false: event timer busy.
 *
 */
ble_err_t bhc_gap_connection_update(uint16_t conn_id, ble_gap_conn_param_t * p_param)
{
    extern int8_t bhc_acl_data_send(uint16_t connID, uint8_t * ScanRspData, uint8_t Length);
    static uint8_t l2cap_identifier = 1;
    ACL_Data * ACL_dataTx;
    uint32_t timeout_base;
    uint8_t host_id;

    host_id    = bhc_query_host_id_by_conn_id(conn_id);
    ACL_dataTx = mem_malloc(sizeof(L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request) + SIZE_BASIC_L2CAP_HEADER);
    if (ACL_dataTx != NULL)
    {
        ACL_dataTx->Length_PDU = 12;
        ACL_dataTx->ChannelID  = L2CAP_CID_LE_L2CAP_SIGNALING_CHANNEL;
        ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Code =
            CODE_SIGNL_CMD_CONNECTION_PARAMETER_UPDATE_REQUEST;
        ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.identifier = l2cap_identifier;
        ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Length     = 8;
        memcpy((uint8_t *) &ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmin, (uint8_t *) p_param,
               sizeof(ble_gap_conn_param_t));
        if (bhc_acl_data_send(conn_id, (uint8_t *) ACL_dataTx, (ACL_dataTx->Length_PDU + SIZE_BASIC_L2CAP_HEADER)) == ERR_OK)
        {
            timeout_base = (LE_Host[host_id].ConnInterval * (LE_Host[host_id].ConnLatency + 1)) << 4;
            bhc_timer_set(host_id, TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP, timeout_base); //(ConnInterval*(ConnLatency+1)*160 ms)
            mem_free(ACL_dataTx);
            l2cap_identifier++;
            return BLE_ERR_OK;
        }
        else
        {
            mem_free(ACL_dataTx);
            return BLE_BUSY;
        }
    }

    return BLE_BUSY;
}

/** BLE GATT Get Characteristic Values from Bonded space
 *
 * @attention The link shall be in connection and encryption/ authentication mode or return error code.
 *
 * @param[in]   hostId    : the link's host id.
 *
 * @retval BLESTACK_STATUS_ERR_DB_PARSING_IN_PROGRESS  : Service/ characteristic discovering or authentication/ authorization have
 * not completed.
 * @retval BLESTACK_STATUS_ERR_INVALID_STATE           : BLE stack has not initialized or there is no connection established with
 * the host id.
 * @retval BLESTACK_STATUS_SUCCESS                     : Setting success.
 */
ble_err_t bhc_sm_restore_cccd_from_bond(uint8_t host_id)
{
    LE_Host_Para * p_host_current;
    uint8_t * tmp;
    uint16_t end_DB_Mapping;
    uint16_t index;
    ble_err_t err_code;

    err_code = BLE_ERR_OK;
    do
    {
        if (chkPermission_ATT_DataTansfer(host_id, 0) != TRUE)
        {
            err_code = BLE_ERR_DB_PARSING_IN_PROGRESS;
            break;
        }

        // not in encryption/ authentication mode.
        p_host_current = &LE_Host[host_id];
        if (p_host_current->IsEncryption != 1)
        {
            err_code = BLE_ERR_INVALID_STATE;
            break;
        }

        if (att_db_link[host_id].p_server_db != (const ble_att_param_t **) 0)
        {
            end_DB_Mapping                          = att_db_mapping_size[host_id].size_map_server_db - 1;
            smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;

            for (index = 1; index <= end_DB_Mapping; index++)
            {
                if ((att_db_link[host_id].p_server_db[index]->db_permission_format & ATT_VALUE_BOND_ENABLE) ==
                    ATT_VALUE_BOND_ENABLE)
                {
                    smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_H] = index >> 8;
                    smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_L] = index;
                    smp_Para_Bond_tmp[TAB_PARA_DATA_GATT_ROLE] = BLE_GATT_ROLE_SERVER;
                    tmp                                        = ble_flashbond_restore_data((uint8_t *) smp_Para_Bond_tmp);
                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                    {
                        ble_evt_att_param_t * pt_evt;

                        pt_evt = mem_malloc(sizeof(ble_evt_att_param_t) + smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_SIZE]);
                        if (pt_evt != NULL)
                        {
                            pt_evt->host_id    = host_id;
                            pt_evt->gatt_role  = BLE_GATT_ROLE_SERVER;
                            pt_evt->cb_index   = index;
                            pt_evt->handle_num = index;
                            pt_evt->opcode     = OPCODE_ATT_RESTORE_BOND_DATA_COMMAND;
                            pt_evt->event      = 0;
                            pt_evt->length     = smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_SIZE];
                            memcpy(pt_evt->data, &smp_Para_Bond_tmp[TAB_PARA_DATA_DAT], pt_evt->length);

                            att_db_link[pt_evt->host_id].p_server_db[pt_evt->cb_index]->att_handler(pt_evt);
                            mem_free(pt_evt);
                        }
                        else
                        {
                            err_code = BLE_BUSY;
                            break;
                        }
                    }
                }
            }
        }
        if (att_db_link[host_id].p_client_db != (const ble_att_param_t **) 0)
        {
            end_DB_Mapping                          = att_db_mapping_size[host_id].size_map_client_db - 1;
            smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;

            for (index = 1; index <= end_DB_Mapping; index++)
            {
                if ((att_db_link[host_id].p_client_db[index]->db_permission_format & ATT_VALUE_BOND_ENABLE) ==
                    ATT_VALUE_BOND_ENABLE)
                {
                    smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_H] = att_db_mapping[host_id].map_client_db[index].handle_num >> 8;
                    smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_L] = att_db_mapping[host_id].map_client_db[index].handle_num;
                    smp_Para_Bond_tmp[TAB_PARA_DATA_GATT_ROLE] = BLE_GATT_ROLE_CLIENT;
                    tmp                                        = ble_flashbond_restore_data((uint8_t *) smp_Para_Bond_tmp);
                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                    {
                        ble_evt_att_param_t * pt_evt;

                        att_db_mapping[host_id].map_client_db[index].value.cfg_client_charc =
                            smp_Para_Bond_tmp[TAB_PARA_DATA_DAT]; // cccd value

                        pt_evt = mem_malloc(sizeof(ble_evt_att_param_t) + smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_SIZE]);
                        if (pt_evt != NULL)
                        {
                            pt_evt->host_id    = host_id;
                            pt_evt->gatt_role  = BLE_GATT_ROLE_CLIENT;
                            pt_evt->cb_index   = index;
                            pt_evt->handle_num = att_db_mapping[host_id].map_client_db[index].handle_num;
                            pt_evt->opcode     = OPCODE_ATT_RESTORE_BOND_DATA_COMMAND;
                            pt_evt->event      = 0;
                            pt_evt->length     = smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_SIZE];
                            memcpy(pt_evt->data, &smp_Para_Bond_tmp[TAB_PARA_DATA_DAT], pt_evt->length);
                            att_db_link[pt_evt->host_id].p_client_db[pt_evt->cb_index]->att_handler(pt_evt);
                            mem_free(pt_evt);
                        }
                        else
                        {
                            err_code = BLE_BUSY;
                            break;
                        }
                    }
                }
            }
        }
    } while (0);

    return err_code;
}

/** BLE Bonding space initialize (clear)
 *
 * @attention The link shall be in connection and encryption/ authentication mode or return error code.
 *
 *
 * @retval BLESTACK_STATUS_ERR_DB_PARSING_IN_PROGRESS  : Service/ characteristic discovering or authentication/ authorization have
 * not completed.
 * @retval BLESTACK_STATUS_ERR_INVALID_STATE           : BLE stack has not initialized or there is no connection established with
 * the host id.
 * @retval BLESTACK_STATUS_SUCCESS                     : Setting success.
 */
ble_err_t bhc_sm_bonding_space_init(void)
{
    ble_err_t status;
    MBLK * p_mblk;

    status = BLE_ERR_OK;
    p_mblk = get_msgblks_L2(1);
    if (p_mblk != NULL)
    {
        p_mblk->primitive = HOST_MSG_SM_BOND_SPACE_INT;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
    }
    else
    {
        status = BLE_ERR_DATA_MALLOC_FAIL;
    }

    return status;
}

/** BLE Bonding space initialize (clear)
 *
 * @attention The link shall be in connection and encryption/ authentication mode or return error code.
 *
 *
 * @retval BLESTACK_STATUS_ERR_DB_PARSING_IN_PROGRESS  : Service/ characteristic discovering or authentication/ authorization have
 * not completed.
 * @retval BLESTACK_STATUS_ERR_INVALID_STATE           : BLE stack has not initialized or there is no connection established with
 * the host id.
 * @retval BLESTACK_STATUS_SUCCESS                     : Setting success.
 */
void bhc_sm_prcss_bonding_space_init(void)
{
    ble_flashbond_cmd(CMD_FB_INIT_INFO_FLASHBOND, (uint8_t *) smp_Para_Bond_tmp);
}

int8_t bhc_le_remote_conn_parameter_req_handle(ble_hci_le_meta_evt_param_conn_para_req_t * p_data)
{
    ble_hci_le_meta_evt_param_conn_para_req_t * p_conn_param = (ble_hci_le_meta_evt_param_conn_para_req_t *) p_data;
    ble_hci_cmd_le_remote_conn_param_req_reply_param_t p_reply_param;

    p_reply_param.conn_handle       = p_conn_param->conn_handle;
    p_reply_param.conn_interval_min = p_conn_param->min_interval;
    p_reply_param.conn_interval_max = p_conn_param->max_interval;
    p_reply_param.max_latency       = p_conn_param->max_latency;
    p_reply_param.supv_timeout      = p_conn_param->timeout;
    p_reply_param.min_celength      = 0xFFFF;
    p_reply_param.max_celength      = 0xFFFF;

    return hci_le_remote_conn_param_req_reply_cmd(&p_reply_param);
}

int8_t bhc_le_remote_conn_parameter_req_neg_handle(ble_hci_cmd_le_remote_conn_param_req_neg_reply_param_t * p_data)
{
    return hci_le_remote_conn_param_req_neg_reply_cmd(p_data);
}

ble_err_t bhc_sm_identity_resolving_key_set(ble_sm_irk_param_t * p_param)
{
    memcpy(&identity_resolving_seed[0], p_param, sizeof(ble_sm_irk_param_t));

    return BLE_ERR_OK;
}

int8_t bhc_disconnect_handle(uint16_t conn_handle)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_acl_data_clear_t * p_hci_message;
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    p_hci_message = mem_malloc(sizeof(ble_hci_acl_data_clear_t));

    if (p_hci_message != NULL)
    {
        p_hci_message->conn_handle = conn_handle;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_ACL_DATA_CLEAR;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

int8_t bhc_host_gen_resolvable_address(uint8_t host_id)
{
    int8_t err_code;
    uint32_t sys_time_cnt;
    private_addr_gen_t addr_gen_param;

    addr_gen_param.irk = param_rpa[host_id].irk;
    sys_time_cnt       = sys_now() & 0x003FFFFF;
    sys_time_cnt |= 0x400000;
    addr_gen_param.private_addr24 = (uint8_t *) &sys_time_cnt;
    err_code                      = private_address_construct_by_id(host_id, &addr_gen_param);
    if (err_code == ERR_OK)
    {
        memcpy(&param_rpa[host_id].rpa_addr[3], (uint8_t *) &sys_time_cnt, 3);
    }

    return err_code;
}

int8_t bhc_host_gen_new_irk(uint8_t host_id)
{
    int8_t err_code;
    ble_hci_le_encrypt_param_t param;
    encrypt_queue_t encrypt_msg;

    err_code = ERR_OK;
    memcpy(param.key, &identity_resolving_seed[0], SIZE_SMP_IRK);
    memcpy(param.plaintext_data, param_rpa[host_id].irk, SIZE_SMP_IRK);
    vPortEnterCritical();
    if (hci_le_encrypt_cmd(&param) == ERR_OK)
    {
        // send encrypt queue
        encrypt_msg.host_id       = host_id;
        encrypt_msg.encrypt_state = STATE_GEN_LOCAL_IRK_BY_SECURITY;
        sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
    }
    else
    {
        err_code = ERR_MEM;
    }
    vPortExitCritical();
    return err_code;
}

uint8_t bhc_host_privacy_private_addr_scan_check(ble_gap_addr_t * addr_param)
{
    uint8_t err_code, host_id, i;
    uint8_t addrSubType;
    uint8_t AdvAddr[6];

    err_code = TRUE;
    do
    {
        if (ble_privacy_parameter_enable_check() == FALSE)
        {
            break;
        }
        host_id = ble_privacy_host_id_get();
        memcpy_inv(AdvAddr, addr_param->addr, BLE_ADDR_LEN);
        addrSubType = AdvAddr[0] & BLE_ADDR_SUB_TYPE_FLD_RANDOM;

        if (ble_privacy_parameter_LL_privacy_on_check() == FALSE)
        {
            if (addr_param->addr_type != PUBLIC_ADDR)
            {
                if (addrSubType == BLE_ADDR_SUB_TYPE_RANDOM_RESOLVABLE)
                {
                    for (i = 0; i < 10; i++)
                    {
                        if (memcmp(resolvable_comp_list[i], addr_param->addr, 3) == 0)
                        {
                            break;
                        }
                    }
                    if (i == 10)
                    {
                        if (host_encrypt_queue_remaining_size() > (QUEUE_HOST_ENCRYPT >> 1))
                        {
                            memcpy(AdvAddr, &addr_param->addr[3], 3);
                            private_address_resolve_by_id(host_id, AdvAddr);
                        }
                        err_code = FALSE;
                    }
                    break;
                }
            }
            if (ble_privacy_parameter_device_mode_check() == TRUE)
            {
                if (addr_param->addr_type == PUBLIC_ADDR)
                {
                    if (LE_Host[host_id].IDAddrType == PUBLIC_ADDR)
                    {
                        if (memcmp(LE_Host[host_id].IDAddr, AdvAddr, BLE_ADDR_LEN) != 0)
                        {
                            err_code = FALSE;
                        }
                    }
                    else
                    {
                        err_code = FALSE;
                    }
                }
                else
                {
                    if (LE_Host[host_id].IDAddrType == RANDOM_STATIC_ADDR)
                    {
                        if (memcmp(LE_Host[host_id].IDAddr, AdvAddr, BLE_ADDR_LEN) != 0)
                        {
                            err_code = FALSE;
                        }
                    }
                    else
                    {
                        err_code = FALSE;
                    }
                }
            }
        }
    } while (0);

    return err_code;
}
