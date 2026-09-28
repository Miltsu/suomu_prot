#include "ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_nimble_hci.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"

// Service UUID: e77e9316-1158-4405-8bf0-5f74ff01f0ac
static const ble_uuid128_t service_uuid =
    BLE_UUID128_INIT(0xac,0xf0,0x01,0xff,0x74,0x5f,0xf0,0x8b,0x05,0x44,0x58,0x11,0x16,0x93,0x7e,0xe7);

// Distance characteristic UUID: 39e83723-096a-4b54-9b2d-38b1ac05d980
static const ble_uuid128_t distance_chr_uuid =
    BLE_UUID128_INIT(0x80,0xd9,0x05,0xac,0xb1,0x38,0x2d,0x9b,0x54,0x4b,0x6a,0x09,0x23,0x37,0xe8,0x39);

// Temperature characteristic UUID: 47cbe295-7e40-4c2c-ac4b-42d927845593
static const ble_uuid128_t temp_chr_uuid =
    BLE_UUID128_INIT(0x93,0x55,0x84,0x27,0xd9,0x42,0x4b,0xac,0x2c,0x4c,0x40,0x7e,0x95,0xe2,0xcb,0x47);

// Forward declaration — ble_advertise() uses this before it's defined below
static int ble_gap_event(struct ble_gap_event *event, void *arg);

// Values stored as text strings, e.g. "23.5", so MIT App Inventor can display them directly
static char current_distance_str[16] = "0";
static char current_temp_str[16] = "0";

// Handles needed so we know which "mailbox" to notify, and who is connected
static uint16_t distance_val_handle;
static uint16_t temp_val_handle;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;

static int distance_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, current_distance_str, strlen(current_distance_str));
    }
    return 0;
}

static int temp_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, current_temp_str, strlen(current_temp_str));
    }
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &distance_chr_uuid.u,
                .access_cb = distance_access_cb,
                .val_handle = &distance_val_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                .uuid = &temp_chr_uuid.u,
                .access_cb = temp_access_cb,
                .val_handle = &temp_val_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 }
        },
    },
    { 0 }
};

static void ble_advertise(void) {
    uint8_t own_addr_type;
    ble_hs_id_infer_auto(0, &own_addr_type);

    struct ble_gap_adv_params adv_params = { 0 };
    struct ble_hs_adv_fields fields = { 0 };

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)ble_svc_gap_device_name();
    fields.name_len = strlen(ble_svc_gap_device_name());
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

// Fires when a phone connects/disconnects — keeps track of who to notify
static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                conn_handle = event->connect.conn_handle;
            } else {
                ble_advertise(); // connection attempt failed, restart advertising
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ble_advertise(); // restart advertising so phone can reconnect
            return 0;

        default:
            return 0;
    }
}

static void on_sync(void) {
    uint8_t addr_type;
    uint8_t addr_val[6] = {0};

    ble_hs_id_infer_auto(0, &addr_type);
    ble_hs_id_copy_addr(addr_type, addr_val, NULL);

    ESP_LOGI("BLE", "Device Address: %02x:%02x:%02x:%02x:%02x:%02x",
             addr_val[5], addr_val[4], addr_val[3],
             addr_val[2], addr_val[1], addr_val[0]);

    ble_advertise();
}

static void nimble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_init(const char *device_name)
{
    esp_nimble_hci_init();
    nimble_port_init();

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_svc_gap_device_name_set(device_name);

    ble_hs_cfg.sync_cb = on_sync;

    nimble_port_freertos_init(nimble_host_task);
}

// Converts the float to text (e.g. 23.5 -> "23.5") and notifies the connected phone
void ble_update_distance(float distance_cm) {
    snprintf(current_distance_str, sizeof(current_distance_str), "%.1f", distance_cm);
    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(current_distance_str, strlen(current_distance_str));
        ble_gattc_notify_custom(conn_handle, distance_val_handle, om);
    }
}

void ble_update_temperature(float temp_c) {
    snprintf(current_temp_str, sizeof(current_temp_str), "%.1f", temp_c);
    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(current_temp_str, strlen(current_temp_str));
        ble_gattc_notify_custom(conn_handle, temp_val_handle, om);
    }
}
