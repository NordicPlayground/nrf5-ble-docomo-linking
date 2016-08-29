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

#include "ble_pdlp_common.h"

//Single precision (float) gives 23 bits of significand, 8 bits of exponent, and 1 sign bit.
//Double precision (double) gives 52 bits of significand, 11 bits of exponent, and 1 sign bit.
// After IEEE754 conversion by the union structure
//  The sign is stored in bit 32.
//  The exponent can be computed from bits 24-31 by subtracting 127. 
//  The mantissa (also known as significand or fraction) is stored in bits 1-23
// Reference http://www.h-schmidt.net/FloatConverter/IEEE754.html
union IEEE754_Converter{
    float f_val;
    union {
      signed int s_val;
      unsigned int u_val;
    } i_val;
};

// Convert a float to 12-bit IEEE754 format (ID1) as described by Linking spec)
// bit 11     sign
// bit 7-10   exponent
// bit 0-6    fraction
uint16_t IEEE754_Convert_Temperature(float f_value)
{
    union IEEE754_Converter value;
    value.f_val  = f_value;

    int8_t sign       = (  value.i_val.s_val >> 31) & 0x1;
    int8_t exponent   = (((value.i_val.s_val >> 23) & 0xFF) - 127 + 7);   // 4-bit, with DoCoMo adjustment
    int16_t fraction  = (( value.i_val.s_val & 0x7FFFFF) >> 16);          // 7-bit
    if (f_value == 0.0f)
    {
        // Zero
        return (sign<<11);
    }
    else if (exponent == 0)
    {
        // Non-normalization
        return (sign<<11) | ((fraction&0x7F) + 0x1);  // Rounding (due to double precision)
    }
    else if (exponent == 0xF)
    {
        // Maximal
        return ((sign<<11) | (exponent<<8));
    }
    else
    {
        // Normalization 
        return (sign<<11) | (exponent<<7) | (fraction&0x7F);
    }
}

// Convert a float to 12-bit IEEE754 format (ID2) as described by Linking spec)
// bit 8-11   exponent
// bit 0-7    fraction
uint16_t IEEE754_Convert_Humidity(float f_value)
{
    union IEEE754_Converter value;
    value.f_val  = f_value;

    uint16_t exponent = ((value.i_val.u_val >> 23) & 0xFF) - 127 + 7;   // 4-bit, with DoCoMo adjustment
    uint16_t fraction = ((value.i_val.u_val & 0x7FFFFF) >> 15);         // 8-bit
    if (f_value == 0.0f)
    {
        // Zero
        return 0x0000;
    }
    else if (exponent == 0)
    {
        // Non-normalization
        return (fraction&0xFF) + 0x1;  // Rounding (due to double precision)
    }
    else if (exponent == 0xF)
    {
        // Maximal
        return (exponent<<8);
    }
    else
    {
        // Normalization 
        return (exponent<<8) | (fraction&0xFF);
    }
}

// Convert a float to 12-bit IEEE754 format (ID3) as described by Linking spec)
// bit 7-11   exponent
// bit 0-6    fraction
uint16_t IEEE754_Convert_Air_Pressure(float f_value)
{
    union IEEE754_Converter value;
    value.f_val  = f_value;

    uint16_t exponent = ((value.i_val.u_val >> 23) & 0xFF) - 127 + 15;   // 4-bit, with DoCoMo adjustment
    uint16_t fraction = ((value.i_val.u_val & 0x7FFFFF) >> 16);          // 7-bit

    if (f_value == 0.0f)
    {
        // Zero
        return 0x0000;
    }
    else if (exponent == 0)
    {
        // Non-normalization
        return (fraction&0x7F) + 0x1;  // Rounding (due to double precision)
    }
    else if (exponent == 0x1F)
    {
        // Maximal
        return (exponent<<7);
    }
    else
    {
        // Normalization 
        return (exponent<<7) | (fraction&0x7F);
    }
}

// Little-endian encoding 
uint32_t pdls_encode_service_header(uint8_t *p_buf, uint8_t service_id, uint16_t message_id, uint8_t number_of_param)
{
    *(p_buf+0)       = service_id;
    *(p_buf+1)       = (message_id >> 0) & 0xFF;
    *(p_buf+2)       = (message_id >> 8) & 0xFF;;
    *(p_buf+3)       = number_of_param;
    return 4;
}

uint32_t pdls_encode_param_uint8(uint8_t *p_buf, uint8_t param_id, uint8_t param_data)
{
    *(p_buf+0)       = param_id;
    *(p_buf+1)       = 1;
    *(p_buf+2)       = 0;
    *(p_buf+3)       = 0;
    *(p_buf+4)       = param_data;
    return 5;
}

uint32_t pdls_encode_param_uint16(uint8_t *p_buf, uint8_t param_id, uint16_t param_data)
{
    *(p_buf+0)       = param_id;
    *(p_buf+1)       = 2;
    *(p_buf+2)       = 0;
    *(p_buf+3)       = 0;
    *(p_buf+4)       = (param_data >> 0) & 0xFF;
    *(p_buf+5)       = (param_data >> 8) & 0xFF;
    return 6;
}

uint32_t pdls_encode_param_uint32(uint8_t *p_buf, uint8_t param_id, uint32_t param_data)
{
    *(p_buf+0)       = param_id;
    *(p_buf+1)       = 4;
    *(p_buf+2)       = 0;
    *(p_buf+3)       = 0;
    *(p_buf+4)       = (param_data >> 0)  & 0xFF;
    *(p_buf+5)       = (param_data >> 8)  & 0xFF;
    *(p_buf+6)       = (param_data >> 16) & 0xFF;
    *(p_buf+7)       = (param_data >> 24) & 0xFF;
    return 8;
}
  
uint32_t pdls_encode_param_opaque(uint8_t *p_buf, uint8_t param_id, pdlp_opaque_t *p_param_data)
{
    *(p_buf+0)       = param_id;
    *(p_buf+1)       = (p_param_data->len >>0)  & 0xFF;
    *(p_buf+2)       = (p_param_data->len >>8)  & 0xFF;
    *(p_buf+3)       = (p_param_data->len >>16) & 0xFF;
    for (int i=0; i<p_param_data->len; i++)
    {
        *(p_buf+4+i) = *(p_param_data->p_val+i);
    }
    return 4+p_param_data->len;
}

ble_pdls_result_code_t pdls_decode_param_uint8(uint8_t *p_buf, uint8_t param_id, uint8_t *p_param_data)
{
    uint32_t len;
    if (*(p_buf+0) != param_id)
    {
      return PDLS_RESULT_ERROR_NO_DATA;
    }
    len = *(p_buf+1) | *(p_buf+2)<<8 | *(p_buf+3) <<16;
    if (len != 0x01)
    {
      return PDLS_RESULT_ERROR_NO_DATA;
    }

    *p_param_data = *(p_buf+4);
    return PDLS_RESULT_OK;
}

ble_pdls_result_code_t pdls_decode_param_uint16(uint8_t *p_buf, uint8_t param_id, uint16_t *p_param_data)
{
    uint32_t len;
    if (*(p_buf+0) != param_id)
    {
      return PDLS_RESULT_ERROR_NO_DATA;
    }
    len = *(p_buf+1) | *(p_buf+2)<<8 | *(p_buf+3) <<16;
    if (len != 0x02)
    {
      return PDLS_RESULT_ERROR_NO_DATA;
    }

    *p_param_data = *(p_buf+4) | *(p_buf+5)<<8;
    return PDLS_RESULT_OK;
}

ble_pdls_result_code_t pdls_decode_param_uint32(uint8_t *p_buf, uint8_t param_id, uint32_t *p_param_data)
{
    uint32_t len;
    if (*(p_buf+0) != param_id)
    {
      return PDLS_RESULT_ERROR_NO_DATA;
    }
    len = *(p_buf+1) | *(p_buf+2)<<8 | *(p_buf+3) <<16;
    if (len != 0x04)
    {
      return PDLS_RESULT_ERROR_NO_DATA;
    }

    *p_param_data = *(p_buf+4) | *(p_buf+5)<<8 | *(p_buf+6)<<16 | *(p_buf+7)<<24;
    return PDLS_RESULT_OK;
}

ble_pdls_result_code_t pdls_decode_param_opaque(uint8_t *p_buf, uint8_t param_id, pdlp_opaque_t *p_param_data)
{
    if (*(p_buf+0) != param_id)
    {
      return PDLS_RESULT_ERROR_NO_DATA;
    }
    p_param_data->len = *(p_buf+1) | *(p_buf+2)<<8 | *(p_buf+3) <<16;
    p_param_data->p_val = p_buf+4;
    return PDLS_RESULT_OK;
}

