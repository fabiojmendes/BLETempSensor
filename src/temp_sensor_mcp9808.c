#include <drivers/sensor.h>
#include <logging/log.h>

#include "temp_sensor.h"

#define MCP9808_REG_CONFIG 0x01

#define MCP9808 DT_LABEL(DT_INST(0, microchip_mcp9808))

LOG_MODULE_REGISTER(temp_sensor_init);

static const struct device *dev;

// static struct sensor_value shutdown = {.val1 = 0x00, .val2 = 0x00};

// static struct sensor_value wakeup = {.val1 = 0x00, .val2 = 0x00};

int temp_sensor_init() {
  dev = device_get_binding(MCP9808);

  if (dev == NULL) {
    LOG_ERR("Device %s not found.\n", MCP9808);
    return -1;
  }

  // int rc = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP, MCP9808_REG_CONFIG, &shutdown);
  // if (rc != 0) {
  //   LOG_ERR("Sensor shutdown error: %d", rc);
  //   return rc;
  // }
  return 0;
}

int temp_sensor_read(int16_t *value) {
  int rc;
  // rc = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP, MCP9808_REG_CONFIG, &wakeup);
  // if (rc != 0) {
  //   LOG_ERR("Sensor wakeup error: %d", rc);
  //   return rc;
  // }
  // k_sleep(K_MSEC(250));

  rc = sensor_sample_fetch(dev);
  if (rc != 0) {
    LOG_ERR("sensor_sample_fetch error: %d", rc);
    return rc;
  }
  // rc = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP, MCP9808_REG_CONFIG, &shutdown);
  // if (rc != 0) {
  //   LOG_ERR("Sensor shutdown error: %d", rc);
  //   return rc;
  // }

  struct sensor_value temp;
  rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
  if (rc != 0) {
    LOG_ERR("sensor_channel_get error: %d", rc);
    return rc;
  }

  *value = sensor_value_to_double(&temp) * 1000.0;
  LOG_INF("Temperature: %d", *value);
  return 0;
}
