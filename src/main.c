#include <math.h>
#include <stdio.h>
#include "wokwi-api.h"

typedef struct {
  pin_t out;
  uint32_t waveform, freq_ms, amp_mv;
  float t;
} chip_state_t;

static void tick(void *userdata) {
  chip_state_t *chip = userdata;
  float freq = (float)chip->freq_ms;
  float amp = chip->amp_mv / 1000.0f;
  float period = 1.0f / freq;
  float phase = fmodf(chip->t, period) / period;
  float value = 0;

  switch (chip->waveform) {
    case 0: value = sinf(2 * M_PI * phase); break;
    case 1: value = 2.0f * fabsf(2.0f * phase - 1.0f) - 1.0f; break;
    case 2: value = phase < 0.5f ? 1.0f : -1.0f; break;
  }

  float v = (value * amp + amp) * 0.5f;
  pin_dac_write(chip->out, v);
  chip->t += 1.0f / 10000.0f;
}

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->out = pin_init("OUT", ANALOG);
  chip->waveform = attr_init("waveform", 0);
  chip->freq_ms  = attr_init("frequency", 100);
  chip->amp_mv   = attr_init("amplitude", 1000);
  chip->t = 0.0f;
  add_clock_event(100, tick, chip);
  printf("Function Generator iniciado. wf=%u freq=%u Hz amp=%.2f V\n",
         chip->waveform, chip->freq_ms, chip->amp_mv / 1000.0f);
}