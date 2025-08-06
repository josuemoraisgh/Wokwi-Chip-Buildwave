#include "wokwi-api.h"
#include <stdlib.h>

#define PI 3.141592653589793
#define NUM_SINE 3

// Função seno aproximada usando série de Taylor simplificada
float sine_taylor(float x) {
  while (x > PI) x -= 2.0f * PI;
  while (x < -PI) x += 2.0f * PI;

  float x2 = x * x;
  float term = x;
  float sum = term;

  term *= -x2 / 6.0f;  // 3! = 6
  sum += term;

  term *= -x2 / 20.0f; // 5! / 3! = 20
  sum += term;

  term *= -x2 / 42.0f; // 7! / 5! = 42
  sum += term;

  term *= -x2 / 72.0f; // 9! / 7! = 72
  sum += term;

  return sum;
}

typedef struct {
  pin_t vout;
  timer_t timer;
  float freqs[NUM_SINE];
  float amps[NUM_SINE];
  uint32_t time_us;
} chip_state_t;

void chip_timer_event(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;

  chip->time_us += 1; // incrementa a cada 1us (frequência de 1 MHz)
  float t = chip->time_us * 1e-6;
  float y = 0.0f;

  for (int i = 0; i < NUM_SINE; i++) {
    if (chip->freqs[i] > 0.0f && chip->amps[i] != 0.0f) {
      float angle = 2.0f * PI * chip->freqs[i] * t;
      y += chip->amps[i] * sine_taylor(angle);
    }
  }

  pin_dac_write(chip->vout, y);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->freqs[0] = attr_read_float(attr_init_float("freq1", 100.0f)) / 10.0f;
  chip->freqs[1] = attr_read_float(attr_init_float("freq2", 1100.0f)) / 10.0f;
  chip->freqs[2] = attr_read_float(attr_init_float("freq3", 0.0f)) / 10.0f;

  chip->amps[0] = attr_read_float(attr_init_float("amp1", 10.0f)) / 10.0f;
  chip->amps[1] = attr_read_float(attr_init_float("amp2", 10.0f)) / 10.0f;
  chip->amps[2] = attr_read_float(attr_init_float("amp3", 10.0f)) / 10.0f;

  chip->vout = pin_init("OUT", ANALOG);

  chip->time_us = 0;

  const timer_config_t timer_config = {
    .callback = chip_timer_event,
    .user_data = chip,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, 1, true);  // timer a cada 1µs
}