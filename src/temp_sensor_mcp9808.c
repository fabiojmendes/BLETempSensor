#include <drivers/sensor.h>
#include <logging/log.h>

#include "temp_sensor.h"

#define MCP9808 DT_LABEL(DT_INST(0, microchip_mcp9808))

static struct device *dev;

int temp_sensor_init() {
  dev = device_get_binding(MCP9808);

  if (dev == NULL) {
    LOG_ERR("Device %s not found.\n", MCP9808);
    return -1;
  }
  return 0;
}

void temp_sensor_fetch() {
}

int temp_sensor_read(int16_t *value) {
  int rc = sensor_sample_fetch(dev);
  if (rc != 0) {
    LOG_ERR("sensor_sample_fetch error: %d", rc);
    return rc;
  }

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
