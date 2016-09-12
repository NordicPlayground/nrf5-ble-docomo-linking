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

/** @file
 *
 * @defgroup ble_sdk_srv_pdlp PDLP Service Server
 * @{
 * @ingroup ble_sdk_srv
 *
 * @brief PDLP Service Server module.
 *
 * @details This module implements a custom PDLP Service.
 *          During initialization, the module adds the PDLP Service and Characteristics
 *          to the BLE stack database.
 *
 * @note The application must propagate BLE stack events to the PDLP Service
 *       module by calling ble_pdls_on_ble_evt() from the @ref softdevice_handler callback.
*/

#ifndef BLE_PDLP_H__
#define BLE_PDLP_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "ble_pdlp_common.h"

//
// PeripheralDeviceLinkProfile (PDLP)
//
#define PDLS_UUID_BASE        {0xCD, 0xA6, 0x13, 0x5B, 0x83, 0x50, 0x8D, 0x80, \
                              0x44, 0x40, 0xD3, 0x50, 0x00, 0x00, 0xB3, 0xB3}
#define PDLS_UUID_SERVICE     0x6901 /**< Peripheral Device Link Service */
#define PDLS_UUID_WRITE_CHAR  0x9101 /**< Write Message characteristic */
#define PDLS_UUID_IND_CHAR    0x9102 /**< Indicate Message characteristic */

#define PDLS_HEADER_SOURCE_Pos                7
#define PDLS_HEADER_CANCEL_Pos                6
#define PDLS_HEADER_SEQNUM_Pos                1
#define PDLS_HEADER_EXECUTE_Pos               0

/**@brief PDLS service type. */
typedef enum
{
    PDLS_SERVICE_PIS,                   /**< 0x00 Peripheral Device Property Information Service. */
    PDLS_SERVICE_NS,                    /**< 0x01 Peripheral Device Notification Service. */
    PDLS_SERVICE_OS,                    /**< 0x02 Peripheral Device Operation Service. */
    PDLS_SERVICE_SIS,                   /**< 0x03 Peripheral Device Sensor Information Service. */
    PDLS_SERVICE_SOS,                   /**< 0x04 Peripheral Device Setting Operation Service. */
    PDLS_SERVICE_MAX
} ble_pdls_service_type_t;

/**@brief PDLP Service structure. This structure contains various status information for the service. */
typedef struct ble_pdls_s
{
    uint16_t                    service_handle;       /**< Handle of PDLP Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t    write_char_handles;   /**< Handles related to the Write Message Characteristic. */
    ble_gatts_char_handles_t    ind_char_handles;     /**< Handles related to the Inidicate Message Characteristic. */
    uint8_t                     uuid_type;            /**< UUID type for the PDLP Service. */
    uint16_t                    conn_handle;          /**< Handle of the current connection (as provided by the BLE stack). BLE_CONN_HANDLE_INVALID if not in a connection. */
} ble_pdls_t;

// 
// PeripheralDevicePropertyInformation (PDPIS)
//
#define PDPIS_SERVICE_BITMASK_NONE             0x00
#define PDPIS_SERVICE_BITMASK_PIS              0x01
#define PDPIS_SERVICE_BITMASK_NS               0x02
#define PDPIS_SERVICE_BITMASK_OS               0x04
#define PDPIS_SERVICE_BITMASK_SIS              0x08
#define PDPIS_SERVICE_BITMASK_SOS              0x10

#define PDPIS_CAPABILITY_BITMASK_NONE          0x01
#define PDPIS_CAPABILITY_BITMASK_GYROSCOPE     0x02
#define PDPIS_CAPABILITY_BITMASK_ACCELERATOR   0x04
#define PDPIS_CAPABILITY_BITMASK_ORIENTATION   0x08
#define PDPIS_CAPABILITY_BITMASK_BATTERY       0x10
#define PDPIS_CAPABILITY_BITMASK_TEMPERATURE   0x20
#define PDPIS_CAPABILITY_BITMASK_HUMIDITY      0x40
#define PDPIS_CAPABILITY_BITMASK_RESERVED      0x80

#define PDPIS_GET_DEVICE_INFORMATION           0x0000
#define PDPIS_GET_DEVICE_INFORMATION_RESP      0x0001

/**@brief PDPIS parameters. */
typedef enum
{
    PDPIS_PARAM_RESULTCODE,
    PDPIS_PARAM_CANCEL,
    PDPIS_PARAM_SERVICELIST,
    PDPIS_PARAM_DEVICEID,
    PDPIS_PARAM_DEVICEUID,
    PDPIS_PARAM_DEVICECAPABILITY,
    PDPIS_PARAM_ORIGINAL_INFO,
    PDPIS_PARAM_EX_SENSROR,
    PDPIS_PARAM_MAX
} ble_pdpis_params_type_t;

//
// PeripheralDeviceOperationService (PDOS)
//
#define PDOS_NOTIFY_PD_OPERATION        0x0000

/**@brief PDOS parameters. */
typedef enum
{
    PDOS_PARAM_RESULTCODE,
    PDOS_PARAM_CANCEL,
    PDOS_PARAM_BUTTONID,
    PDOS_PARAM_MAX
} ble_pdos_params_type_t;

/**@brief PDOS Button ID */
typedef enum
{
    PDOS_BUTTON_ID_POWER,
    PDOS_BUTTON_ID_RETURN,
    PDOS_BUTTON_ID_ENTER,
    PDOS_BUTTON_ID_HOME,
    PDOS_BUTTON_ID_MENU,
    PDOS_BUTTON_ID_VOLUMEUP,
    PDOS_BUTTON_ID_VOLUMEDOWN,
    PDOS_BUTTON_ID_PLAY,
    PDOS_BUTTON_ID_PAUSE,
    PDOS_BUTTON_ID_STOP,
    PDOS_BUTTON_ID_FASTFORWARD,
    PDOS_BUTTON_ID_REWIND,
    PDOS_BUTTON_ID_SHUTTER,
    PDOS_BUTTON_ID_UP,
    PDOS_BUTTON_ID_DOWN,
    PDOS_BUTTON_ID_LEFT,
    PDOS_BUTTON_ID_RIGHT,
    PDOS_NOTIFY_CATEGORY_MAX
} ble_pdos_button_id_t;

//
// PeripheralDeviceSensorInformatioService (PDSIS)
//
#define PDSIS_GET_SENSOR_INFO                  0x0000
#define PDSIS_GET_SENSOR_INFO_RESP             0x0001
#define PDSIS_SET_NOTIFY_SENSOR_INFO           0x0002
#define PDSIS_SET_NOTIFY_SENSOR_INFO_RESP      0x0003
#define PDSIS_NOTIFY_PD_SENSOR_INFO            0x0004

#define PDSIS_SENSOR_BITMASK_NONE              0x00
#define PDSIS_SENSOR_BITMASK_GYROSCOPE         0x01
#define PDSIS_SENSOR_BITMASK_ACCELEROMETER     0x02
#define PDSIS_SENSOR_BITMASK_ORIENTATION       0x04
#define PDSIS_SENSOR_BITMASK_BATTERY           0x08
#define PDSIS_SENSOR_BITMASK_TEMPERATURE       0x10
#define PDSIS_SENSOR_BITMASK_HUMIDITY          0x20

/**@brief PDSIS parameters. */
typedef enum
{
    PDSIS_PARAM_RESULTCODE,
    PDSIS_PARAM_CANCEL,
    PDSIS_PARAM_SENSORTYPE,
    PDSIS_PARAM_STATUS,
    PDSIS_PARAM_X_VALUE,
    PDSIS_PARAM_Y_VALUE,
    PDSIS_PARAM_Z_VALUE,
    PDSIS_PARAM_X_THRESHOLD,
    PDSIS_PARAM_Y_THRESHOLD,
    PDSIS_PARAM_Z_THRESHOLD,
    PDSIS_PARAM_ORIGINALDATA,
    PDSIS_PARAM_MAX
} ble_pdsis_params_type_t;

/**@brief PDSIS Sensor Type */
typedef enum
{
    PDSIS_SENSOR_TYPE_GYROSCOPE,
    PDSIS_SENSOR_TYPE_ACCELEROMETER,
    PDSIS_SENSOR_TYPE_ORIENTATION,
    PDSIS_SENSOR_TYPE_BATTERY,
    PDSIS_SENSOR_TYPE_TEMPERATURE,
    PDSIS_SENSOR_TYPE_HUMIDITY,
    PDSIS_SETTING_MAX
} ble_pdsis_sensor_type_t;

/**@brief PDSIS Status */
typedef enum
{
    PDSIS_STATUS_OFF,
    PDSIS_STATUS_ON,
    PDSIS_STATUS_MAX
} ble_pdsis_status_t;

/**@brief PDSIS Notify value or threshold*/
typedef union
{
  struct value_s {
    uint32_t x_value;
    uint32_t y_value;
    uint32_t z_value;
  } value;
  struct {
    uint32_t x_threshold;
    uint32_t y_threshold;
    uint32_t z_threshold;
  } threshold;
  int8_t   s8_originaldata[12];
  uint8_t  u8_originaldata[12];
  int16_t  s16_originaldata[6];
  uint16_t u16_originaldata[6];
  int32_t  s32_originaldata[3];
  uint32_t u32_originaldata[3];
} ble_pdsis_notify_value_t;

/**@brief PDSIS events */
typedef enum
{
    PDSIS_EVT_SET_NOTIFY_INFO,
    PDSIS_EVT_GET_SENSOR_INFO
} ble_pdsis_event_t;

/**@brief PDSIS Event data structure */
typedef struct
{
  ble_pdsis_event_t         event;
  ble_pdsis_sensor_type_t   type;
  // below are either data to or from app
  ble_pdsis_status_t        status;
  ble_pdsis_notify_value_t  data;
} ble_pdsis_event_data_t;

typedef ble_pdls_result_code_t (*ble_pdsis_event_handler_t) (ble_pdls_t * p_pdls, ble_pdsis_event_data_t *p_pdsis_event);

//
// PeripheralDeviceNotificationService (PDNS)
//
#define PDNS_CONFIRM_NOTIFY_CATEGORY                    0x0000
#define PDNS_CONFIRM_NOTIFY_CATEGORY_RESP               0x0001
#define PDNS_NOTIFY_INFORMATION                         0x0002
#define PDNS_GET_PD_NOTIFY_DETAIL_DATA                  0x0003
#define PDNS_GET_PD_NOTIFY_DETAIL_DATA_RESP             0x0004
#define PDNS_NOTIFY_PD_GENERAL_INFORMATION              0x0005
#define PDNS_START_PD_APPLICATION                       0x0006
#define PDNS_START_PD_APPLICATION_RESP                  0x0007

#define PDNS_NOTIFY_CATEGORY_NOTNOTIFY                  0x0001
#define PDNS_NOTIFY_CATEGORY_ALL                        0x0002
#define PDNS_NOTIFY_CATEGORY_PHONEINCOMINGCALL          0x0004
#define PDNS_NOTIFY_CATEGORY_PHONEINCALL                0x0008
#define PDNS_NOTIFY_CATEGORY_PHONEIDLE                  0x0010
#define PDNS_NOTIFY_CATEGORY_MAIL                       0x0020
#define PDNS_NOTIFY_CATEGORY_SCHEDULE                   0x0040
#define PDNS_NOTIFY_CATEGORY_GENERAL                    0x0080
#define PDNS_NOTIFY_CATEGORY_ETC                        0x0100

#define PDNS_PARAMID_PHONEINCOMINGCALL_NOTIFYID         0x0001
#define PDNS_PARAMID_PHONEINCOMINGCALL_NOTIFYCATEGORY   0x0002
#define PDNS_PARAMID_PHONEINCALL_NOTIFYID               0x0001
#define PDNS_PARAMID_PHONEINCALL_NOTIFYCATEGORY         0x0002
#define PDNS_PARAMID_PHONEIDLE_NOTIFYID                 0x0001
#define PDNS_PARAMID_PHONEIDLE_NOTIFYCATEGORY           0x0002
#define PDNS_PARAMID_MAIL_APPNAME                       0x0001
#define PDNS_PARAMID_MAIL_APPNAMELOCAL                  0x0002
#define PDNS_PARAMID_MAIL_PACKAGE                       0x0004
#define PDNS_PARAMID_MAIL_TITLE                         0x0008
#define PDNS_PARAMID_MAIL_TEXT                          0x0010
#define PDNS_PARAMID_MAIL_SENDER                        0x0020
#define PDNS_PARAMID_MAIL_SENDERADDRESS                 0x0040
#define PDNS_PARAMID_MAIL_RECEIVEDATE                   0x0080
#define PDNS_PARAMID_MAIL_NOTIFYID                      0x0100
#define PDNS_PARAMID_MAIL_NOTIFYCATEGORY                0x0200
#define PDNS_PARAMID_SCHEDULE_APPNAME                   0x0001
#define PDNS_PARAMID_SCHEDULE_APPNAMELOCAL              0x0002
#define PDNS_PARAMID_SCHEDULE_PACKAGE                   0x0004
#define PDNS_PARAMID_SCHEDULE_TITLE                     0x0008
#define PDNS_PARAMID_SCHEDULE_STARTDATE                 0x0010
#define PDNS_PARAMID_SCHEDULE_ENDDATE                   0x0020
#define PDNS_PARAMID_SCHEDULE_AREA                      0x0040
#define PDNS_PARAMID_SCHEDULE_PERSON                    0x0080
#define PDNS_PARAMID_SCHEDULE_TEXT                      0x0100
#define PDNS_PARAMID_SCHEDULE_CONTENTS1                 0x0200
#define PDNS_PARAMID_SCHEDULE_CONTENTS2                 0x0400
#define PDNS_PARAMID_SCHEDULE_CONTENTS3                 0x0800
#define PDNS_PARAMID_SCHEDULE_NOTIFYID                  0x1000
#define PDNS_PARAMID_SCHEDULE_NOTIFYCATEGORY            0x2000
#define PDNS_PARAMID_GENERAL_APPNAME                    0x0001
#define PDNS_PARAMID_GENERAL_APPNAMELOCAL               0x0002
#define PDNS_PARAMID_GENERAL_PACKAGE                    0x0004
#define PDNS_PARAMID_GENERAL_TITLE                      0x0008
#define PDNS_PARAMID_GENERAL_TEXT                       0x0010
#define PDNS_PARAMID_GENERAL_NOTIFYID                   0x0020
#define PDNS_PARAMID_GENERAL_NOTIFYCATEGORY             0x0040
#define PDNS_PARAMID_ETC_APPNAME                        0x0001
#define PDNS_PARAMID_ETC_APPNAMELOCAL                   0x0002
#define PDNS_PARAMID_ETC_PACKAGE                        0x0004
#define PDNS_PARAMID_ETC_CONTENTS1                      0x0008
#define PDNS_PARAMID_ETC_CONTENTS2                      0x0010
#define PDNS_PARAMID_ETC_CONTENTS3                      0x0020
#define PDNS_PARAMID_ETC_CONTENTS4                      0x0040
#define PDNS_PARAMID_ETC_CONTENTS5                      0x0080
#define PDNS_PARAMID_ETC_CONTENTS6                      0x0100
#define PDNS_PARAMID_ETC_CONTENTS7                      0x0200
#define PDNS_PARAMID_ETC_MIMETYPEFORMEDIA               0x0400
#define PDNS_PARAMID_ETC_MEDIA                          0x0800
#define PDNS_PARAMID_ETC_MIMETYPEFORIMAGE               0x1000
#define PDNS_PARAMID_ETC_IMAGE                          0x2000
#define PDNS_PARAMID_ETC_NOTIFYID                       0x4000
#define PDNS_PARAMID_ETC_NOTIFYCATEGORY                 0x8000

/**@brief PDNS parameters. */
typedef enum
{
    PDNS_PARAM_RESULTCODE,
    PDNS_PARAM_CANCEL,
    PDNS_PARAM_GETSTATUS,
    PDNS_PARAM_NOTIFYCATEGORY,
    PDNS_PARAM_NOTIFYCATEGORYID,
    PDNS_PARAM_GETPARAMETERID,
    PDNS_PARAM_GETPARAMETERLENGTH,
    PDNS_PARAM_PARAMETERIDLIST,
    PDNS_PARAM_UNIQUEID,
    PDNS_PARAM_NOTIFYID,
    PDNS_PARAM_NOTIFICATIONOPERATION,
    PDNS_PARAM_TITLE,
    PDNS_PARAM_TEXT,
    PDNS_PARAM_APPNAME,
    PDNS_PARAM_APPNAMELOCAL,
    PDNS_PARAM_NOTIFYAPP,
    PDNS_PARAM_RUMBLINGSETTING,
    PDNS_PARAM_VABRATIONPATTERN,
    PDNS_PARAM_LEDPATTERN,
    PDNS_PARAM_SENDER,
    PDNS_PARAM_SENDERADDRESS,
    PDNS_PARAM_RECEIVEDATE,
    PDNS_PARAM_STARTDATE,
    PDNS_PARAM_ENDDATE,
    PDNS_PARAM_AREA,
    PDNS_PARAM_PERSON,
    PDNS_PARAM_MIMETYPEFORIMAGE,
    PDNS_PARAM_MIMETYPEFORMEDIA,
    PDNS_PARAM_IMAGE,
    PDNS_PARAM_CONTENTS1,
    PDNS_PARAM_CONTENTS2,
    PDNS_PARAM_CONTENTS3,
    PDNS_PARAM_CONTENTS4,
    PDNS_PARAM_CONTENTS5,
    PDNS_PARAM_CONTENTS6,
    PDNS_PARAM_CONTENTS7,
    PDNS_PARAM_CONTENTS8,
    PDNS_PARAM_CONTENTS9,
    PDNS_PARAM_CONTENTS10,
    PDNS_PARAM_MEDIA,
    PDNS_PARAM_PACKAGE,
    PDNS_PARAM_CLASS,
    PDNS_PARAM_SHARINGINFORMATION,
    PDNS_PARAM_INVALID
} ble_pdns_params_type_t;

/**@brief PDNS STATUS code */
typedef enum
{
    PDNS_STATUS_OK,
    PDNS_STATUS_PARTIAL_OK,
    PDNS_STATUS_CANCEL,
    PDNS_STATUS_ERROR_FAILED,
    PDNS_STATUS_ERROR_UNKOWN,
    PDNS_STATUS_ERROR_NO_DATA,
    PDNS_STATUS_ERROR_NOT_SUPPORTED,
    PDNS_STATUS_MAX
} ble_pdns_status_t;

/**@brief PDNS Rumbling Setting code */
typedef enum
{
    PDNS_RUMBLING_SETTING_LED,
    PDNS_RUMBLING_SETTING_VIBRATION,
    PDNS_RUMBLING_SETTING_MAX
} ble_pdns_rumbling_setting_t;

/**@brief PDNS notify information */
typedef struct
{
  uint16_t notifycategory;
  uint16_t uniqueid;
  uint16_t parameteridlist;
  ble_pdns_rumbling_setting_t rumblingsetting;
  uint8_t vibratiobpattern[4];  //TBD
  uint8_t ledpattern[5];        //TBD
} ble_pdns_notify_info_t;

/**@brief PDNS get notify detail data response */
typedef struct
{
  ble_pdls_result_code_t result;
  uint16_t uniqueid;
  pdlp_opaque_t data;
} ble_pdns_notify_detail_resp_t;

/**@brief PDNS start app response */
typedef struct
{
  ble_pdls_result_code_t result;
} ble_pdns_start_app_resp_t;

/**@brief PDSIS events */
typedef enum
{
    PDNS_EVT_NOTIFY_INFO,
    PDNS_EVT_GET_PD_NOTIFY_DETAIL_DATA_RESP,
    PDNS_EVT_START_PD_APP_RESP
} ble_pdns_event_t;

/**@brief PDNS event data structure*/
typedef struct
{
  ble_pdns_event_t          event;
  union
  {
    ble_pdns_notify_info_t  notifyinfo;
    ble_pdns_notify_detail_resp_t notifydetail;
    ble_pdns_start_app_resp_t startappresult;
  } data;
} ble_pdns_event_data_t;

typedef ble_pdls_result_code_t (*ble_pdns_event_handler_t) (ble_pdls_t * p_pdls, ble_pdns_event_data_t *p_pdns_event);

//
// PeripheralDeviceSettingOperationService (PDSOS)
//
#define PDSOS_GET_APP_VERSION                  0x0000
#define PDSOS_GET_APP_VERSION_RESP             0x0001
#define PDSOS_CONFIRM_INSTALL_APP              0x0002
#define PDSOS_CONFIRM_INSTALL_APP_RESP         0x0003
#define PDSOS_GET_SETTING_INFORMATION          0x0004
#define PDSOS_GET_SETTING_INFORMATION_RESP     0x0005
#define PDSOS_GET_SETTING_NAME                 0x0006
#define PDSOS_GET_SETTING_NAME_RESP            0x0007
#define PDSOS_SELECT_SETTING_INFORMATION       0x0008
#define PDSOS_SELECT_SETTING_INFORMATION_RESP  0x0009

#define PDSOS_SETTING_VALUE_ID_LED             0x00
#define PDSOS_SETTING_VALUE_ID_VIBRATOR        0x01

#define PDSOS_SETTING_REQ_ID_SETTING           0x00
#define PDSOS_SETTING_REQ_ID_START_DEMO        0x01
#define PDSOS_SETTING_REQ_ID_STOP_DEMO         0x02

/**@brief PDSOS parameters. */
typedef enum
{
    PDSOS_PARAM_RESULTCODE,
    PDSOS_PARAM_CANCEL,
    PDSOS_PARAM_SETTINGNAMETYPE,
    PDSOS_PARAM_APPNAME,
    PDSOS_PARAM_FILEVER,
    PDSOS_PARAM_FILESIZE,
    PDSOS_PARAM_INSTALLCONFIRMSTATUS,
    PDSOS_PARAM_SETTINGINFORMATIONREQUEST,
    PDSOS_PARAM_SETTINGINFORMATIONDATA,
    PDSOS_PARAM_SETTINGNAMEDATA,
    PDSOS_PARAM_MAX
} ble_pdsos_params_type_t;

/**@brief PDSOS Setting Name Type */
typedef enum
{
    PDSOS_SETTING_LEDCOLORNAME,
    PDSOS_SETTING_LEDPATTERNNAME,
    PDSOS_SETTING_VIBRATIONPATTERNNAME,
    PDSOS_SETTING_MAX
} ble_pdsos_setting_name_type_t;

/**@brief PDSOS Install Confirm Status */
typedef enum
{
    PDSOS_INSTALL_CONFIRM_STATUS_OK,
    PDSOS_INSTALL_CONFIRM_STATUS_CANCEL,
    PDSOS_INSTALL_CONFIRM_STATUS_ERROR_FAILED,
    PDSOS_INSTALL_CONFIRM_STATUS_ERROR_UNKOWN,
    PDSOS_INSTALL_CONFIRM_STATUS_ERROR_NO_DATA,
    PDSOS_INSTALL_CONFIRM_STATUS_ERROR_NOT_SUPPORTED,
    PDSOS_INSTALL_CONFIRM_STATUS_ERROR_NO_SPACE,
    PDSOS_INSTALL_CONFIRM_STATUS_ERROR_ALREADY_INSTTALLED,
    PDSOS_INSTALL_CONFIRM_STATUS_ERROR_VERSION_OLD,
    PDSOS_INSTALL_CONFIRM_STATUS_MAX
} ble_pdsos_status_t;

/**@brief PDSOS setting notification time */
typedef enum
{
    PDSOS_NOTIFY_TIME_OFF        = 0x00,
    PDSOS_NOTIFY_TIME_5SEC       = 0x05,
    PDSOS_NOTIFY_TIME_10SEC      = 0x0A,
    PDSOS_NOTIFY_TIME_30SEC      = 0x1E,
    PDSOS_NOTIFY_TIME_1MIN       = 0x3C,
    PDSOS_NOTIFY_TIME_3MIN       = 0xB4,
} ble_pdsos_setting_notify_time_t;

/**@brief PDSOS LED setting info. */
typedef struct
{
    uint8_t color_num;
    uint8_t color_selected;   // 0x01~
    uint8_t pattern_num;
    uint8_t pattern_selected; // 0x01~
} ble_pdsos_led_setting_info;

/**@brief PDSOS Vibrator setting info. */
typedef struct
{
    uint8_t pattern_num;
    uint8_t pattern_selected;
} ble_pdsos_vibrator_setting_info;

/**@brief PDSOS setting info. */
typedef struct
{
    uint8_t setting_id;   //0: LED; 1: Vibration
    uint8_t notify_time;  //ble_pdsos_setting_notify_time_t
    union {
      ble_pdsos_led_setting_info led_setting;
      ble_pdsos_vibrator_setting_info vibrator_setting;
    } setting;
} ble_pdsos_setting_info;

/**@brief PDSOS setting name*/
typedef struct
{
    pdlp_opaque_t setting;
} ble_pdsos_setting_name;

/**@brief PDSOS events */
typedef enum
{
    PDSOS_EVT_GET_SETTING_INFO,
    PDSOS_EVT_GET_SETTING_NAME,
    PDSOS_EVT_SELECT_SETTING_INFO
} ble_pdsos_event_t;

/**@brief PDSOS Event data structure */
typedef struct
{
  ble_pdsos_event_t         event;
  union {
    ble_pdsos_setting_name_type_t setting_name_type;
    uint8_t setting_info_request;
    ble_pdsos_setting_info setting_info;
  } data;
} ble_pdsos_event_data_t;

typedef ble_pdls_result_code_t (*ble_pdsos_event_handler_t) (ble_pdls_t * p_pdls, ble_pdsos_event_data_t *p_pdsos_event);

/** @brief PDLP Service init structure. This structure contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    //PDPIS
    uint8_t   servicelist;      /**< Service list. */
    uint16_t  deviceid;         /**< Device ID. */
    uint32_t  deviceuid;        /**< Device UID. */
    uint8_t   devicecapability; /**< Device Capability. */
    uint8_t   *originalinfo;    /**< Device original Information. */
    uint8_t   exsensortype;     /**< Extended Sensor Type info. */
    //PDNS
    uint16_t  notifycategory;   /**< Notify category */
    ble_pdns_event_handler_t    pdns_event_handler;
    //PDSIS
    uint8_t   sensortypes;      /**< Sensor types */
    ble_pdsis_event_handler_t   pdsis_event_handler;
    //PDSOS
    ble_pdsos_event_handler_t   pdsos_event_handler;
} ble_pdls_init_t;

/**@brief Function for initializing the PDLP Service.
 *
 * @param[out] p_pdls     PDLP Service structure. This structure must be supplied by
 *                        the application. It is initialized by this function and will later
 *                        be used to identify this particular service instance.
 * @param[in] p_pdls_init  Information needed to initialize the service.
 *
 * @retval NRF_SUCCESS If the service was initialized successfully. Otherwise, an error code is returned.
 */
uint32_t ble_pdls_init(ble_pdls_t * p_pdls, const ble_pdls_init_t * p_pdls_init);

/**@brief Function for handling the application's BLE stack events.
 *
 * @details This function handles all events from the BLE stack that are of interest to the PDLP Service.
 *
 * @param[in] p_pdlp     PDLP Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
void ble_pdls_on_ble_evt(ble_pdls_t * p_pdls, ble_evt_t * p_ble_evt);

/**@brief Function for PDOS device operation notification
 *
 * @param[in] p_pdls      PDLP Service structure. This structure must be supplied by
 *                        the application.
 * @param[in] button_id   Button ID to be notified to PDLP Client.
 *
 * @retval NRF_SUCCESS If the service was handled successfully. Otherwise, an error code is returned.
 */
uint32_t ble_pdls_pdos_notify(ble_pdls_t * p_pdls, ble_pdos_button_id_t button_id);

/**@brief Function for PDSIS sensor information notification
 *
 * @param[in] p_pdls          PDLP Service structure. This data must be supplied by the application.
 * @param[in] sensor_type     Type of sensor
 * @param[in] p_notify_value  Sensor data to be notified to PDLP Client.
 *
 * @retval NRF_SUCCESS If the service was handled successfully. Otherwise, an error code is returned.
 */
uint32_t ble_pdls_pdsis_notify(ble_pdls_t * p_pdls, ble_pdsis_sensor_type_t sensor_type, ble_pdsis_notify_value_t *p_notify_value);

/**@brief Function for PDNS, get the notification details from PDLP Client 
 *
 * @param[in] p_pdls      PDLP Service structure. This data must be supplied by the application.
 * @param[in] unique_id   Unique ID for indentification
 * @param[in] param_id    Parameter ID to get details for
 * @param[in] param_len   Length of parameter details, if known
 *
 * @retval NRF_SUCCESS If the service was initialized successfully. Otherwise, an error code is returned.
 */
uint32_t ble_pdls_pdns_get_pd_notify_detail_data(ble_pdls_t * p_pdls, uint16_t unique_id, uint8_t param_id, uint32_t param_len);

/**@brief Function for PDNS, start an application on PDLP Client 
 *
 * @param[in] p_pdls      PDLP Service structure. This data must be supplied by the application.
 * @param[in] p_package   Package name of notification source application
 * @param[in] p_notifyapp Package name of notification destination application
 * @param[in] p_class     Name of class
 * @param[in] p_sharing_info   Sharing info (optional)
 *
 * @retval NRF_SUCCESS If the service was handled successfully. Otherwise, an error code is returned.
 */
uint32_t ble_pdls_pdns_start_pd_app(ble_pdls_t *p_pdls, pdlp_opaque_t *p_package, pdlp_opaque_t *p_notifyapp, 
         pdlp_opaque_t *p_class, pdlp_opaque_t *p_sharing_info);

/**@brief Function for PDSOS, response of get seting info request from PDLP Client 
 *
 * @param[in] p_pdls      PDLP Service structure. This data must be supplied by the application.
 * @param[in] result      Handling result on local device
 * @param[in] p_setting_info Setting information
 *
 * @retval NRF_SUCCESS If the service was initialized successfully. Otherwise, an error code is returned.
 */
uint32_t ble_pdls_pdsos_get_setting_info_resp(ble_pdls_t * p_pdls, ble_pdls_result_code_t result, ble_pdsos_setting_info* p_setting_info);

/**@brief Function for PDSOS, response of get seting name request from PDLP Client 
 *
 * @param[in] p_pdls      PDLP Service structure. This data must be supplied by the application.
 * @param[in] result      Handling result on local device
 * @param[in] p_setting_name Setting name(s)
 *
 * @retval NRF_SUCCESS If the service was handled successfully. Otherwise, an error code is returned.
 */
uint32_t ble_pdls_pdsos_get_setting_name_resp(ble_pdls_t * p_pdls, ble_pdls_result_code_t result, ble_pdsos_setting_name* p_setting_name);

/**@brief Function for PDSOS, response of select seting info request from PDLP Client 
 *
 * @param[in] p_pdls      PDLP Service structure. This data must be supplied by the application.
 * @param[in] result      Handling result on local device
 *
 * @retval NRF_SUCCESS If the service was handled successfully. Otherwise, an error code is returned.
 */
uint32_t ble_pdls_pdsos_select_setting_info_resp(ble_pdls_t * p_pdls, ble_pdls_result_code_t result);

#endif // BLE_PDLP_H__

/** @} */
