/**
 * @file crypto.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2022-04-21
 *
 * @copyright Copyright (c) 2022
 *
 */
#include OPENTHREAD_PROJECT_CORE_CONFIG_FILE
#include <openthread/config.h>
#include "openthread-system.h"
#include "cm3_mcu.h"
#include "aes.h"
#include <openthread/instance.h>
#include <openthread/platform/crypto.h>
#include <openthread/platform/entropy.h>
#include <openthread/platform/time.h>

static struct aes_ctx saes_ecbctx;

otError otPlatCryptoAesInit(otCryptoContext *aContext)
{
    otError                error = OT_ERROR_NONE;

    aes_acquire(&saes_ecbctx);

exit:
    return error;
}

otError otPlatCryptoAesSetKey(otCryptoContext *aContext, const otCryptoKey *aKey)
{
    otError                error = OT_ERROR_NONE;

    aes_key_init(&saes_ecbctx, aKey->mKey, AES_KEY128);
    aes_load_round_key(&saes_ecbctx);

exit:
    return error;
}

otError otPlatCryptoAesEncrypt(otCryptoContext *aContext, const uint8_t *aInput, uint8_t *aOutput)
{
    otError                error = OT_ERROR_NONE;
    uint32_t status;

    status = aes_ecb_encrypt(&saes_ecbctx, aInput, aOutput);

exit:
    return error;
}

otError otPlatCryptoAesFree(otCryptoContext *aContext)
{
    otError                error = OT_ERROR_NONE;

    aes_release(&saes_ecbctx);
exit:
    return error;
}
