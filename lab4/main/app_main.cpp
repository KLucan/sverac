#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "BLE-Server";

// MACRO DEFINICIJE

#define JMBAG "0036530273"
#define JMBAG_LAST4 0x0273

#define UUID_SERVICE JMBAG_LAST4
#define UUID_CHAR1 (JMBAG_LAST4 + 1) // counterBLE
#define UUID_CHAR2 (JMBAG_LAST4 + 2) // readBLE
#define UUID_CHAR3 (JMBAG_LAST4 + 3) // writeBLE

// VARIJABLE APLIKACIJE
static uint8_t ble_addr_type;
static uint16_t conn_handle = 0;
static bool connected = false;
static uint16_t counterBLE = 0;
static uint16_t writeBLE = 1;
static const char *readBLE = JMBAG;
static uint16_t s_chr1_val_handle;

void ble_app_advertise(void);

// CALLBACK FUNKCIJE

static int device_read_chr1(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
  return os_mbuf_append(ctxt->om, &counterBLE, sizeof(counterBLE));
}

static int device_read_chr2(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
  return os_mbuf_append(ctxt->om, readBLE, strlen(readBLE));
}

static int device_write_chr3(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg) {
  uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
  if (len >= 1) {
    uint8_t value = ctxt->om->om_data[0];
    if (value >= 1 && value <= 10) {
      writeBLE = value;
      ESP_LOGI(TAG, "writeBLE = %d", writeBLE);
    } else {
      ESP_LOGW(TAG, "writeBLE %d izvan [1,10]", value);
    }
  }
  return 0;
}

void ble_app_on_sync(void) {
  ble_hs_id_infer_auto(0, &ble_addr_type);
  ble_app_advertise();
}

// UUID

static const ble_uuid16_t g_uuid_service = BLE_UUID16_INIT(UUID_SERVICE);
static const ble_uuid16_t g_uuid_chr1 = BLE_UUID16_INIT(UUID_CHAR1);
static const ble_uuid16_t g_uuid_chr2 = BLE_UUID16_INIT(UUID_CHAR2);
static const ble_uuid16_t g_uuid_chr3 = BLE_UUID16_INIT(UUID_CHAR3);

// DEFINICIJA SERVISA

static const struct ble_gatt_chr_def g_chrs[] = {
    {
        // 1: counterBLE
        .uuid = &g_uuid_chr1.u,
        .access_cb = device_read_chr1,
        .arg = NULL,
        .descriptors = NULL,
        .flags =
            (ble_gatt_chr_flags)(BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY),
        .min_key_size = 0,
        .val_handle = &s_chr1_val_handle,
        .cpfd = NULL,
    },
    {
        // 2: readBLE
        .uuid = &g_uuid_chr2.u,
        .access_cb = device_read_chr2,
        .arg = NULL,
        .descriptors = NULL,
        .flags = BLE_GATT_CHR_F_READ,
        .min_key_size = 0,
        .val_handle = NULL,
        .cpfd = NULL,
    },
    {
        // 3: writeBLE
        .uuid = &g_uuid_chr3.u,
        .access_cb = device_write_chr3,
        .arg = NULL,
        .descriptors = NULL,
        .flags = BLE_GATT_CHR_F_WRITE,
        .min_key_size = 0,
        .val_handle = NULL,
        .cpfd = NULL,
    },
    {0}};

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &g_uuid_service.u,
        .includes = NULL,
        .characteristics = g_chrs,
    },
    {0}};

// EVENT HANDLER

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
  switch (event->type) {

  case BLE_GAP_EVENT_CONNECT:
    if (event->connect.status == 0) {
      conn_handle = event->connect.conn_handle;
      connected = true;
      ESP_LOGI(TAG, "Connected (conn_handle = %d)", conn_handle);
    } else {
      ESP_LOGW(TAG, "Connection failed (status = %d)", event->connect.status);
      conn_handle = 0;
      connected = false;
      ble_app_advertise();
    }
    break;

  case BLE_GAP_EVENT_DISCONNECT:
    ESP_LOGI(TAG, "Disconnect (reason = %d)", event->disconnect.reason);
    conn_handle = 0;
    connected = false;
    ble_app_advertise();
    break;

  case BLE_GAP_EVENT_ADV_COMPLETE:
    ble_app_advertise();
    break;

  default:
    break;
  }
  return 0;
}

// ADVERTISING

void ble_app_advertise(void) {
  struct ble_hs_adv_fields fields;
  memset(&fields, 0, sizeof(fields));
  fields.name = (uint8_t *)ble_svc_gap_device_name();
  fields.name_len = strlen(ble_svc_gap_device_name());
  fields.name_is_complete = 1;

  ble_gap_adv_set_fields(&fields);

  struct ble_gap_adv_params adv_params;
  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                    ble_gap_event, NULL);
}

// TASKS

void host_task(void *param) { nimble_port_run(); }

void counter_task(void *param) {
  TickType_t lastWake = xTaskGetTickCount();

  while (1) {
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1000));

    counterBLE += writeBLE;
    ESP_LOGI(TAG, "counterBLE = %d (writeBLE = %d)", counterBLE, writeBLE);

    if (connected && s_chr1_val_handle != 0) {
      ble_gatts_chr_updated(s_chr1_val_handle);
    }
  }
}

// MAIN

extern "C" void app_main(void) {

  nvs_flash_init();
  nimble_port_init();
  ble_svc_gap_init();
  ble_svc_gap_device_name_set("KarloLucan");
  ble_svc_gatt_init();
  ble_gatts_count_cfg(gatt_svcs);
  ble_gatts_add_svcs(gatt_svcs);
  ble_hs_cfg.sync_cb = ble_app_on_sync;

  xTaskCreate(counter_task, "counter", 4096, NULL, 1, NULL);
  nimble_port_freertos_init(host_task);
}
