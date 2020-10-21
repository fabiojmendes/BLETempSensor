/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/adc.h>
#include <logging/log.h>
#include <stddef.h>
#include <sys/util.h>
#include <zephyr/types.h>

#include "temp_sensor.h"
#include "voltage_sensor.h"

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define ADC_LABEL DT_LABEL(DT_NODELABEL(adc))

#define ADV_INTERVAL_MIN 5000 * 0.625
#define ADV_INTERVAL_MAX 5200 * 0.625

#define NO_READING SHRT_MIN

LOG_MODULE_REGISTER(main);

struct manuf_data_t {
  uint16_t id;
  int16_t temperature;
  int16_t voltage;
  uint8_t reserved;
  uint8_t counter;
};

static struct manuf_data_t manuf_data = {.id = 0xFFFF, .reserved = 0x01};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
};

// /* Set Scan Response data */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, &manuf_data, sizeof(manuf_data)),
};

void main(void) {
  int err;

  LOG_INF("Starting Beacon Demo");

  /* Initialize the Bluetooth Subsystem */
  err = bt_enable(NULL);
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return;
  }

  LOG_INF("Bluetooth initialized");

  /* Start advertising */
  err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,  //
                                        ADV_INTERVAL_MIN,            //
                                        ADV_INTERVAL_MAX, NULL),     //
                        ad, ARRAY_SIZE(ad),                          //
                        sd, ARRAY_SIZE(sd));                         //
  if (err) {
    LOG_ERR("Advertising failed to start (err %d)\n", err);
    return;
  }

  LOG_INF("Beacon started");

  err = temp_sensor_init();
  if (err) {
    LOG_ERR("Error initializing temp sensor.");
  }

  err = volt_sensor_init();
  if (err) {
    LOG_ERR("Error initializing voltage sensor.");
  }

  uint8_t counter = 0;

  while (true) {
    int16_t temperaure = NO_READING;
    err = temp_sensor_read(&temperaure);
    if (err) {
      LOG_WRN("Error reading temperature");
    }

    uint16_t voltage = NO_READING;
    err = volt_sensor_read(&voltage);
    if (err) {
      LOG_WRN("Error reading voltage");
    }

    manuf_data.temperature = temperaure;
    manuf_data.voltage = voltage;
    manuf_data.counter = counter++;
    err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
      LOG_ERR("Advertising failed to update (err %d)", err);
    }

    k_sleep(K_SECONDS(10));
  }
}
