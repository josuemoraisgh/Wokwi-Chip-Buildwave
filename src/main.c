#include <math.h>
#include <stdint.h>
#include "wokwi-api.h"

typedef struct {
  pin_t out;
  uint32_t waveform, freq, amp;
  float t;
} chip_state_t;

static chip_state_t chip;

#define SAMPLE_RATE 10000.0f // 10 kHz

void chip_init(void) {
  chip.out = pin_init("OUT", 1);
  chip.waveform = attr_init("waveform", 0);   // 0: seno, 1: tri, 2: quad
  chip.freq = attr_init("frequency", 100);    // Hz
  chip.amp = attr_init("amplitude", 1000);    // mV
  chip.t = 0.0f;
}

// Esta função será chamada automaticamente pelo simulador Wokwi
void chip_tick(void) {
  float freq = (float)chip.freq;
  float amp = chip.amp / 1000.0f; // em Volts
  float period = 1.0f / freq;
  float phase = fmodf(chip.t, period) / period;
  float value = 0;

  switch (chip.waveform) {
    case 0: value = sinf(2 * M_PI * phase); break;
    case 1: value = 2.0f * fabsf(2.0f * phase - 1.0f) - 1.0f; break;
    case 2: value = phase < 0.5f ? 1.0f : -1.0f; break;
  }

  float v = (value * amp + amp) * 0.5f; // entre 0 e amp
  pin_dac_write(chip.out, v); // Saída analógica (0–5 V)
  chip.t += 1.0f / SAMPLE_RATE;
}