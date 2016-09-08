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

#include "ble_srv_common.h"
#include "sdk_common.h"
#include "ble_pdlp.h"

#define ERROR_CHECK(err_code)                           \
    do                                                  \
    {                                                   \
        if ((ble_pdls_result_code_t)err_code != PDLS_RESULT_OK)           \
        {                                               \
            return err_code;                            \
        }                                               \
    } while (0)

#define PARAM_UINT8_LENGTH            5
#define PARAM_UINT16_LENGTH           6
#define PARAM_UINT32_LENGTH           8
    
#define MAX_BLE_DATA_PACKET           5
#define MAX_BLE_CMD_PACKET_SIZE      (GATT_MTU_SIZE_DEFAULT - 3)      //20
#define MAX_BLE_RSP_PACKET_SIZE      (GATT_MTU_SIZE_DEFAULT - 3 - 1)  //19
static uint8_t m_cmd_buf[MAX_BLE_DATA_PACKET*MAX_BLE_CMD_PACKET_SIZE];
static uint8_t m_rsp_buf[MAX_BLE_DATA_PACKET*MAX_BLE_RSP_PACKET_SIZE];
static uint8_t m_current_packet     = 0;
static uint8_t *m_data_pos          = m_cmd_buf;   // Pointer where to save next BLE data packet
static uint8_t m_data_size          = 0;           // Total PDLP data received (excluding header)
    
static uint8_t m_pdns_param_id      = PDNS_PARAM_INVALID; // PDNS current parameter id for detail fetching

/**@brief PDLS service type. */
static enum
{
    PDLS_STATE_IDLE,
    PDLS_STATE_WRITING,
    PDLS_STATE_INDICATING}       m_transmit_state = PDLS_STATE_IDLE;
static bool                      m_indication_confirmed = false;

static ble_pdls_init_t m_pdlp_service;

// Forward declaration
static void service_reset(void);
static uint32_t handle_transmit_written(ble_pdls_t * p_pdls);
static ble_pdls_result_code_t PDPIS_service_handler(uint16_t msgid, uint16_t *rsp_len);
static ble_pdls_result_code_t PDSIS_service_handler(ble_pdls_t * p_pdls, uint16_t msgid, uint8_t param_num, uint16_t *rsp_len);
static ble_pdls_result_code_t PDNS_service_handler(ble_pdls_t * p_pdls, uint16_t msgid, uint8_t param_num, uint16_t *rsp_len);
static ble_pdls_result_code_t PDSOS_service_handler(ble_pdls_t * p_pdls, uint16_t msgid, uint8_t param_num, uint16_t *rsp_len);

/**@brief Function to send an Error or Cancel message to PDLP Client.
 *
 * @param[in] p_pdls     PDLP Service structure.
 * @param[in] error_or_cancelled  PDLP Error or cancel result code
 * @param[in] service             PDLP service type
 * @param[in] msgid               Message ID in above PDLP service
 *
 * @retval NRF_SUCCESS If BLE indication is sent successfully. Otherwise, an error code is returned.
 */
#define MSG_LENGTH_NACK_INDICATION  10
static uint32_t indicate_nack(ble_pdls_t * p_pdls, 
                              uint8_t error_or_cancelled,
                              ble_pdls_service_type_t service,
                              uint8_t msgid)
{
    ble_gatts_hvx_params_t        params;
    uint8_t                       header;
    uint8_t                       data[MSG_LENGTH_NACK_INDICATION];
    uint16_t                      len = MSG_LENGTH_NACK_INDICATION;

    header  = 1<<PDLS_HEADER_SOURCE_Pos; // Server indication
    header |= (error_or_cancelled == PDLS_RESULT_CANCEL)<<PDLS_HEADER_CANCEL_Pos; // Cancel or not
    header |= 0<<PDLS_HEADER_SEQNUM_Pos; // First packet
    header |= 1<<PDLS_HEADER_EXECUTE_Pos;// One packet only
    
    memset(&params, 0, sizeof(params));
    params.type = BLE_GATT_HVX_INDICATION;
    params.handle = p_pdls->ind_char_handles.value_handle;
    data[0] = header;
    // service header
    data[1] = (uint8_t)service;
    data[2] = msgid & 0xFF;
    data[3] = msgid >> 8;
    data[4] = 0x01;
    // result
    data[5] = PDSOS_PARAM_RESULTCODE;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = error_or_cancelled;
    
    params.p_data = data;
    params.p_len = &len;
    m_indication_confirmed = false;
    m_data_size = MSG_LENGTH_NACK_INDICATION;   // To make sure service is reset after confirmation 
    return sd_ble_gatts_hvx(p_pdls->conn_handle, &params);
}

/**@brief Function to send a normal message (prepared in global data structure) to PDLP Client.
 *
 * @param[in] p_pdls     PDLP Service structure.
 *
 * @retval NRF_SUCCESS If BLE indication is sent successfully. Otherwise, an error code is returned.
 */
static uint32_t indicate_ack(ble_pdls_t * p_pdls)
{
    ble_gatts_hvx_params_t        params;
    uint16_t                      len;
    uint8_t                       data[MAX_BLE_CMD_PACKET_SIZE];

    if (m_data_size > MAX_BLE_RSP_PACKET_SIZE)
    {
      // insert header
      data[0]  = 1<<PDLS_HEADER_SOURCE_Pos; // Server indication
      data[0] |= 0<<PDLS_HEADER_CANCEL_Pos; // No cancel
      data[0] |= m_current_packet<<PDLS_HEADER_SEQNUM_Pos;
      data[0] |= 0<<PDLS_HEADER_EXECUTE_Pos;// Multiple packet
      
      len = MAX_BLE_RSP_PACKET_SIZE + 1;
      m_transmit_state = PDLS_STATE_INDICATING;
    }
    else
    {
      // insert header
      data[0]  = 1<<PDLS_HEADER_SOURCE_Pos; // Server indication
      data[0] |= 0<<PDLS_HEADER_CANCEL_Pos; // No cancel
      data[0] |= m_current_packet<<PDLS_HEADER_SEQNUM_Pos;
      data[0] |= 1<<PDLS_HEADER_EXECUTE_Pos;// Last packet
      
      len = m_data_size + 1;
    }
    
    memset(&params, 0, sizeof(params));
    params.type     = BLE_GATT_HVX_INDICATION;
    params.handle   = p_pdls->ind_char_handles.value_handle;
    memcpy(&data[1], m_data_pos, len-1);
    params.p_data   = data;
    params.p_len    = &len;
    
    m_indication_confirmed = false;
    return sd_ble_gatts_hvx(p_pdls->conn_handle, &params);
}

/**@brief Function for handling the Connect event.
 *
 * @param[in] p_pdls      LED Button Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
static void on_connect(ble_pdls_t * p_pdls, ble_evt_t * p_ble_evt)
{
    p_pdls->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in] p_pdls     PDLP Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
static void on_disconnect(ble_pdls_t * p_pdls, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_pdls->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the Indication Confirmation event.
 *
 * @param[in] p_pdls     PDLP Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
static void on_confirm(ble_pdls_t * p_pdls, ble_evt_t * p_ble_evt)
{
  if (p_pdls->conn_handle == p_ble_evt->evt.gatts_evt.conn_handle)
  {
    m_indication_confirmed = true;
    if (m_data_size > MAX_BLE_RSP_PACKET_SIZE)
    {
      // send next indication
      m_data_size -= MAX_BLE_RSP_PACKET_SIZE;
      m_data_pos  += MAX_BLE_RSP_PACKET_SIZE;
      m_current_packet++;
      // send next indication
      indicate_ack(p_pdls);
    }
    else
    {
      // Either NACK or last Indication
      m_transmit_state = PDLS_STATE_IDLE;
      service_reset();
    }
  }
}
  
/**@brief Function for handling the Indication timeout event.
 *
 * @param[in] p_pdls     PDLP Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
static void on_timeout(ble_pdls_t * p_pdls, ble_evt_t * p_ble_evt)
{
  //To-Do
}

/**@brief Function for handling the BLE ATT Write event.
 *
 * @param[in] p_pdls     PDLP Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
static void on_write(ble_pdls_t * p_pdls, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    if ((p_evt_write->handle == p_pdls->write_char_handles.value_handle) && (p_evt_write->len > 1))
    {
        uint8_t header = p_evt_write->data[0];
        if (((header>>PDLS_HEADER_SOURCE_Pos)&0x01) == 1)
        {
          indicate_nack(p_pdls, PDLS_RESULT_ERROR_NOT_SUPPORT,
              (ble_pdls_service_type_t)p_evt_write->data[1], p_evt_write->data[2]);
          return; // Wrong message, should be 0 i.e. from Client
        }
        if (((header>>PDLS_HEADER_CANCEL_Pos)&0x01) == 1)
        {
          if (m_transmit_state == PDLS_STATE_WRITING || m_transmit_state == PDLS_STATE_INDICATING)
          {
              indicate_nack(p_pdls, PDLS_RESULT_CANCEL,
                  (ble_pdls_service_type_t)p_evt_write->data[1], p_evt_write->data[2]);
          }
        }
        if (((header>>PDLS_HEADER_EXECUTE_Pos)&0x01) == 0)
        {
          m_current_packet = ((header>>PDLS_HEADER_SEQNUM_Pos)&0x01F);
          m_transmit_state = PDLS_STATE_WRITING;  // allow cancel
          // copy PDLP data, exclude the header
          memcpy(m_data_pos, p_evt_write->data + 1, p_evt_write->len - 1);
          m_data_pos  += p_evt_write->len - 1;  // pointer
          m_data_size += p_evt_write->len - 1;  // size
        }
        else
        {
          m_current_packet = 0;
          // copy PDLP data, exclude the header
          memcpy(m_data_pos, p_evt_write->data + 1, p_evt_write->len - 1);
          m_data_size += p_evt_write->len - 1;
          handle_transmit_written(p_pdls);
        }
    }
}

/**@brief Function for PDLP service reset after one request/respone transaction
 *
 */
static void service_reset()
{
    memset(m_cmd_buf, 0x00, MAX_BLE_DATA_PACKET*MAX_BLE_CMD_PACKET_SIZE);
    memset(m_rsp_buf, 0x00, MAX_BLE_DATA_PACKET*MAX_BLE_RSP_PACKET_SIZE);
    m_current_packet    = 0;
    m_data_pos          = m_cmd_buf;
    m_data_size         = 0;
    m_transmit_state    = PDLS_STATE_IDLE;
}

/**@brief Function for handling the PDLP Write done, i.e. after all messages received.
 *
 * @param[in] p_pdls     PDLP Service structure.
 *
 * @retval PDLS_RESULT_OK If the message is handled successfully. Otherwise, an error code is returned.
 */
static uint32_t handle_transmit_written(ble_pdls_t * p_pdls)
{
    ble_pdls_result_code_t result;
    ble_pdls_service_type_t service;
    uint16_t msgid;
    uint8_t param_num;
    uint16_t len = 0;
    
    // check service header
    if (m_data_size < 4)
    {
      return  indicate_nack(p_pdls, (uint8_t)PDLS_RESULT_ERROR_NO_DATA, 
          (ble_pdls_service_type_t)m_cmd_buf[0], m_cmd_buf[1]);
    }
    else
    {
      service = (ble_pdls_service_type_t)m_cmd_buf[0];
      msgid = (*(m_cmd_buf+1) | *(m_cmd_buf+2)<<8);
      param_num = *(m_cmd_buf+3);
      m_data_pos = m_cmd_buf+4;      // start of params buffer      
    }
    
    // service dispatch
    switch (service)
    {
      case PDLS_SERVICE_PIS:
        result = PDPIS_service_handler(msgid, &len);
        break;
      case PDLS_SERVICE_NS:
        result = PDNS_service_handler(p_pdls, msgid, param_num, &len);
        break;
      case PDLS_SERVICE_SOS:
        result = PDSOS_service_handler(p_pdls, msgid, param_num, &len);
        break;
      case PDLS_SERVICE_SIS:
        result = PDSIS_service_handler(p_pdls,  msgid, param_num, &len);
        break;
      case PDLS_SERVICE_OS:
      default:
        result = PDLS_RESULT_ERROR_NOT_SUPPORT;
    }
    
    // send ACK or NACK
    if (PDLS_RESULT_OK != result)
    {
      return indicate_nack(p_pdls, (uint8_t)result, 
          (ble_pdls_service_type_t)m_cmd_buf[0], m_cmd_buf[1]);
    }
    else if (len > 0)
    {
      m_data_pos       = m_rsp_buf;
      m_data_size      = len;
      m_current_packet = 0;
      return indicate_ack(p_pdls);
    }
    
    return result;
}

/**@brief Function for adding the Write Message Characteristic.
 *
 * @param[in] p_pdls      PDLP Service structure.
 * @param[in] p_pdls_init PDLP Service initialization structure.
 *
 * @retval NRF_SUCCESS on success, else an error value from the SoftDevice
 */
static uint32_t write_char_add(ble_pdls_t * p_pdls, const ble_pdls_init_t * p_pdls_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.write  = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;

    ble_uuid.type = p_pdls->uuid_type;
    ble_uuid.uuid = PDLS_UUID_WRITE_CHAR;
    
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = MAX_BLE_CMD_PACKET_SIZE;
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_pdls->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_pdls->write_char_handles);
}


/**@brief Function for adding the Indicate Message Characteristic.
 *
 * @param[in] p_pdls      PDLP Service structure.
 * @param[in] p_pdls_init PDLP Service initialization structure.
 *
 * @retval NRF_SUCCESS on success, else an error value from the SoftDevice
 */
static uint32_t ind_char_add(ble_pdls_t * p_pdls, const ble_pdls_init_t * p_pdls_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    
    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.indicate = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;

    ble_uuid.type = p_pdls->uuid_type;
    ble_uuid.uuid = PDLS_UUID_IND_CHAR;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = MAX_BLE_CMD_PACKET_SIZE;
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_pdls->service_handle,
                                               &char_md,
                                               &attr_char_value,
                                               &p_pdls->ind_char_handles);
}

/**@brief Function for handling a PDPIS request.
 *
 * @param[in]  msgid       PDPIS message ID.
 * @param[out] rsp_len     Prepared response message length.
 *
 * @retval PDLS_RESULT_OK on success, else an error value
 */
static ble_pdls_result_code_t PDPIS_service_handler(uint16_t msgid, uint16_t *rsp_len)
{
    // check msgid
    if (msgid != PDPIS_GET_DEVICE_INFORMATION)
    {
      return PDLS_RESULT_ERROR_NOT_SUPPORT;
    }
    m_data_pos = m_rsp_buf;
    // Prepare response, first service header
    m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_PIS, PDPIS_GET_DEVICE_INFORMATION_RESP, 5);
    // param #1 Reult code
    m_data_pos += pdls_encode_param_uint8(m_data_pos, PDPIS_PARAM_RESULTCODE, PDLS_RESULT_OK);
    // param #2 Service List
    m_data_pos += pdls_encode_param_uint8(m_data_pos, PDPIS_PARAM_SERVICELIST, m_pdlp_service.servicelist);
    // param #3 Device ID
    m_data_pos += pdls_encode_param_uint16(m_data_pos, PDPIS_PARAM_DEVICEID, m_pdlp_service.deviceid);
    // param #4 Device Uid
    m_data_pos += pdls_encode_param_uint32(m_data_pos, PDPIS_PARAM_DEVICEUID, m_pdlp_service.deviceuid);
    // param #5 Device Capability
    m_data_pos += pdls_encode_param_uint8(m_data_pos, PDPIS_PARAM_DEVICECAPABILITY, m_pdlp_service.devicecapability);
    
    *rsp_len   = m_data_pos - m_rsp_buf;
    return PDLS_RESULT_OK;
}

/**@brief Function for handling a PDSIS request.
 *
 * @param[in]  p_pdls      PDLP Service structure.
 * @param[in]  msgid       PDSIS message ID.
 * @param[in]  param_num   Number of parameters in this PDSIS message
 * @param[out] rsp_len     Prepared response message length.
 *
 * @retval PDLS_RESULT_OK on success, else an error value
 */
static ble_pdls_result_code_t PDSIS_service_handler(ble_pdls_t * p_pdls, uint16_t msgid, uint8_t param_num, uint16_t *rsp_len)
{
    ble_pdsis_event_data_t event_data;
    ble_pdls_result_code_t result;
    uint8_t  paramindex = 0;

     // check msgid
    switch (msgid)
    {
      case PDSIS_GET_SENSOR_INFO:
      {
          // Check sensor type
          result = pdls_decode_param_uint8(m_data_pos, PDSIS_PARAM_SENSORTYPE, (uint8_t *)&event_data.type);
          ERROR_CHECK(result);
          if ((m_pdlp_service.sensortypes & (0x1<<event_data.type)) == 0)
          {
            // Sensor type not supported
            return PDLS_RESULT_ERROR_NOT_SUPPORT;
          }
          // Send the request to App
          event_data.event = PDSIS_EVT_GET_SENSOR_INFO;
          result = m_pdlp_service.pdsis_event_handler(p_pdls, &event_data);
          // Prepare response, first service header
          m_data_pos = m_rsp_buf;
          if (result == PDLS_RESULT_OK)
          {
            if (event_data.type == PDSIS_SENSOR_TYPE_GYROSCOPE ||
                event_data.type == PDSIS_SENSOR_TYPE_ACCELEROMETER ||
                event_data.type == PDSIS_SENSOR_TYPE_ORIENTATION )
            {
                // Service header
                m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SIS, PDSIS_SET_NOTIFY_SENSOR_INFO_RESP, 4);
                // param #1 Reult code
                m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSIS_PARAM_RESULTCODE, PDLS_RESULT_OK);
                // param #2 X-value
                m_data_pos += pdls_encode_param_uint32(m_data_pos, PDSIS_PARAM_X_VALUE, event_data.data.value.x_value);
                // param #3 Y-value
                m_data_pos += pdls_encode_param_uint32(m_data_pos, PDSIS_PARAM_Y_VALUE, event_data.data.value.y_value);
                // param #4 Z-value
                m_data_pos += pdls_encode_param_uint32(m_data_pos, PDSIS_PARAM_Z_VALUE, event_data.data.value.z_value);
            }
            if (event_data.type == PDSIS_SENSOR_TYPE_BATTERY ||
                event_data.type == PDSIS_SENSOR_TYPE_TEMPERATURE ||
                event_data.type == PDSIS_SENSOR_TYPE_HUMIDITY )
            {
                // Service header
                m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SIS, PDSIS_SET_NOTIFY_SENSOR_INFO_RESP, 2);
                // param #1 Reult code
                m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSIS_PARAM_RESULTCODE, PDLS_RESULT_OK);
                // param #2 OriginalData
                m_data_pos += pdls_encode_param_uint16(m_data_pos, PDSIS_PARAM_X_VALUE, event_data.data.u16_originaldata[0]);
            }
          }
          else
          {
              // Service header
              m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SIS, PDSIS_SET_NOTIFY_SENSOR_INFO_RESP, 1);
              // param #1 Reult code
              m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSIS_PARAM_RESULTCODE, result);
          }
          *rsp_len   = m_data_pos - m_rsp_buf;
          result = PDLS_RESULT_OK;    // allow sending ACK
      }
      break;
      
      case PDSIS_SET_NOTIFY_SENSOR_INFO:
      {
          // Check sensor type
          result = pdls_decode_param_uint8(m_data_pos, PDSIS_PARAM_SENSORTYPE, (uint8_t *)&event_data.type);
          ERROR_CHECK(result);
          if ((m_pdlp_service.sensortypes & (0x1<<event_data.type)) == 0)
          {
            // Sensor type not supported
            return PDLS_RESULT_ERROR_NOT_SUPPORT;
          }
          m_data_pos += PARAM_UINT8_LENGTH; paramindex++;
          // Status
          event_data.event = PDSIS_EVT_SET_NOTIFY_INFO;
          result = pdls_decode_param_uint8(m_data_pos, PDSIS_PARAM_STATUS, (uint8_t *)&event_data.status);
          ERROR_CHECK(result); paramindex++;
          m_data_pos += PARAM_UINT8_LENGTH; paramindex++;
          // Threshold or original data
          switch (event_data.type) {
            {
              case PDSIS_SENSOR_TYPE_GYROSCOPE:
              case PDSIS_SENSOR_TYPE_ACCELEROMETER:
              case PDSIS_SENSOR_TYPE_ORIENTATION:
                result = pdls_decode_param_uint32(m_data_pos, PDSIS_PARAM_X_THRESHOLD, (uint32_t *)&event_data.data.threshold.x_threshold);
                ERROR_CHECK(result); 
                m_data_pos += PARAM_UINT32_LENGTH;
                result = pdls_decode_param_uint32(m_data_pos, PDSIS_PARAM_Y_THRESHOLD, (uint32_t *)&event_data.data.threshold.y_threshold);
                ERROR_CHECK(result); 
                m_data_pos += PARAM_UINT32_LENGTH;
                result = pdls_decode_param_uint32(m_data_pos, PDSIS_PARAM_Z_THRESHOLD, (uint32_t *)&event_data.data.threshold.z_threshold);
                ERROR_CHECK(result);
                break;
              
              case PDSIS_SENSOR_TYPE_BATTERY:
              case PDSIS_SENSOR_TYPE_TEMPERATURE:
              case PDSIS_SENSOR_TYPE_HUMIDITY:
                // optional data
                if (paramindex < param_num)
                {
                  // set the *optional* OriginalData if needed
                }
                break;
              
              default:
                break;
            }
          }
          // Send the request to App for handling
          event_data.event = PDSIS_EVT_SET_NOTIFY_INFO;
          result = m_pdlp_service.pdsis_event_handler(p_pdls, &event_data);
          // Prepare response, first service header
          m_data_pos = m_rsp_buf;
          m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SIS, PDSIS_SET_NOTIFY_SENSOR_INFO_RESP, 1);
          // param #1 Reult code
          m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSIS_PARAM_RESULTCODE, result);
          
          *rsp_len   = m_data_pos - m_rsp_buf;
          result = PDLS_RESULT_OK;  // allow sending ACK
      }
      break;

      default:
        return PDLS_RESULT_ERROR_NOT_SUPPORT;
    }
    
    return result;
}

/**@brief Function for handling a PDNS request.
 *
 * @param[in]  p_pdls      PDLP Service structure.
 * @param[in]  msgid       PDNS message ID.
 * @param[in]  param_num   Number of parameters in this PDNS message
 * @param[out] rsp_len     Prepared response message length.
 *
 * @retval PDLS_RESULT_OK on success, else an error value
 */
static ble_pdls_result_code_t PDNS_service_handler(ble_pdls_t * p_pdls, uint16_t msgid, uint8_t param_num, uint16_t *rsp_len)
{
    ble_pdns_event_data_t event_data;
    ble_pdls_result_code_t result;
    uint8_t  paramindex = 0;
    pdlp_opaque_t paramdata;
    
    // check msgid
    switch (msgid)
    { 
      case PDNS_CONFIRM_NOTIFY_CATEGORY:
      {
          // Prepare response, first service header
          m_data_pos = m_rsp_buf;
          m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_NS, PDNS_CONFIRM_NOTIFY_CATEGORY_RESP, 2);
          // param #1 Reult code
          m_data_pos += pdls_encode_param_uint8(m_data_pos, PDNS_PARAM_RESULTCODE, PDLS_RESULT_OK);
          // param #2 Notify category
          m_data_pos += pdls_encode_param_uint16(m_data_pos, PDNS_PARAM_NOTIFYCATEGORY, m_pdlp_service.notifycategory);

          *rsp_len = m_data_pos - m_rsp_buf;
          result = PDLS_RESULT_OK;
      }
      break;
      
      case PDNS_NOTIFY_INFORMATION:
      {
          // Check notification category
          result = pdls_decode_param_uint16(m_data_pos, PDNS_PARAM_NOTIFYCATEGORY, (uint16_t *)&event_data.data.notifyinfo.notifycategory);
          ERROR_CHECK(result);
          if ((m_pdlp_service.notifycategory & event_data.data.notifyinfo.notifycategory) == 0)
          {
            // Notify category not supported
            return PDLS_RESULT_ERROR_NOT_SUPPORT;
          }
          m_data_pos += PARAM_UINT16_LENGTH; paramindex++;
          // Unique ID
          result = pdls_decode_param_uint16(m_data_pos, PDNS_PARAM_UNIQUEID, (uint16_t *)&event_data.data.notifyinfo.uniqueid);
          ERROR_CHECK(result);
          m_data_pos += PARAM_UINT16_LENGTH; paramindex++;
          // Parameter ID list
          result = pdls_decode_param_uint16(m_data_pos, PDNS_PARAM_PARAMETERIDLIST, (uint16_t *)&event_data.data.notifyinfo.parameteridlist);
          ERROR_CHECK(result);
          m_data_pos += PARAM_UINT16_LENGTH; paramindex++;
          // Optional parameters
          while (paramindex < param_num)
          {
            switch (*m_data_pos)
            {
              case PDNS_PARAM_RUMBLINGSETTING:
                result = pdls_decode_param_uint8(m_data_pos, PDNS_PARAM_RUMBLINGSETTING, (uint8_t *)&event_data.data.notifyinfo.rumblingsetting);
                ERROR_CHECK(result);
                m_data_pos += PARAM_UINT8_LENGTH; paramindex++;
                break;
              case PDNS_PARAM_VABRATIONPATTERN:
                paramdata.p_val = event_data.data.notifyinfo.vibratiobpattern;
                result = pdls_decode_param_opaque(m_data_pos, PDNS_PARAM_RUMBLINGSETTING, &paramdata); 
                ERROR_CHECK(result);
                m_data_pos += paramdata.len; paramindex++;
                break;
              case PDNS_PARAM_LEDPATTERN:
                paramdata.p_val = event_data.data.notifyinfo.ledpattern;
                result = pdls_decode_param_opaque(m_data_pos, PDNS_PARAM_RUMBLINGSETTING, &paramdata);
                ERROR_CHECK(result);
                m_data_pos += paramdata.len; paramindex++;
                break;
              default:
                paramindex++;
                break;
            }
          }
          
          // Send the request to App for handling
          event_data.event = PDNS_EVT_NOTIFY_INFO;
          result = m_pdlp_service.pdns_event_handler(p_pdls, &event_data);
          // No need of response if App return OK
          if ( result == PDLS_RESULT_OK)
          {
            *rsp_len = 0;
          }
      }
      break;
      
      case PDNS_GET_PD_NOTIFY_DETAIL_DATA_RESP:
      {
          // Check notification category
          if (m_pdns_param_id == PDNS_PARAM_INVALID)
          {
            // Wrong status
            return PDLS_RESULT_ERROR_NO_DATA;
          }
          // Result code
          result = pdls_decode_param_uint8(m_data_pos, PDNS_PARAM_RESULTCODE, (uint8_t *)&event_data.data.notifydetail.result);
          ERROR_CHECK(result);
          m_data_pos += PARAM_UINT8_LENGTH;
          // Unique ID
          result = pdls_decode_param_uint16(m_data_pos, PDNS_PARAM_UNIQUEID, (uint16_t *)&event_data.data.notifydetail.uniqueid);
          ERROR_CHECK(result);
          m_data_pos += PARAM_UINT16_LENGTH;
          if (event_data.data.notifydetail.result == PDLS_RESULT_OK)
          {
            // Parameter data 
            //paramdata.p_val = event_data.data.notifydetail.parameterdata;
            result = pdls_decode_param_opaque(m_data_pos, m_pdns_param_id, &event_data.data.notifydetail.data);
            ERROR_CHECK(result);
          }
          // Send the response to App 
          event_data.event = PDNS_EVT_GET_PD_NOTIFY_DETAIL_DATA_RESP;
          result = m_pdlp_service.pdns_event_handler(p_pdls, &event_data);
          // No need of response if App return OK
          if ( result == PDLS_RESULT_OK)
          {
            *rsp_len = 0;
          }
          // Transaction comleted
          service_reset();
          // Parameter details fetched
          m_pdns_param_id = PDNS_PARAM_INVALID;
      }
      break;
      
      case PDNS_START_PD_APPLICATION_RESP:
      {
          // Result code
          result = pdls_decode_param_uint8(m_data_pos, PDNS_PARAM_RESULTCODE, (uint8_t *)&event_data.data.startappresult.result);
          ERROR_CHECK(result);
          m_data_pos += PARAM_UINT8_LENGTH;
          // Send the response to App 
          event_data.event = PDNS_EVT_START_PD_APP_RESP;
          result = m_pdlp_service.pdns_event_handler(p_pdls, &event_data);
          // No need of response if App return OK
          if ( result == PDLS_RESULT_OK)
          {
            *rsp_len = 0;
          }
          // Transaction comleted
          service_reset();
      }
      break;
      
      default:
        return PDLS_RESULT_ERROR_NOT_SUPPORT;
    }
    
    return result;
}

/**@brief Function for handling a PDSOS request.
 *
 * @param[in]  p_pdls      PDLP Service structure.
 * @param[in]  msgid       PDSOS message ID.
 * @param[in]  param_num   Number of parameters in this PDSOS message
 * @param[out] rsp_len     Prepared response message length.
 *
 * @retval PDLS_RESULT_OK on success, else an error value
 */
static ble_pdls_result_code_t PDSOS_service_handler(ble_pdls_t * p_pdls, uint16_t msgid, uint8_t param_num, uint16_t *rsp_len)
{
    ble_pdsos_event_data_t event_data;
    ble_pdls_result_code_t result;
    uint8_t  paramindex = 0;

    // check msgid
    switch (msgid)
    {
      case PDSOS_GET_SETTING_INFORMATION:
        {
          // Send the request to App for handling
          event_data.event = PDSOS_EVT_GET_SETTING_INFO;
          result = m_pdlp_service.pdsos_event_handler(p_pdls, &event_data);
          // No need of response if App return OK
          if ( result == PDLS_RESULT_OK)
          {
            *rsp_len = 0;
          }
        }
        break;
        
      case PDSOS_GET_SETTING_NAME:
        {
          // Setting Name Type
          result = pdls_decode_param_uint8(m_data_pos, PDSOS_PARAM_SETTINGNAMETYPE, (uint8_t *)&event_data.data.setting_name_type);
          ERROR_CHECK(result);
          m_data_pos += PARAM_UINT8_LENGTH; paramindex++;
          
          // Send the request to App for handling
          event_data.event = PDSOS_EVT_GET_SETTING_NAME;
          result = m_pdlp_service.pdsos_event_handler(p_pdls, &event_data);
          // No need of response if App return OK
          if ( result == PDLS_RESULT_OK)
          {
            *rsp_len = 0;
          }
        }
        break;
        
      case PDSOS_SELECT_SETTING_INFORMATION:
      {
          // Setting Information Request
          result = pdls_decode_param_uint8(m_data_pos, PDSOS_PARAM_SETTINGINFORMATIONREQUEST, (uint8_t *)&event_data.data.setting_info_request);
          ERROR_CHECK(result);

          if (PDSOS_SETTING_REQ_ID_SETTING == event_data.data.setting_info_request)
          {
              pdlp_opaque_t setting_info;
              result = pdls_decode_param_opaque(m_data_pos, PDSOS_PARAM_SETTINGINFORMATIONDATA, &setting_info);
              ERROR_CHECK(result);
              event_data.data.setting_info.setting_id = *(setting_info.p_val + 0);
              if (event_data.data.setting_info.setting_id == PDSOS_SETTING_VALUE_ID_LED)
              {
                  event_data.data.setting_info.setting.led_setting.color_num          = *(setting_info.p_val + 1);
                  event_data.data.setting_info.setting.led_setting.color_selected     = *(setting_info.p_val + 2);
                  event_data.data.setting_info.setting.led_setting.pattern_num        = *(setting_info.p_val + 3);
                  event_data.data.setting_info.setting.led_setting.pattern_selected   = *(setting_info.p_val + 4);
                  event_data.data.setting_info.notify_time                            = *(setting_info.p_val + 5);
              }
              else if (event_data.data.setting_info.setting_id == PDSOS_SETTING_VALUE_ID_VIBRATOR)
              {
                  event_data.data.setting_info.setting.vibrator_setting.pattern_num       = *(setting_info.p_val + 1);
                  event_data.data.setting_info.setting.vibrator_setting.pattern_selected  = *(setting_info.p_val + 2);
                  event_data.data.setting_info.notify_time                                = *(setting_info.p_val + 3);
              }
          }
          
          // Send the request to App for handling
          event_data.event = PDSOS_EVT_SELECT_SETTING_INFO;
          result = m_pdlp_service.pdsos_event_handler(p_pdls, &event_data);
          // No need of response if App return OK
          if ( result == PDLS_RESULT_OK)
          {
            *rsp_len = 0;
          }
      }
        
      case PDSOS_GET_APP_VERSION:
      case PDSOS_CONFIRM_INSTALL_APP:
      default:
        result = PDLS_RESULT_ERROR_NOT_SUPPORT;
    }
    
    return result;
}

//
// API of PDLP services
//
/**@brief Function for handling the application's BLE stack events.
 *
 * @details This function handles all events from the BLE stack that are of interest to the PDLP Service.
 *
 * @param[in] p_pdlp     PDLP Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
uint32_t ble_pdls_init(ble_pdls_t * p_pdls, const ble_pdls_init_t * p_pdls_init)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure.
    p_pdls->conn_handle       = BLE_CONN_HANDLE_INVALID;

    // Add service.
    ble_uuid128_t base_uuid = {PDLS_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_pdls->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_pdls->uuid_type;
    ble_uuid.uuid = PDLS_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_pdls->service_handle);
    VERIFY_SUCCESS(err_code);

    // Add characteristics.
    err_code = write_char_add(p_pdls, p_pdls_init);
    VERIFY_SUCCESS(err_code);

    err_code = ind_char_add(p_pdls, p_pdls_init);
    VERIFY_SUCCESS(err_code);

    m_pdlp_service = *p_pdls_init;
    return NRF_SUCCESS;
}

void ble_pdls_on_ble_evt(ble_pdls_t * p_pdls, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_pdls, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_pdls, p_ble_evt);
            break;
            
        case BLE_GATTS_EVT_WRITE:
            on_write(p_pdls, p_ble_evt);
            break;

        case BLE_GATTS_EVT_HVC:
            on_confirm(p_pdls, p_ble_evt);
            break;
        
        case BLE_GATTS_EVT_TIMEOUT:
            on_timeout(p_pdls, p_ble_evt);
        
        default:
            // No implementation needed.
            break;
    }
}

uint32_t ble_pdls_pdos_notify(ble_pdls_t * p_pdls, ble_pdos_button_id_t button_id)
{
    // check state
    if (m_transmit_state != PDLS_STATE_IDLE || !m_indication_confirmed)
    {
      return NRF_ERROR_INVALID_STATE;
    }
    
    // Prepare PDOS indication
    m_data_pos = m_rsp_buf;
    // Service header
    m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_OS, PDOS_NOTIFY_PD_OPERATION, 1);
    // param #1 Button ID
    m_data_pos += pdls_encode_param_uint8(m_data_pos, PDOS_PARAM_BUTTONID, button_id);

    // Start indicating
    m_data_size      = m_data_pos - m_rsp_buf;
    m_data_pos       = m_rsp_buf;
    m_current_packet = 0;
    return indicate_ack(p_pdls);
}

uint32_t ble_pdls_pdsis_notify(ble_pdls_t * p_pdls, ble_pdsis_sensor_type_t sensor_type, ble_pdsis_notify_value_t *p_notify_value)
{
    // check state
    if (m_transmit_state != PDLS_STATE_IDLE || !m_indication_confirmed)
    {
      return NRF_ERROR_INVALID_STATE;
    }
    
    // Prepare PDOS indication
    m_data_pos = m_rsp_buf;
    switch (sensor_type)
      {
        case PDSIS_SENSOR_TYPE_GYROSCOPE:
        case PDSIS_SENSOR_TYPE_ACCELEROMETER:
        case PDSIS_SENSOR_TYPE_ORIENTATION:
          // Service header
          m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SIS, PDSIS_NOTIFY_PD_SENSOR_INFO, 4);
          // param #1 sensor type
          m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSIS_PARAM_SENSORTYPE, (uint8_t)sensor_type);
          // param #2 X-value
          m_data_pos += pdls_encode_param_uint32(m_data_pos, PDSIS_PARAM_X_VALUE, p_notify_value->value.x_value);
          // param #3 Y-value
          m_data_pos += pdls_encode_param_uint32(m_data_pos, PDSIS_PARAM_Y_VALUE, p_notify_value->value.y_value);
          // param #4 Z-value
          m_data_pos += pdls_encode_param_uint32(m_data_pos, PDSIS_PARAM_Z_VALUE, p_notify_value->value.z_value);
          break;
        
        case PDSIS_SENSOR_TYPE_BATTERY:
        case PDSIS_SENSOR_TYPE_TEMPERATURE:
        case PDSIS_SENSOR_TYPE_HUMIDITY:
          // Service header
          m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SIS, PDSIS_NOTIFY_PD_SENSOR_INFO, 2);
          // param #1 sensor type
          m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSIS_PARAM_SENSORTYPE, (uint8_t)sensor_type);
          // param #2 OriginalData in DoCoMo foramt (12-bit)
          m_data_pos += pdls_encode_param_uint16(m_data_pos, PDSIS_PARAM_ORIGINALDATA, p_notify_value->u16_originaldata[0]);
          break;
        
        default:
          return NRF_ERROR_INVALID_DATA;
      }

    // Start indicating
    m_data_size      = m_data_pos - m_rsp_buf;
    m_data_pos       = m_rsp_buf;
    m_current_packet = 0;
    return indicate_ack(p_pdls);
}

uint32_t ble_pdls_pdns_get_pd_notify_detail_data(ble_pdls_t * p_pdls, uint16_t unique_id, uint8_t param_id, uint32_t param_len)
{
    // Prepare PDNS indication
    m_data_pos = m_rsp_buf;
    // Service header
    m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_NS, PDNS_GET_PD_NOTIFY_DETAIL_DATA, 3);
    // param #1 UniqueID
    m_data_pos += pdls_encode_param_uint16(m_data_pos, PDNS_PARAM_UNIQUEID, unique_id);
    // param #2 GetParameterID
    m_data_pos += pdls_encode_param_uint8(m_data_pos, PDNS_PARAM_GETPARAMETERID, param_id);
    m_pdns_param_id = param_id;
    // param #3 GetParameterLength
    m_data_pos += pdls_encode_param_uint32(m_data_pos, PDNS_PARAM_GETPARAMETERLENGTH, param_len);

    // Start indicating
    m_data_size      = m_data_pos - m_rsp_buf;
    m_data_pos       = m_rsp_buf;
    m_current_packet = 0;
    return indicate_ack(p_pdls);
}

uint32_t ble_pdls_pdns_start_pd_app(ble_pdls_t *p_pdls, pdlp_opaque_t *p_package, pdlp_opaque_t *p_notifyapp, 
         pdlp_opaque_t *p_class, pdlp_opaque_t *p_sharing_info)
{
    // Prepare PDNS indication
    m_data_pos = m_rsp_buf;
    // Service header
    if (p_sharing_info != NULL)
    {
      m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_NS, PDNS_GET_PD_NOTIFY_DETAIL_DATA, 4);
    }
    else
    {
      m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_NS, PDNS_GET_PD_NOTIFY_DETAIL_DATA, 3);
    }
    // param #1 package
    m_data_pos += pdls_encode_param_opaque(m_data_pos, PDNS_PARAM_PACKAGE, p_package);
    // param #2 NotifyApp
    m_data_pos += pdls_encode_param_opaque(m_data_pos, PDNS_PARAM_NOTIFYAPP, p_package);
    // param #3 Class
    m_data_pos += pdls_encode_param_opaque(m_data_pos, PDNS_PARAM_CLASS, p_class);
    if (p_sharing_info != NULL)
    {
      // param #4 SharingInformation
      m_data_pos += pdls_encode_param_opaque(m_data_pos, PDNS_PARAM_SHARINGINFORMATION, p_sharing_info);
    }

    // Start indicating
    m_data_size      = m_data_pos - m_rsp_buf;
    m_data_pos       = m_rsp_buf;
    m_current_packet = 0;
    return indicate_ack(p_pdls);
}

uint32_t ble_pdls_pdsos_get_setting_info_resp(ble_pdls_t * p_pdls, ble_pdls_result_code_t result, ble_pdsos_setting_info* p_setting_info)
{
    // Prepare response, first service header
    m_data_pos = m_rsp_buf;
    m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SOS, PDSOS_GET_SETTING_INFORMATION_RESP, 2);
    // param #1 Reult code
    m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSOS_PARAM_RESULTCODE, result);
    if (result == PDLS_RESULT_OK)
    {
        // param #2 Setting Information Data
        pdlp_opaque_t setting_info;
        if (p_setting_info->setting_id == PDSOS_SETTING_VALUE_ID_LED)
        {
            uint8_t led_setting[6];
            led_setting[0] = PDSOS_SETTING_VALUE_ID_LED;
            led_setting[1] = p_setting_info->setting.led_setting.color_num;
            led_setting[2] = p_setting_info->setting.led_setting.color_selected;
            led_setting[3] = p_setting_info->setting.led_setting.pattern_num;
            led_setting[4] = p_setting_info->setting.led_setting.pattern_selected;
            led_setting[5] = p_setting_info->notify_time;
            setting_info.len = 6;
            setting_info.p_val = led_setting;
        }
        else if (p_setting_info->setting_id == PDSOS_SETTING_VALUE_ID_VIBRATOR)
        {
            uint8_t vib_setting[4];
            vib_setting[0] = PDSOS_SETTING_VALUE_ID_VIBRATOR;
            vib_setting[1] = p_setting_info->setting.vibrator_setting.pattern_num;
            vib_setting[2] = p_setting_info->setting.vibrator_setting.pattern_selected;
            vib_setting[3] = p_setting_info->notify_time;
            setting_info.len = 4;
            setting_info.p_val = vib_setting;
        }
        m_data_pos += pdls_encode_param_opaque(m_data_pos, PDSOS_PARAM_SETTINGINFORMATIONDATA, &setting_info);
    }

    // Start indicating
    m_data_size      = m_data_pos - m_rsp_buf;
    m_data_pos       = m_rsp_buf;
    m_current_packet = 0;
    return indicate_ack(p_pdls);
}

uint32_t ble_pdls_pdsos_get_setting_name_resp(ble_pdls_t * p_pdls, ble_pdls_result_code_t result, ble_pdsos_setting_name* p_setting_name)
{
    // Prepare response, first service header
    m_data_pos = m_rsp_buf;
    m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SOS, PDSOS_GET_SETTING_NAME_RESP, 2);
    // param #1 Reult code
    m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSOS_PARAM_RESULTCODE, result);
    if (result == PDLS_RESULT_OK)
    {
        // param #2 Setting Name Data
        m_data_pos += pdls_encode_param_opaque(m_data_pos, PDSOS_PARAM_SETTINGNAMEDATA, &p_setting_name->setting);
    }

    // Start indicating
    m_data_size      = m_data_pos - m_rsp_buf;
    m_data_pos       = m_rsp_buf;
    m_current_packet = 0;
    return indicate_ack(p_pdls);
}

uint32_t ble_pdls_pdsos_select_setting_info_resp(ble_pdls_t * p_pdls, ble_pdls_result_code_t result)
{
    // Prepare response, first service header
    m_data_pos = m_rsp_buf;
    m_data_pos += pdls_encode_service_header(m_data_pos, PDLS_SERVICE_SOS, PDSOS_SELECT_SETTING_INFORMATION_RESP, 1);
    // param #1 Reult code
    m_data_pos += pdls_encode_param_uint8(m_data_pos, PDSOS_PARAM_RESULTCODE, result);

    // Start indicating
    m_data_size      = m_data_pos - m_rsp_buf;
    m_data_pos       = m_rsp_buf;
    m_current_packet = 0;
    return indicate_ack(p_pdls);
}



