/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <zephyr/types.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define DS1621_ADDRESS 0x48

LOG_MODULE_REGISTER(main);

uint8_t manuf_data[] = {0xff, 0xff, 0x00, 0x00, 0x00, 0x00};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
};

// /* Set Scan Response data */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, manuf_data, ARRAY_SIZE(manuf_data)),
};

static void bt_ready(int err) {
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return;
  }

  LOG_INF("Bluetooth initialized");

  /* Start advertising */
  err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
  if (err) {
    printk("Advertising failed to start (err %d)\n", err);
    return;
  }

  LOG_INF("Beacon started");

  const struct device *i2c_dev = device_get_binding("I2C_0");
  if (!i2c_dev) {
    LOG_ERR("I2C Device not found");
    return;
  }

  const uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;
  if (i2c_configure(i2c_dev, i2c_cfg)) {
    LOG_ERR("I2C config failed");
    return;
  }

  uint8_t data[] = {0xAC, 0x01};  // Sets to ONE-SHOT
  if (i2c_write(i2c_dev, data, sizeof(data), DS1621_ADDRESS)) {
    LOG_ERR("I2C write config failed");
    return;
  }

  // Prints the stored config
  // uint8_t d1 = 0xAC, d2 = 0;
  // if (i2c_write_read(i2c_dev, DS1621_ADDRESS, &d1, sizeof(d1), &d2, sizeof(d2))) {
  //   LOG_ERR("I2C write read failed");
  //   return;
  // }
  // LOG_INF("I2C Config: %x", d2);

  uint8_t counter = 0;
  while (true) {
    uint8_t data1 = 0xEE;
    if (i2c_write(i2c_dev, &data1, sizeof(data1), DS1621_ADDRESS)) {
      LOG_ERR("I2C write failed");
      return;
    }
    k_sleep(K_MSEC(2000));

    data1 = 0xAA;
    uint8_t reading[2];
    if (i2c_write_read(i2c_dev, DS1621_ADDRESS, &data1, sizeof(data1), reading, sizeof(reading))) {
      LOG_ERR("I2C write read failed");
      return;
    }

    uint16_t temperature = reading[0] * 10 + (reading[1] ? 5 : 0);
    LOG_INF("Temperature: %d, reading: (%x,%x)", temperature, reading[0], reading[1]);

    memcpy(&manuf_data[2], &temperature, sizeof(temperature));
    manuf_data[5] = counter++;
    err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
      LOG_ERR("Advertising failed to update (err %d)", err);
      return;
    }
  }
}

void main(void) {
  int err;

  LOG_INF("Starting Beacon Demo");

  /* Initialize the Bluetooth Subsystem */
  err = bt_enable(bt_ready);
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return;
  }
}
