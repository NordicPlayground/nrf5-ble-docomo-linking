/* Copyright (c) 2016 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#ifndef BLE_PDLP_UTIL_H__
#define BLE_PDLP_UTIL_H__

#include <stdint.h>
#include <string.h>

/**@brief PDLS result code */
typedef enum
{
    PDLS_RESULT_OK,
    PDLS_RESULT_CANCEL,
    PDLS_RESULT_ERROR_FAILED,
    PDLS_RESULT_ERROR_UNKNOWN,
    PDLS_RESULT_ERROR_NO_DATA,
    PDLS_RESULT_ERROR_NOT_SUPPORT,
    PDLS_RESULT_MAX
} ble_pdls_result_code_t;

/**@brief PDLS cancel reason code */
typedef enum
{
    PDLS_CANCEL_USER_CANCEL,
    PDLS_CANCEL_MAX
} ble_pdls_cancel_t;

/**@brief PDLP opaque type. */
typedef struct
{
    uint8_t * p_val;                               /**< Pointer to the value of the opaque data. */                               
    uint32_t  len;                                 /**< Length of p_val. */
} pdlp_opaque_t;

uint32_t pdls_encode_service_header(uint8_t *p_buf, uint8_t service_id, uint16_t message_id, uint8_t number_of_param);
uint32_t pdls_encode_param_uint8 (uint8_t *p_buf, uint8_t param_id, uint8_t  param_data);
uint32_t pdls_encode_param_uint16(uint8_t *p_buf, uint8_t param_id, uint16_t param_data);
uint32_t pdls_encode_param_uint32(uint8_t *p_buf, uint8_t param_id, uint32_t param_data);
uint32_t pdls_encode_param_opaque(uint8_t *p_buf, uint8_t param_id, pdlp_opaque_t *p_param_data);

ble_pdls_result_code_t pdls_decode_param_uint8(uint8_t *p_buf, uint8_t param_id, uint8_t *param_data);
ble_pdls_result_code_t pdls_decode_param_uint16(uint8_t *p_buf, uint8_t param_id, uint16_t *param_data);
ble_pdls_result_code_t pdls_decode_param_uint32(uint8_t *p_buf, uint8_t param_id, uint32_t *param_data);
ble_pdls_result_code_t pdls_decode_param_opaque(uint8_t *p_buf, uint8_t param_id, pdlp_opaque_t *p_param_data);

uint16_t IEEE754_Convert_Temperature(float f_value);
uint16_t IEEE754_Convert_Humidity(float f_value);
uint16_t IEEE754_Convert_Air_Pressure(float f_value);

#endif // BLE_PDLP_UTIL_H__

/** @} */
