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

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define ADC_LABEL DT_LABEL(DT_NODELABEL(adc))

#define ADV_INTERVAL_MIN 5000 * 0.625
#define ADV_INTERVAL_MAX 5200 * 0.625

LOG_MODULE_REGISTER(main);

struct manuf_data_t {
  uint16_t id;
  int16_t temperature;
  int16_t voltage;
  uint8_t reserved;
  uint8_t counter;
};

static struct manuf_data_t manuf_data = {.id = 0xFFFF};

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

  const struct device *adc = device_get_binding(ADC_LABEL);
  if (adc == NULL) {
    LOG_ERR("Device %s not found.", ADC_LABEL);
    return;
  }

  struct adc_channel_cfg adc_config = {
      .gain = ADC_GAIN_1_6,
      .reference = ADC_REF_INTERNAL,
      .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
      .input_positive = SAADC_CH_PSELP_PSELP_VDD,
  };

  err = adc_channel_setup(adc, &adc_config);
  if (err) {
    LOG_ERR("Error setting up ADC (%d)", err);
    return;
  }

  uint8_t counter = 0;
  while (true) {
    int16_t temperaure = 0;
    err = temp_sensor_read(&temperaure);
    if (err) {
      LOG_WRN("Error reading temperature");
    }
    LOG_INF("Temperature: %d", temperaure);

    uint16_t adc_buffer;

    struct adc_sequence seq = {
        .channels = BIT(0),
        .buffer = &adc_buffer,
        .buffer_size = sizeof(adc_buffer),
        .calibrate = true,
        .oversampling = 4,
        .resolution = 14,
    };
    adc_read(adc, &seq);

    int voltage = adc_buffer;
    adc_raw_to_millivolts(adc_ref_internal(adc), ADC_GAIN_1_6, 14, &voltage);

    LOG_INF("Voltage %d (raw: %d, reference: %d)", voltage, adc_buffer, adc_ref_internal(adc));

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
