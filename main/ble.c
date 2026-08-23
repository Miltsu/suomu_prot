//
// Created by Elden Lord on 22.8.2026.
//

#include "ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_nimble_hci.h"
#include <string.h>

// THESE UUID'S NOT RIGHT ONES GOTTA GET EM FROM THE MIT APP!!!
static const ble_uuid128_t service_uuid =
    BLE_UUID128_INIT(0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00);
static const ble_uuid128_t distance_chr_uuid =
    BLE_UUID128_INIT(0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x01,0x00,0x00);
static const ble_uuid128_t temp_chr_uuid =
    BLE_UUID128_INIT(0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x02,0x00,0x00);

static float current_distance = 0.0f;
static float current_temp = 0.0f;

static int distance_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, &current_distance, sizeof(current_distance));
    }
    return 0;
}

static int temp_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, &current_temp, sizeof(current_temp));
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
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = &temp_chr_uuid.u,
                .access_cb = temp_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            { 0 }
        },
    },
    { 0 }
};

static void ble_advertise(void) {
    struct ble_gap_adv_params adv_params = { 0 };
    struct ble_hs_adv_fields fields = { 0 };

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)ble_svc_gap_device_name();
    fields.name_len = strlen(ble_svc_gap_device_name());
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
}

static void on_sync(void) {
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

    ble_svc_gap_device_name_set(device_name);
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = on_sync;

    nimble_port_freertos_init(nimble_host_task);
}

void ble_update_distance(float distance_cm) { current_distance = distance_cm; }
void ble_update_temperature(float temp_c) { current_temp = temp_c; }