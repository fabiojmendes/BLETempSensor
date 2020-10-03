#include <drivers/i2c.h>
#include <logging/log.h>

#include "temp_sensor.h"

#define DS1621_ADDRESS 0x48

#define I2C_LABEL DT_LABEL(DT_NODELABEL(i2c0))

LOG_MODULE_REGISTER(temp_sensor_init);

const struct device *i2c_dev;

int temp_sensor_init() {
  i2c_dev = device_get_binding(I2C_LABEL);
  if (!i2c_dev) {
    LOG_ERR("I2C Device not found");
    return -1;
  }

  const uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;
  if (i2c_configure(i2c_dev, i2c_cfg)) {
    LOG_ERR("I2C config failed");
    return -1;
  }

  uint8_t data[] = {0xAC, 0x01};  // Sets to ONE-SHOT
  if (i2c_write(i2c_dev, data, sizeof(data), DS1621_ADDRESS)) {
    LOG_ERR("I2C write config failed");
    return -1;
  }

  // Prints the stored config
  // uint8_t d1 = 0xAC, d2 = 0;
  // if (i2c_write_read(i2c_dev, DS1621_ADDRESS, &d1, sizeof(d1), &d2, sizeof(d2))) {
  //   LOG_ERR("I2C write read failed");
  //   return;
  // }
  // LOG_INF("I2C Config: %x", d2);
  return 0;
}

int temp_sensor_read(int16_t *value) {
  uint8_t data1 = 0xEE;
  if (i2c_write(i2c_dev, &data1, sizeof(data1), DS1621_ADDRESS)) {
    LOG_ERR("I2C write failed");
    return -1;
  }
  k_sleep(K_MSEC(2000));

  data1 = 0xAA;
  uint8_t reading[2];
  if (i2c_write_read(i2c_dev, DS1621_ADDRESS, &data1, sizeof(data1), reading, sizeof(reading))) {
    LOG_ERR("I2C write read failed");
    return -1;
  }

  *value = reading[0] * 1000 + (reading[1] ? 500 : 0);
  LOG_INF("Temperature: %d, reading: (%x,%x)", *value, reading[0], reading[1]);

  return 0;
}
