#include "wokwi-api.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
  pin_t ain[4];
  uint8_t pointer_reg;
  uint16_t config_reg;
  uint8_t config_bytes[2]; // MSB, LSB
  uint8_t config_byte_count;
  uint16_t conversion_reg;
  bool msb;
  bool expecting_pointer;
} chip_t;

static bool on_i2c_connect(void *user_data, uint32_t address, bool read);
static bool on_i2c_write(void *user_data, uint8_t data);
static uint8_t on_i2c_read(void *user_data);

static int16_t clamp(int32_t v) {
  if (v > 32767) return 32767;
  if (v < -32768) return -32768;
  return v;
}

void chip_init() {
  chip_t *chip = calloc(1, sizeof(chip_t));
  //printf("[chip-ads1115] chip_init chamada, chip=%p\n", chip);

  for (int i = 0; i < 4; i++) {
    char name[6];
    snprintf(name, sizeof(name), "AIN%d", i);
    chip->ain[i] = pin_init(name, ANALOG);
    //printf("[chip-ads1115] pin_init: %s -> pin=%d\n", name, chip->ain[i]);
  }
  chip->pointer_reg = 0x00;
  chip->msb = true;
  chip->config_byte_count = 0;
  chip->config_bytes[0] = 0x85; // default power-on reset
  chip->config_bytes[1] = 0x83;
  chip->config_reg = ((uint16_t)chip->config_bytes[0] << 8) | chip->config_bytes[1];
  chip->expecting_pointer = true;

  i2c_config_t cfg = {
    .address    = 0x48,
    .scl        = pin_init("SCL", INPUT_PULLUP),
    .sda        = pin_init("SDA", INPUT_PULLUP),
    .connect    = on_i2c_connect,
    .write      = on_i2c_write,
    .read       = on_i2c_read,
    .disconnect = NULL,
    .user_data  = chip
  };
  //printf("[chip-ads1115] Registrando i2c chip: addr=0x%02x, scl=%d, sda=%d\n", cfg.address, cfg.scl, cfg.sda);
  i2c_init(&cfg);

  //printf("[chip-ads1115] ADS1115 custom chip (FULL DEBUG, real-like) pronto!\n");
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_t *chip = user_data;
  //printf("[chip-ads1115] on_i2c_connect: addr=0x%02x, read=%d, chip=%p\n", address, read, chip);
  chip->expecting_pointer = true; // Sempre espera novo ponteiro após START
  chip->config_byte_count = 0;    // Reseta contagem de bytes para registrador config
  return address == 0x48;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_t *chip = user_data;
  //printf("[chip-ads1115] on_i2c_write: data=0x%02x, expecting_pointer=%d, pointer_reg=0x%02x, config_byte_count=%d\n", data, chip->expecting_pointer, chip->pointer_reg, chip->config_byte_count);

  if (chip->expecting_pointer) {
    chip->pointer_reg = data;
    chip->msb = true;
    chip->expecting_pointer = false;
    //printf("[chip-ads1115]   Novo pointer_reg=0x%02x, msb=%d\n", chip->pointer_reg, chip->msb);
    if (chip->pointer_reg == 0x01) {
      chip->config_byte_count = 0; // Vai começar escrita de config
      chip->config_bytes[0] = (chip->config_reg >> 8) & 0xFF;
      chip->config_bytes[1] = chip->config_reg & 0xFF;
    }
  } else {
    if (chip->pointer_reg == 0x01) {
      // Para config_reg, aceita até dois bytes, sobrescrevendo MSB e LSB
      chip->config_bytes[chip->config_byte_count++] = data;
      //printf("[chip-ads1115]   Armazenando config_bytes[%d]=0x%02x\n", chip->config_byte_count-1, data);

      // Sempre que receber um byte, atualiza parcialmente o config_reg
      if (chip->config_byte_count == 1) {
        // Apenas MSB recebido: LSB permanece o que estava antes!
        chip->config_reg = (chip->config_bytes[0] << 8) | (chip->config_bytes[1]);
        //printf("[chip-ads1115]   (MSB recebido) config_reg=0x%04x\n", chip->config_reg);
      } else if (chip->config_byte_count == 2) {
        chip->config_reg = (chip->config_bytes[0] << 8) | (chip->config_bytes[1]);
        //printf("[chip-ads1115]   (MSB+LSB recebidos) config_reg=0x%04x\n", chip->config_reg);
        chip->config_byte_count = 0;
        chip->expecting_pointer = true; // Após 2 bytes, espera novo ponteiro
      }
    } else {
      // Qualquer outro registrador: apenas um byte, já espera novo ponteiro em seguida
      chip->expecting_pointer = true;
    }
  }
  return true;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_t *chip = user_data;
 // printf("[chip-ads1115] on_i2c_read: pointer_reg=0x%02x, msb=%d\n", chip->pointer_reg, chip->msb);
  uint8_t out = 0xFF;

  if (chip->pointer_reg == 0x00) { // Conversion register
    if (chip->msb) {
      uint8_t mux = (chip->config_reg >> 12) & 0x07;
      float v = 0.0f;
      int16_t code = 0;
      const float fsr = 4.096f;
      float vin_p = 0, vin_n = 0, vdiff = 0;
      //printf("[chip-ads1115]   Conversão: mux=%d, config_reg=0x%04x\n", mux, chip->config_reg);
      switch (mux) {
        case 0: // AIN0-AIN1 (diferencial)
          vin_p = pin_adc_read(chip->ain[0]);
          vin_n = pin_adc_read(chip->ain[1]);
          vdiff = vin_p - vin_n;
          code = clamp((int32_t)((vdiff / fsr) * 32767.0f));
          //printf("[chip-ads1115]     MUX=0 Diferencial: AIN0=%.3fV, AIN1=%.3fV, vdiff=%.3fV, code=%d\n", vin_p, vin_n, vdiff, code);
          break;
        case 1: // AIN0-AIN3 (diferencial)
          vin_p = pin_adc_read(chip->ain[0]);
          vin_n = pin_adc_read(chip->ain[3]);
          vdiff = vin_p - vin_n;
          code = clamp((int32_t)((vdiff / fsr) * 32767.0f));
          //printf("[chip-ads1115]     MUX=1 Diferencial: AIN0=%.3fV, AIN3=%.3fV, vdiff=%.3fV, code=%d\n", vin_p, vin_n, vdiff, code);
          break;
        case 2: // AIN1-AIN3 (diferencial)
          vin_p = pin_adc_read(chip->ain[1]);
          vin_n = pin_adc_read(chip->ain[3]);
          vdiff = vin_p - vin_n;
          code = clamp((int32_t)((vdiff / fsr) * 32767.0f));
          //printf("[chip-ads1115]     MUX=2 Diferencial: AIN1=%.3fV, AIN3=%.3fV, vdiff=%.3fV, code=%d\n", vin_p, vin_n, vdiff, code);
          break;
        case 3: // AIN2-AIN3 (diferencial)
          vin_p = pin_adc_read(chip->ain[2]);
          vin_n = pin_adc_read(chip->ain[3]);
          vdiff = vin_p - vin_n;
          code = clamp((int32_t)((vdiff / fsr) * 32767.0f));
          //printf("[chip-ads1115]     MUX=3 Diferencial: AIN2=%.3fV, AIN3=%.3fV, vdiff=%.3fV, code=%d\n", vin_p, vin_n, vdiff, code);
          break;
        case 4: // AIN0-GND (single-ended)
          v = pin_adc_read(chip->ain[0]);
          code = clamp((int32_t)((v / fsr) * 32767.0f));
          //printf("[chip-ads1115]     MUX=4 Single-ended: AIN0=%.3fV, code=%d\n", v, code);
          break;
        case 5: // AIN1-GND
          v = pin_adc_read(chip->ain[1]);
          code = clamp((int32_t)((v / fsr) * 32767.0f));
          //printf("[chip-ads1115]     MUX=5 Single-ended: AIN1=%.3fV, code=%d\n", v, code);
          break;
        case 6: // AIN2-GND
          v = pin_adc_read(chip->ain[2]);
          code = clamp((int32_t)((v / fsr) * 32767.0f));
          //printf("[chip-ads1115]     MUX=6 Single-ended: AIN2=%.3fV, code=%d\n", v, code);
          break;
        case 7: // AIN3-GND
          v = pin_adc_read(chip->ain[3]);
          code = clamp((int32_t)((v / fsr) * 32767.0f));
          //printf("[chip-ads1115]     MUX=7 Single-ended: AIN3=%.3fV, code=%d\n", v, code);
          break;
        default:
          printf("[chip-ads1115]     MUX=%d não implementado\n", mux);
      }
      chip->conversion_reg = (uint16_t)code;
    }
    out = chip->msb ? (chip->conversion_reg >> 8) : (chip->conversion_reg & 0xFF);
    //printf("[chip-ads1115]     Retornando byte: 0x%02x\n", out);
    chip->msb = !chip->msb;
  }
  else if (chip->pointer_reg == 0x01) { // Config register
    out = chip->msb ? (chip->config_reg >> 8) : (chip->config_reg & 0xFF);
    //printf("[chip-ads1115]     Read config_reg: 0x%04x (byte: 0x%02x)\n", chip->config_reg, out);
    chip->msb = !chip->msb;
  }
  return out;
}
