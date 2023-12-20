
#include "crypto.h"

/*
 *  secp256r1
 *
 *  Remark: This is demo sample only.
 *  HERE PRIVATE KEY IS TEST ONLY. Private Key should be random generated.
 *  It can NOT be hardcode, otherwise key will be extracted from binary or hex file.
 *
 *     Please Notice: Share key is in little endian format...
 *  So if you want to use the share key for some cryption, please notice
 *  key format is little-endian or big-endian
 */

/*Remark: data is little endian format. that is MSB is the last byte. */

int rt583_ecc_public_key_gen(uint8_t * pt_pri_k, uint8_t * pt_pub_k_x, uint8_t * pt_pub_k_y)
{

    ECPoint_P256 Public_key;

    /* initial the ECC Engine */
    gfp_ecc_curve_p256_init();

    /* generate the publick key by */
    gfp_point_p256_mult((ECPoint_P256 *) &Public_key, (ECPoint_P256 *) &Curve_Gx_p256, (uint32_t *) pt_pri_k);

    memcpy(pt_pub_k_x, Public_key.x, secp256r1_op_num_in_byte);
    memcpy(pt_pub_k_y, Public_key.y, secp256r1_op_num_in_byte);

    return 0;
}

int rt583_ecc_shared_secert_gen(uint8_t * pt_sh_k_x, uint8_t * pt_sh_k_y, uint8_t * pt_pub_k_x, uint8_t * pt_pub_k_y,
                                uint8_t * pt_prv_k_y)
{

    ECPoint_P256 Share_key, Public_key;

    // printf("    caculate shared secret by using hardware ECDH\n");

    memcpy(Public_key.x, pt_pub_k_x, secp256r1_op_num_in_byte);
    memcpy(Public_key.y, pt_pub_k_y, secp256r1_op_num_in_byte);

    /* initial the curve data */
    gfp_ecc_curve_p256_init();

    /* Caculate the secert shared key*/
    gfp_point_p256_mult((ECPoint_P256 *) &Share_key, (ECPoint_P256 *) &Public_key, (uint32_t *) pt_prv_k_y);

    memcpy(pt_sh_k_x, Share_key.x, secp256r1_op_num_in_byte);
    memcpy(pt_sh_k_y, Share_key.y, secp256r1_op_num_in_byte);

    return 0;
}
