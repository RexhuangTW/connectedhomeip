
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

/* spake2+ compute the L for verifier*/
int rt582_spake2p_compute_L(uint8_t * L, uint8_t * w1)
{

    /*computer L = w1*P ... P is generator...*/

    ECPoint_P256 Result_point;
    uint8_t w1_le[32];

    /*change w1 to little endian*/
    buffer_endian_exchange((uint32_t *) w1_le, (uint32_t *) w1, 8);

    gfp_ecc_curve_p256_init();
    gfp_point_p256_mult((ECPoint_P256 *) &Result_point, (ECPoint_P256 *) &Curve_Gx_p256, (uint32_t *) w1_le);

    buffer_endian_exchange((uint32_t *) L, (uint32_t *) Result_point.x, 8);
    buffer_endian_exchange((uint32_t *) (L + secp256r1_op_num_in_byte), (uint32_t *) Result_point.y, 8);

    return 0;
}

/* doing the (R =  m * P + n * Q ) */
int rt582_ecc_multi_add(uint8_t * R_x, uint8_t * R_y, uint8_t * m, uint8_t * P_x, uint8_t * P_y, uint8_t * n, uint8_t * Q_x,
                        uint8_t * Q_y)

{

    ECPoint_P256 R, P, Q, tmp_1, tmp_2;

    /* init the P, Q values*/
    memcpy(P.x, P_x, secp256r1_op_num_in_byte);
    memcpy(P.y, P_y, secp256r1_op_num_in_byte);
    memcpy(Q.x, Q_x, secp256r1_op_num_in_byte);
    memcpy(Q.y, Q_y, secp256r1_op_num_in_byte);

    /* (tmp_1 = m * P) */
    /* initial the ECC Engine */
    gfp_ecc_curve_p256_init();
    /* generate the publick key by */
    gfp_point_p256_mult((ECPoint_P256 *) &tmp_1, (ECPoint_P256 *) &P, (uint32_t *) m);

    /* (tmp_2 = n * Q) */
    /* initial the ECC Engine */
    gfp_ecc_curve_p256_init();

    /* generate the publick key by */
    gfp_point_p256_mult((ECPoint_P256 *) &tmp_2, (ECPoint_P256 *) &Q, (uint32_t *) n);

    gfp_point_p256_add((ECPoint_P256 *) &R, (ECPoint_P256 *) &tmp_1, (ECPoint_P256 *) &tmp_2);

    memcpy(R_x, R.x, secp256r1_op_num_in_byte);
    memcpy(R_y, R.y, secp256r1_op_num_in_byte);

    /*change little endian format ECC point to big endian format... temp use result_le and result_be */
    //    buffer_endian_exchange((uint32_t *) R_x, (uint32_t *) &(R.x), 8);
    //    buffer_endian_exchange((uint32_t *) R_y, (uint32_t *) &(R.y), 8);

    return 0;
}

/* special function for SPAKE2+ , Z = y*(X-w0*M) */

int rt582_spake2p_compute_Z(uint8_t * Z_x, uint8_t * Z_y, uint8_t * y, uint8_t * X_x, uint8_t * X_y, uint8_t * w0, uint8_t * M_x,
                            uint8_t * M_y)
{
    ECPoint_P256 Z, X, M, tmp_1, tmp_2;

    /* init the X, M values*/
    memcpy(X.x, X_x, secp256r1_op_num_in_byte);
    memcpy(X.y, X_y, secp256r1_op_num_in_byte);
    memcpy(M.x, M_x, secp256r1_op_num_in_byte);
    memcpy(M.y, M_y, secp256r1_op_num_in_byte);

    gfp_ecc_curve_p256_init();

    /*Invert M...*/ /*Invert M = (-M).  Invert M is little endian */
    gfp_point_p256_invert(&M, &M);

    /*so M now is -w0*M,  little endian */
    gfp_ecc_curve_p256_init();
    gfp_point_p256_mult((ECPoint_P256 *) &M, (ECPoint_P256 *) &M, (uint32_t *) w0);

    /*caculate tmp_1 = X-w0*M  */
    gfp_point_p256_add((ECPoint_P256 *) &tmp_1, (ECPoint_P256 *) &M, (ECPoint_P256 *) &X);

    /*get Z = y*(X-w0*M) */
    gfp_ecc_curve_p256_init();
    gfp_point_p256_mult((ECPoint_P256 *) &Z, (ECPoint_P256 *) &tmp_1, (uint32_t *) y);

    memcpy(Z_x, Z.x, secp256r1_op_num_in_byte);
    memcpy(Z_y, Z.y, secp256r1_op_num_in_byte);

    return 0;
}

/* special function for SPAKE2+ , V = y*L */
int rt582_spake2p_verifier_V(uint8_t * V_x, uint8_t * V_y, uint8_t * y, uint8_t * L_x, uint8_t * L_y)
{

    ECPoint_P256 V, L;

    /* init the X, M values*/
    memcpy(L.x, L_x, secp256r1_op_num_in_byte);
    memcpy(L.y, L_y, secp256r1_op_num_in_byte);

    gfp_ecc_curve_p256_init();
    gfp_point_p256_mult((ECPoint_P256 *) &V, (ECPoint_P256 *) &L, (uint32_t *) y);

    memcpy(V_x, V.x, secp256r1_op_num_in_byte);
    memcpy(V_y, V.y, secp256r1_op_num_in_byte);

#if 0
    {
        int i;
        printf("verifier V point:\n");
        for (i = 0; i < 32; i++)
            printf("%2x-", V_x[i]);
        printf("\n");
        for (i = 0; i < 32; i++)
            printf("%2x-", V_y[i]);
        printf("\n");
    }
#endif

    return 0;
}

/* special function for SPAKE2+ , V = w1*(Y-w0*N) */
int rt582_spake2p_prover_V(uint8_t * V_x, uint8_t * V_y, uint8_t * w0, uint8_t * w1, uint8_t * Y_x, uint8_t * Y_y, uint8_t * N_x,
                           uint8_t * N_y)
{
    ECPoint_P256 V, Y, N, tmp_1, tmp_2;

    /* init the X, M values*/
    memcpy(Y.x, Y_x, secp256r1_op_num_in_byte);
    memcpy(Y.y, Y_y, secp256r1_op_num_in_byte);
    memcpy(N.x, N_x, secp256r1_op_num_in_byte);
    memcpy(N.y, N_y, secp256r1_op_num_in_byte);

    gfp_ecc_curve_p256_init();

    /*Invert N...*/ /*Invert N = (-N).  Invert M is little endian */
    gfp_point_p256_invert(&N, &N);

    /*so M now is -w0*N,  little endian */
    gfp_ecc_curve_p256_init();
    gfp_point_p256_mult((ECPoint_P256 *) &N, (ECPoint_P256 *) &N, (uint32_t *) w0);

    /*caculate tmp_1 = Y-w0*M  */
    gfp_point_p256_add((ECPoint_P256 *) &tmp_1, (ECPoint_P256 *) &N, (ECPoint_P256 *) &Y);

    /*get Z = w1*(X-w0*M) */
    gfp_ecc_curve_p256_init();
    gfp_point_p256_mult((ECPoint_P256 *) &V, (ECPoint_P256 *) &tmp_1, (uint32_t *) w1);

    memcpy(V_x, V.x, secp256r1_op_num_in_byte);
    memcpy(V_y, V.y, secp256r1_op_num_in_byte);

    return 0;
}