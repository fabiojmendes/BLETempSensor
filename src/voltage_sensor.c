#include <drivers/adc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(volt_sensor);

#define ADC_LABEL DT_LABEL(DT_NODELABEL(adc))

static const struct device *adc;

static bool calibrate = true;

int volt_sensor_init() {
  adc = device_get_binding(ADC_LABEL);
  if (adc == NULL) {
    LOG_ERR("Device %s not found.", ADC_LABEL);
    return -1;
  }

  struct adc_channel_cfg adc_config = {
      .gain = ADC_GAIN_1_6,
      .reference = ADC_REF_INTERNAL,
      .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
      .input_positive = SAADC_CH_PSELP_PSELP_VDD,
  };

  int err = adc_channel_setup(adc, &adc_config);
  if (err) {
    LOG_ERR("Error setting up ADC (%d)", err);
    return err;
  }

  return 0;
}

int volt_sensor_read(int16_t *value) {
  uint16_t adc_buffer;

  struct adc_sequence seq = {
      .channels = BIT(0),
      .buffer = &adc_buffer,
      .buffer_size = sizeof(adc_buffer),
      .calibrate = calibrate,
      .oversampling = 4,
      .resolution = 14,
  };
  int err = adc_read(adc, &seq);
  if (err) {
    LOG_ERR("Error reading ADC (%d)", err);
    return err;
  }
  calibrate = false;

  int32_t voltage = adc_buffer;
  adc_raw_to_millivolts(adc_ref_internal(adc), ADC_GAIN_1_6, 14, &voltage);

  *value = voltage;

  LOG_INF("Voltage %d (raw: %d, reference: %d)", *value, adc_buffer, adc_ref_internal(adc));
  return 0;
}
