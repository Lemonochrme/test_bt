#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#define PROFILE_NUM 1
#define PROFILE_APP_ID 0
#define SERVICE_UUID  0x4242 
#define CHAR_UUID     0x6969 

static const char *GATTS_TAG = "BLE_DEMO";

static uint8_t hello_world_value[] = "Hello World";

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min       = 0x20,
    .adv_int_max       = 0x40,
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
};

static struct gatts_profile_inst gl_profile = {
    .gatts_cb = NULL,
    .gatts_if = ESP_GATT_IF_NONE,
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TAG, "Advertising failed");
            } else {
                ESP_LOGI(GATTS_TAG, "Advertising started successfully");
            }
            break;
        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            gl_profile.service_id.is_primary = true;
            gl_profile.service_id.id.inst_id = 0x00;
            gl_profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
            gl_profile.service_id.id.uuid.uuid.uuid16 = SERVICE_UUID;

            esp_ble_gap_config_adv_data(&adv_data);
            esp_ble_gatts_create_service(gatts_if, &gl_profile.service_id, 4);
            break;

        case ESP_GATTS_CREATE_EVT:
            gl_profile.service_handle = param->create.service_handle;
            gl_profile.char_uuid.len = ESP_UUID_LEN_16;
            gl_profile.char_uuid.uuid.uuid16 = CHAR_UUID;

            esp_ble_gatts_start_service(gl_profile.service_handle);
            esp_ble_gatts_add_char(gl_profile.service_handle, &gl_profile.char_uuid,
                                   ESP_GATT_PERM_READ, ESP_GATT_CHAR_PROP_BIT_READ,
                                   &(esp_attr_value_t) {
                                       .attr_max_len = sizeof(hello_world_value),
                                       .attr_len = sizeof(hello_world_value),
                                       .attr_value = hello_world_value
                                   }, NULL);
            break;

        case ESP_GATTS_ADD_CHAR_EVT:
            gl_profile.char_handle = param->add_char.attr_handle;
            ESP_LOGI(GATTS_TAG, "Characteristic added");
            break;

        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TAG, "Read request received");
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = sizeof(hello_world_value);
            memcpy(rsp.attr_value.value, hello_world_value, sizeof(hello_world_value));
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "Device connected");
            gl_profile.conn_id = param->connect.conn_id;
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "Device disconnected");
            esp_ble_gap_start_advertising(&adv_params);
            break;

        default:
            break;
    }
}

void app_main(void) {
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(PROFILE_APP_ID));

    ESP_LOGI(GATTS_TAG, "Bluetooth device is ready");
}
