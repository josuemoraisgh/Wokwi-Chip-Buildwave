#include <math.h>
#include "wokwi-api.h"
#define PI 3.141592653589793

static float freqs[5];
static float amplitude;

void chip_init(void) {
  freqs[0] = attr_read_float(attr_init_float("freq1", 10.0f));
  freqs[1] = attr_read_float(attr_init_float("freq2", 110.0f));
  freqs[2] = attr_read_float(attr_init_float("freq3", 0.0f));
  freqs[3] = attr_read_float(attr_init_float("freq4", 0.0f));
  freqs[4] = attr_read_float(attr_init_float("freq5", 0.0f));
  amplitude = attr_read_float(attr_init_float("amplitude", 1.0f));
}

void chip_tick(void) {
  double t = get_sim_nanos() * 1e-9;
  double y = 0.0;
  for (int i = 0; i < 5; i++) {
    if (freqs[i] > 0.0f) {
      y += sin(2.0 * PI * freqs[i] * t);
    }
  }
  pin_dac_write(pin_init("OUT", ANALOG), (float)(y * amplitude));
}
