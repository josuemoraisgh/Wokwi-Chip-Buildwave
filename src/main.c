#include <stdint.h>
#include <math.h>
#include "wokwi-api.h"

typedef enum {
  WAVE_SINE,
  WAVE_TRIANGLE,
  WAVE_SQUARE
} waveform_t;

typedef struct {
  pin_t out_pin;
  waveform_t waveform;
  float freq;
  float amp;
  float t;
} chip_state_t;

chip_state_t chip;

#define SAMPLE_RATE 10000.0 // Hz

// Prototipos
void chip_init();
void chip_tick();

void chip_init() {
  chip.out_pin = pin_init("OUT", ANALOG_OUTPUT);
  chip.t = 0.0;

  // Atributos customizados
  const char *wf = attr_init("waveform", "sine");
  if (strcmp(wf, "triangle") == 0) {
    chip.waveform = WAVE_TRIANGLE;
  } else if (strcmp(wf, "square") == 0) {
    chip.waveform = WAVE_SQUARE;
  } else {
    chip.waveform = WAVE_SINE;
  }
  chip.freq = attr_init_float("frequency", 1000.0);
  chip.amp = attr_init_float("amplitude", 1.0);

  // Tick
  set_interval_us(chip_tick, (int)(1e6 / SAMPLE_RATE));
}

void chip_tick() {
  float out = 0;
  float period = 1.0 / chip.freq;
  float phase = fmodf(chip.t, period) / period; // [0,1)
  switch (chip.waveform) {
    case WAVE_SINE:
      out = chip.amp * sinf(2 * M_PI * phase);
      break;
    case WAVE_TRIANGLE:
      out = chip.amp * (4.0f * fabsf(phase - 0.5f) - 1.0f);
      break;
    case WAVE_SQUARE:
      out = chip.amp * (phase < 0.5f ? 1.0f : -1.0f);
      break;
  }
  pin_write_analog(chip.out_pin, out);
  chip.t += 1.0 / SAMPLE_RATE;
}