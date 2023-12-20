#include "crypto.h"

static void endian_swap(uint8_t * dst, uint8_t * src, uint32_t size)
{
    uint32_t i = 0;

    while (i < size)
    {
        dst[i] = src[(size - 1) - i];

        i++;
    }
}

int rt583_ecdsa_p256_sign(uint8_t * s_r, uint8_t * s_s, uint8_t * hash, uint8_t * pri_k, uint8_t * mod_k)
{
    /* sha 256 size is 32 bytes */
    uint8_t buf[32];
    Signature_P256 data_signature;

    /* swap the hash value endian */
    endian_swap(buf, hash, sizeof(buf));

    /*ecdsa signature.*/
    gfp_ecc_curve_p256_init();

    gfp_ecdsa_p256_signature(&data_signature, (uint32_t *) buf, (uint32_t *) pri_k, (uint32_t *) mod_k);

    memcpy(s_r, data_signature.r, secp256r1_op_num_in_byte);
    memcpy(s_s, data_signature.s, secp256r1_op_num_in_byte);

    return 0;
}

int rt583_ecdsa_p256_verify(uint8_t * s_r, uint8_t * s_s, uint8_t * hash, uint8_t * key_x, uint8_t * key_y)
{
    /* sha 256 size is 32 bytes */
    uint8_t buf[32];
    uint32_t status;
    Signature_P256 data_signature;
    ECPoint_P256 Public_key;

    /* swap the hash value endian */
    endian_swap(buf, hash, sizeof(buf));

    memcpy(data_signature.r, s_r, secp256r1_op_num_in_byte);
    memcpy(data_signature.s, s_s, secp256r1_op_num_in_byte);
    memcpy(Public_key.x, key_x, secp256r1_op_num_in_byte);
    memcpy(Public_key.y, key_y, secp256r1_op_num_in_byte);

    /*ecdsa signature.*/
    gfp_ecdsa_p256_verify_init();

    status = gfp_ecdsa_p256_verify(&data_signature, (uint32_t *) buf, &Public_key);

    return ((int) status);
}
