#include "wokwi-api.h"
#include <stdlib.h>

#define PI 3.141592653589793
#define NUM_SINE 3

// Aproximação da função seno por série de Taylor
float sine_taylor(float x) {
  // Normaliza x para o intervalo [-PI, PI]
  while (x > PI) x -= 2.0f * PI;
  while (x < -PI) x += 2.0f * PI;

  float x2 = x * x;
  float term = x;       // Primeiro termo da série
  float sum = term;

  term *= -x2 / (2.0f * 3.0f);
  sum += term;

  term *= -x2 / (4.0f * 5.0f);
  sum += term;

  term *= -x2 / (6.0f * 7.0f);
  sum += term;

  term *= -x2 / (8.0f * 9.0f);
  sum += term;

  return sum;
}

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
      float angle = 2.0f * PI * chip->freqs[i] * t;
      y += chip->amps[i] * sine_taylor(angle);
    }
  }

  pin_dac_write(chip->vout, (float)y);
}

void chip_init(void) {
  opamp_chip_t *chip = malloc(sizeof(opamp_chip_t));

  chip->freqs[0] = attr_read_float(attr_init_float("freq1", 100.0f)) / 10.0f;
  chip->freqs[1] = attr_read_float(attr_init_float("freq2", 1100.0f)) / 10.0f;
  chip->freqs[2] = attr_read_float(attr_init_float("freq3", 0.0f)) / 10.0f;

  chip->amps[0] = attr_read_float(attr_init_float("amp1", 10.0f)) / 10.0f;
  chip->amps[1] = attr_read_float(attr_init_float("amp2", 10.0f)) / 10.0f;
  chip->amps[2] = attr_read_float(attr_init_float("amp3", 10.0f)) / 10.0f;

  chip->vout = pin_init("OUT", ANALOG);

  const timer_config_t tcfg = {
    .callback = chip_tick,
    .user_data = chip,
  };

  chip->tid = timer_init(&tcfg);
  timer_start(chip->tid, 1000, true);  // 1µs = 1 MHz update rate
}