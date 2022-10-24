/*******************************************************************
 *
 * File Name  : ble_memory.c
 * Description:
 *
 *******************************************************************
 *
 *      Copyright (c) 2020, All Right Reserved
 *      Rafael Microelectronics Co. Ltd.
 *      Taiwan, R.O.C.
 *
 *******************************************************************/
/*******************************************************************
 *      Include List
 *******************************************************************/
#include <stdio.h>
#include "cm3_mcu.h"
#include "sys_arch.h"
#include "host_management.h"
#include "ble_memory.h"

#define MAX_MSGBLK_DATA_LENGTH  512
/*******************************************************************
 *      Global Variable Defines
 *******************************************************************/
uint32_t msg_data[MAX_MBLK_NO][SIZE_MBLK >> 2];
MBLK *msgblk = (MBLK *) &msg_data[0][0];
MBLK *mblk_free;                  /* pointer to head of free MBLK list */
uint16_t mblk_depth_remaining;
BufProcess      ConnBuffPrcsR;

void ble_memory_init(void)
{
    uint16_t i;

    /* Initial message block,
       link up free list of message block */
    for (i = 0; i < (MAX_MBLK_NO - 1); i++)
    {
        msgblk[i].next = (MBLK *)&msgblk[i + 1];
    }
    msgblk[i].next = (MBLK *)0;
    mblk_free = (MBLK *)&msgblk[0];
    mblk_depth_remaining = MAX_MBLK_NO;             //initialization
}

MBLK *get_msgblks(uint16_t length)
{
    MBLK *pmblk;
    MBLK *pmblk_Temp;
    uint16_t num_MsgBlk;

    if (length > MAX_MSGBLK_DATA_LENGTH)
    {
        return (MBLK *)0;
    }

    vPortEnterCritical();
    if (mblk_free == (MBLK *)0)
    {
        pmblk = (MBLK *)0;
    }
    else
    {
        if (length == 0)
        {
            pmblk = (MBLK *)0;
        }
        else
        {
            num_MsgBlk = 1;
            pmblk_Temp = mblk_free;
            while (length > SIZE_MBLK_ACL_DATA_UNIT)
            {
                num_MsgBlk++;
                if (pmblk_Temp->next != (MBLK *)0)
                {
                    pmblk_Temp = pmblk_Temp->next;
                }
                else
                {
                    break;
                }
                length -= SIZE_MBLK_ACL_DATA_UNIT;
            }
            if (length <= SIZE_MBLK_ACL_DATA_UNIT)
            {
                if (mblk_depth_remaining >= num_MsgBlk)
                {
                    mblk_depth_remaining -= num_MsgBlk;

                    pmblk = mblk_free;
                    mblk_free = pmblk_Temp->next;
                    pmblk_Temp->next = (MBLK *)0;
                }
                else
                {
                    pmblk = (MBLK *)0;
                }
            }
            else
            {
                pmblk = (MBLK *)0;
            }
        }
    }
    vPortExitCritical();
    return (pmblk);         //return "(MBLK *)0" means fail
}

MBLK *get_msgblks_L1(uint16_t length)
{
    MBLK *pmblk;
    MBLK *pmblk_Temp;
    uint16_t num_MsgBlk;

    if (length > MAX_MSGBLK_DATA_LENGTH)
    {
        return (MBLK *)0;
    }

    vPortEnterCritical();
    if (mblk_free == (MBLK *)0)
    {
        pmblk = (MBLK *)0;
    }
    else
    {
        if (length == 0)
        {
            pmblk = (MBLK *)0;
        }
        else
        {
            num_MsgBlk = 1;
            pmblk_Temp = mblk_free;
            while (length > SIZE_MBLK_ACL_DATA_UNIT)
            {
                num_MsgBlk++;
                if (pmblk_Temp->next != (MBLK *)0)
                {
                    pmblk_Temp = pmblk_Temp->next;
                }
                else
                {
                    break;
                }
                length -= SIZE_MBLK_ACL_DATA_UNIT;
            }
            if (length <= SIZE_MBLK_ACL_DATA_UNIT)
            {
                if (mblk_depth_remaining >= (num_MsgBlk + MAX_MBLK_RSV_L1))
                {
                    mblk_depth_remaining -= num_MsgBlk;

                    pmblk = mblk_free;
                    mblk_free = pmblk_Temp->next;
                    pmblk_Temp->next = (MBLK *)0;
                }
                else
                {
                    pmblk = (MBLK *)0;
                }
            }
            else
            {
                pmblk = (MBLK *)0;
            }
        }
    }
    vPortExitCritical();
    return (pmblk);         //return "(MBLK *)0" means fail
}

MBLK *get_msgblks_L2(uint16_t length)
{
    MBLK *pmblk;
    MBLK *pmblk_Temp;
    uint16_t num_MsgBlk;

    if (length > MAX_MSGBLK_DATA_LENGTH)
    {
        return (MBLK *)0;
    }

    vPortEnterCritical();
    if (mblk_free == (MBLK *)0)
    {
        pmblk = (MBLK *)0;
    }
    else
    {
        if (length == 0)
        {
            pmblk = (MBLK *)0;
        }
        else
        {
            num_MsgBlk = 1;
            pmblk_Temp = mblk_free;
            while (length > SIZE_MBLK_ACL_DATA_UNIT)
            {
                num_MsgBlk++;
                if (pmblk_Temp->next != (MBLK *)0)
                {
                    pmblk_Temp = pmblk_Temp->next;
                }
                else
                {
                    break;
                }
                length -= SIZE_MBLK_ACL_DATA_UNIT;
            }
            if (length <= SIZE_MBLK_ACL_DATA_UNIT)
            {
                if (mblk_depth_remaining >= (num_MsgBlk + MAX_MBLK_RSV_L2))
                {
                    mblk_depth_remaining -= num_MsgBlk;

                    pmblk = mblk_free;
                    mblk_free = pmblk_Temp->next;
                    pmblk_Temp->next = (MBLK *)0;
                }
                else
                {
                    pmblk = (MBLK *)0;
                }
            }
            else
            {
                pmblk = (MBLK *)0;
            }
        }
    }
    vPortExitCritical();
    return (pmblk);         //return "(MBLK *)0" means fail
}

int8_t check_msgblk_L1_size(uint16_t length)
{
    uint16_t num_MsgBlk;

    num_MsgBlk = 1;
    vPortEnterCritical();
    while (length > SIZE_MBLK_ACL_DATA_UNIT)
    {
        num_MsgBlk++;
        length -= SIZE_MBLK_ACL_DATA_UNIT;
    }
    if (mblk_depth_remaining >= (num_MsgBlk + MAX_MBLK_RSV_L1))
    {
        vPortExitCritical();
        return ERR_OK;
    }
    else
    {
        vPortExitCritical();
        return ERR_MEM;
    }
}

int8_t check_msgblk_L2_size(uint16_t length)
{
    uint16_t num_MsgBlk;

    num_MsgBlk = 1;
    vPortEnterCritical();
    while (length > SIZE_MBLK_ACL_DATA_UNIT)
    {
        num_MsgBlk++;
        length -= SIZE_MBLK_ACL_DATA_UNIT;
    }
    if (mblk_depth_remaining >= (num_MsgBlk + MAX_MBLK_RSV_L2))
    {
        vPortExitCritical();
        return ERR_OK;
    }
    else
    {
        vPortExitCritical();
        return ERR_MEM;
    }
}

void msgblks_free(MBLK *pMsgBlk)
{
    MBLK *pmblk;
    uint8_t i;

    pmblk = pMsgBlk;
    i = 0;
    while (1)
    {
        if (pmblk != (MBLK *)0)
        {
            i++;
            if (pmblk->next != (MBLK *)0)
            {
                pmblk = pmblk->next;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    if (pmblk != (MBLK *)0)
    {
        vPortEnterCritical();
        pmblk->next = mblk_free;
        mblk_free = pMsgBlk;
        mblk_depth_remaining += i;
        vPortExitCritical();
    }
}

void acl_data_get(get_acl_data_t *p_param)
{
    uint16_t i, idx, Len2, data_index;
    MBLK *mblk;
    MQUEUE pqueue;

    if (p_param->src_length)
    {
        mblk = (MBLK *)p_param->pDst;
        pqueue.QOut = mblk;
        if (p_param->received_length == 0)
        {
            i = p_param->total_length;
            while (1)
            {
                mblk->para.hci_acl_data_evt.data_idx0 = 0;
                mblk->para.hci_acl_data_evt.data_idx1 = 0;

                if (i)
                {
                    if (i > SIZE_MBLK_ACL_DATA_UNIT)
                    {
                        mblk->para.hci_acl_data_evt.length = SIZE_MBLK_ACL_DATA_UNIT;
                        i -= SIZE_MBLK_ACL_DATA_UNIT;
                    }
                    else
                    {
                        mblk->para.hci_acl_data_evt.length = i;
                    }
                }
                if (mblk->next != (MBLK *)0)
                {
                    mblk = mblk->next;
                }
                else
                {
                    break;
                }
            }
            ConnBuffPrcsR.BufPrcsF = pqueue.QOut;
            if ((pqueue.QOut)->next != (MBLK *)0)
            {
                ConnBuffPrcsR.BufPrcsN = (pqueue.QOut)->next;
            }
            else
            {
                ConnBuffPrcsR.BufPrcsN = ConnBuffPrcsR.BufPrcsF;
            }
        }
        else
        {
            i = p_param->received_length;
            while (1)
            {
                if (i >= SIZE_MBLK_ACL_DATA_UNIT)
                {
                    i -= SIZE_MBLK_ACL_DATA_UNIT;
                }
                else
                {
                    break;
                }
                if (mblk->next != (MBLK *)0)
                {
                    mblk = mblk->next;
                }
            }
            ConnBuffPrcsR.BufPrcsF = mblk;
            if ((ConnBuffPrcsR.BufPrcsF)->next != (MBLK *)0)
            {
                ConnBuffPrcsR.BufPrcsN = (ConnBuffPrcsR.BufPrcsF)->next;
            }
            else
            {
                ConnBuffPrcsR.BufPrcsN = ConnBuffPrcsR.BufPrcsF;
            }
        }

        data_index = 0;
        Len2 = p_param->src_length;
        while (Len2 != 0)
        {
            p_param->src_length = Len2;
            if (p_param->src_length > SIZE_MBLK_ACL_DATA_UNIT)
            {
                p_param->src_length = (SIZE_MBLK_ACL_DATA_UNIT - 1);
                Len2 -= (SIZE_MBLK_ACL_DATA_UNIT - 1);
            }
            else
            {
                Len2 -= p_param->src_length;
            }

            if (ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.length != ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.data_idx0)
            {
                i = ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.length - ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.data_idx0;
                idx = ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.data_idx0;
                if ((idx + p_param->src_length) > SIZE_MBLK_ACL_DATA_UNIT)
                {
                    memcpy(&ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.acl_data[idx], p_param->pSrc + data_index, i);
                    ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.data_idx0 += i;
                    if (ConnBuffPrcsR.BufPrcsF != ConnBuffPrcsR.BufPrcsN)
                    {
                        memcpy(ConnBuffPrcsR.BufPrcsN->para.hci_acl_data_evt.acl_data, p_param->pSrc + data_index + i, (p_param->src_length - i));
                        ConnBuffPrcsR.BufPrcsN->para.hci_acl_data_evt.data_idx0 += (p_param->src_length - i);
                    }
                }
                else
                {
                    memcpy(&ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.acl_data[idx], p_param->pSrc + data_index, p_param->src_length);
                    ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.data_idx0 += p_param->src_length;
                }
                if (ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.length == ConnBuffPrcsR.BufPrcsF->para.hci_acl_data_evt.data_idx0)
                {
                    if (ConnBuffPrcsR.BufPrcsN != (MBLK *)0)
                    {
                        ConnBuffPrcsR.BufPrcsF = ConnBuffPrcsR.BufPrcsN;
                        ConnBuffPrcsR.BufPrcsN = ConnBuffPrcsR.BufPrcsN->next;
                    }
                }
                data_index += p_param->src_length;
            }
        }
    }
}

void get_acl_data_from_msgblks(uint8_t *pDst, MBLK *pSrc)
{
    uint16_t i;
    MBLK *mblk;

    mblk = pSrc;
    i = 0;
    while (1)
    {
        if (mblk != (MBLK *)0)
        {
            memcpy(pDst + i, mblk->para.hci_acl_data_evt.acl_data, mblk->para.hci_acl_data_evt.length);
            i += mblk->para.hci_acl_data_evt.length;
            mblk = mblk->next;
        }
        else
        {
            break;
        }
    }
}

void acl_data2msgblk(MBLK *pDst, uint8_t *p_data, uint16_t length)
{
    MBLK *mblk;
    MBLK *pmblk;
    uint8_t data_idx, len;

    mblk = pDst;
    pmblk = mblk;

    data_idx = 0;
    mblk->para.hci_acl_data_evt.length = length;
    while (length)
    {
        if (length >= SIZE_MBLK_ACL_DATA_UNIT)
        {
            len = SIZE_MBLK_ACL_DATA_UNIT;
            length -= SIZE_MBLK_ACL_DATA_UNIT;
        }
        else
        {
            len = length;
            length = 0;
        }
        pmblk->para.hci_acl_data_evt.length = len;
        pmblk->para.hci_acl_data_evt.data_idx0 = len;
        pmblk->para.hci_acl_data_evt.data_idx1 = 0;
        memcpy(pmblk->para.hci_acl_data_evt.acl_data, (p_data + data_idx), len);

        if (pmblk->next != (MBLK *)0)
        {
            pmblk = pmblk->next;
            data_idx += SIZE_MBLK_ACL_DATA_UNIT;
        }
    }
}

MBLK *acl_data2msgblk_L2(uint8_t *p_data, uint16_t length)
{
    MBLK *mblk;
    MBLK *pmblk;
    uint8_t data_idx, len;

    do
    {
        mblk = get_msgblks_L2(length);
        if (mblk == NULL)
        {
            break;
        }
        pmblk = mblk;

        data_idx = 0;
        mblk->para.hci_acl_data_evt.length = length;
        while (length)
        {
            if (length >= SIZE_MBLK_ACL_DATA_UNIT)
            {
                len = SIZE_MBLK_ACL_DATA_UNIT;
                length -= SIZE_MBLK_ACL_DATA_UNIT;
            }
            else
            {
                len = length;
                length = 0;
            }
            pmblk->para.hci_acl_data_evt.length = len;
            pmblk->para.hci_acl_data_evt.data_idx0 = len;
            pmblk->para.hci_acl_data_evt.data_idx1 = 0;
            memcpy(pmblk->para.hci_acl_data_evt.acl_data, (p_data + data_idx), len);

            if (pmblk->next != (MBLK *)0)
            {
                pmblk = pmblk->next;
                data_idx += SIZE_MBLK_ACL_DATA_UNIT;
            }
        }
    } while (0);

    return mblk;
}
