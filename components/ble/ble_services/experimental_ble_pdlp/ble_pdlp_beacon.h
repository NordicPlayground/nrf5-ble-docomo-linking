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
#ifndef BLE_PDLP_BEACON_H__
#define BLE_PDLP_BEACON_H__

#include <stdint.h>
#include <stdbool.h>

/* Raw radio packets definition
 */
#define BD_ADDR_OFFS                (3)     /* BLE device address offest of the beacon advertising pdu. */
#define M_BD_ADDR_SIZE              (6)     /* BLE device address size. */

#define SINT16_SERVICE_DATA_OFFS    (38)    /* The offset of the temperature in the beacon advertising pdu */

/* The linking beacon service types based on DoCoMo spec v2.0.2 (2016-08-08)
 */
enum
{
    LINKING_SERVICE_TYPE_NONE = 0,
    LINKING_SERVICE_TYPE_TEMPERATURE,
    LINKING_SERVICE_TYPE_HUMIDITY,
    LINKING_SERVICE_TYPE_AIRPRESSURE,
    LINKING_SERVICE_TYPE_BATTERY,
    LINKING_SERVICE_TYPE_BUTTON,
    LINKING_SERVICE_TYPE_OPEN_CLOSE_SENSE,
    LINKING_SERVICE_TYPE_HUMAN_SENSE,
    LINKING_SERVICE_TYPE_VIBRATION_SENSE,
    LINKING_SERVICE_TYPE_GENERAL = 15
};

typedef struct
{
    uint16_t service_id:4;
    uint16_t charge_needed:1;       /* 0: not needed; 1: needed */
    uint16_t battery_level:11;      /* 0.0% ~ 100.0%, no details on float coding yet */
} linking_battery_info_t;

#define LINKING_BUTTON_ID_MIN       0
#define LINKING_BUTTON_ID_MAX       4095
typedef struct
{
    uint16_t service_id:4;
    uint16_t button_id:12;          /* 0 ~ 4095 */
} linking_button_info_t;

#define LINKING_OPEN_COUNT_MAX      2047
typedef struct
{
    uint16_t service_id:4;
    uint16_t open_close_flag:1;     /* 0: open; 1: close */
    uint16_t open_count:11;         /* 0 ~ 2047 */
} linking_openclose_sensor_info_t;

typedef struct
{
    uint16_t service_id:4;
    uint16_t human_sensed:1;        /* 0: not sensed; 1: sensed */
    uint16_t dummy:11;
} linking_human_sensor_info_t;

typedef struct
{
    uint16_t service_id:4;
    uint16_t vibration_sensed:1;    /* 0: not sensed; 1: sensed */
    uint16_t dummy:11;
} linking_vibration_sensor_info_t;

#endif // BLE_PDLP_BEACON_H__

/** @} */
