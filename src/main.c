#include <math.h>
#include "wokwi-api.h"

#define PI 3.141592653589793
#define NUM_SINE 3

typedef struct {
  pin_t vout;
  timer_t tid;
  float freqs[NUM_SINE];
  float amps[NUM_SINE];
} opamp_chip_t;

void chip_tick(void *user_data) {
  opamp_chip_t *chip = (opamp_chip_t *)user_data;
  double t = get_sim_nanos() * 1e-9;
  double y = 0.0;

  for (int i = 0; i < NUM_SINE; i++) {
    if (chip->freqs[i] > 0.0f && chip->amps[i] != 0.0f) {
      y += chip->amps[i] * sin(2.0 * PI * chip->freqs[i] * t);
    }
  }

  pin_dac_write(chip->vout, (float)y);
}

void chip_init(void) {
  opamp_chip_t *chip = malloc(sizeof(opamp_chip_t));

  chip->freqs[0] = attr_read_float(attr_init_float("freq1", 10.0f));
  chip->freqs[1] = attr_read_float(attr_init_float("freq2", 110.0f));
  chip->freqs[2] = attr_read_float(attr_init_float("freq3", 0.0f));

  chip->amps[0] = attr_read_float(attr_init_float("amp1", 1.0f));
  chip->amps[1] = attr_read_float(attr_init_float("amp2", 1.0f));
  chip->amps[2] = attr_read_float(attr_init_float("amp3", 1.0f));

  chip->vout = pin_init("OUT", ANALOG);

  const timer_config_t tcfg = {
    .callback = chip_tick,
    .user_data = chip,
  };

  chip->tid = timer_init(&tcfg);
  timer_start(chip->tid, 1000, true);  // 1µs = 1 MHz update rate
}