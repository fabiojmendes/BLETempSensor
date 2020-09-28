/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/adc.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <power/power.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <zephyr/types.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))

#define ADC_LABEL DT_LABEL(DT_NODELABEL(adc))

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

void sys_pm_notify_power_state_entry(enum power_states state) {  //
  LOG_INF("Power state entered: %d", state);
}

void main(void) {
  int err;

#ifdef CONFIG_BOARD_NRF52840DONGLE_NRF52840
  const struct device *cons = device_get_binding(CONSOLE_LABEL);
  device_set_power_state(cons, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
#endif

  LOG_INF("Starting Beacon Demo");

  /* Initialize the Bluetooth Subsystem */
  err = bt_enable(NULL);
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return;
  }

  LOG_INF("Bluetooth initialized");

  /* Start advertising */
  err = bt_le_adv_start(BT_LE_ADV_PARAM(0, BT_GAP_ADV_SLOW_INT_MIN,      //
                                        BT_GAP_ADV_SLOW_INT_MAX, NULL),  //
                        ad, ARRAY_SIZE(ad),                              //
                        sd, ARRAY_SIZE(sd));                             //
  if (err) {
    LOG_ERR("Advertising failed to start (err %d)\n", err);
    return;
  }

  LOG_INF("Beacon started");

  const char *const devname = DT_LABEL(DT_INST(0, microchip_mcp9808));
  const struct device *dev = device_get_binding(devname);

  if (dev == NULL) {
    LOG_ERR("Device %s not found.\n", devname);
    return;
  }

  const struct device *adc = device_get_binding(ADC_LABEL);
  if (adc == NULL) {
    LOG_ERR("Device %s not found.\n", ADC_LABEL);
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
    struct sensor_value temp;

    int rc = sensor_sample_fetch(dev);
    if (rc != 0) {
      LOG_ERR("sensor_sample_fetch error: %d", rc);
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    if (rc != 0) {
      LOG_ERR("sensor_channel_get error: %d", rc);
    }

    int16_t temperaure = sensor_value_to_double(&temp) * 1000.0;
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
