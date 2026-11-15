#include "cyd_hw.h"
#include "../CYD_Pins.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CYD_BUZZER_MODE LEDC_LOW_SPEED_MODE
#define CYD_BUZZER_TIMER LEDC_TIMER_0
#define CYD_BUZZER_CHANNEL LEDC_CHANNEL_0
#define CYD_LDR_ADC_CHANNEL ADC_CHANNEL_6 // GPIO 34

static adc_oneshot_unit_handle_t s_adc1_handle = NULL;

void cyd_hw_init(void) {
  // RGB Status LEDs (Active LOW: 1 = OFF, 0 = ON)
  gpio_config_t led_conf = {
    .pin_bit_mask = (1ULL << CYD_LED_RED) | (1ULL << CYD_LED_GREEN) | (1ULL << CYD_LED_BLUE),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&led_conf);
  cyd_set_rgb(false, false, false);

  // Backlight GPIO initialized off until display ready
  gpio_config_t bl_conf = {
    .pin_bit_mask = (1ULL << CYD_TFT_BL),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&bl_conf);
  gpio_set_level((gpio_num_t)CYD_TFT_BL, 0);

  // BOOT Button (GPIO 0, internal pull-up)
  gpio_config_t boot_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_0),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&boot_conf);

  // LDR Light Sensor (GPIO 34 -> ADC1 Channel 6)
  adc_oneshot_unit_init_cfg_t adc_init_cfg = {
    .unit_id = ADC_UNIT_1,
  };
  if (adc_oneshot_new_unit(&adc_init_cfg, &s_adc1_handle) == ESP_OK && s_adc1_handle) {
    adc_oneshot_chan_cfg_t chan_cfg = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(s_adc1_handle, CYD_LDR_ADC_CHANNEL, &chan_cfg);
  }

  // Piezo Buzzer (LEDC PWM on GPIO 26)
  ledc_timer_config_t timer_conf = {
    .speed_mode = CYD_BUZZER_MODE,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .timer_num = CYD_BUZZER_TIMER,
    .freq_hz = 2000,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t ch_conf = {
    .gpio_num = CYD_SPEAKER_PIN,
    .speed_mode = CYD_BUZZER_MODE,
    .channel = CYD_BUZZER_CHANNEL,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = CYD_BUZZER_TIMER,
    .duty = 0,
    .hpoint = 0
  };
  ledc_channel_config(&ch_conf);
}

void cyd_set_rgb(bool r, bool g, bool b) {
  gpio_set_level((gpio_num_t)CYD_LED_RED, r ? 0 : 1);
  gpio_set_level((gpio_num_t)CYD_LED_GREEN, g ? 0 : 1);
  gpio_set_level((gpio_num_t)CYD_LED_BLUE, b ? 0 : 1);
}

bool cyd_boot_button_pressed(void) {
  return gpio_get_level(GPIO_NUM_0) == 0;
}

uint16_t cyd_read_ldr(void) {
  if (!s_adc1_handle) return 2048;
  int raw = 2048;
  if (adc_oneshot_read(s_adc1_handle, CYD_LDR_ADC_CHANNEL, &raw) == ESP_OK) {
    if (raw < 0) raw = 0;
    if (raw > 4095) raw = 4095;
    return (uint16_t)raw;
  }
  return 2048;
}

uint32_t cyd_millis(void) {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void cyd_delay_ms(uint32_t ms) {
  if (ms == 0) return;
  TickType_t ticks = pdMS_TO_TICKS(ms);
  if (ticks == 0) ticks = 1;
  vTaskDelay(ticks);
}

void cyd_play_tone(uint16_t freq, uint16_t duration_ms) {
  if (freq > 0) {
    ledc_set_freq(CYD_BUZZER_MODE, CYD_BUZZER_TIMER, freq);
    ledc_set_duty(CYD_BUZZER_MODE, CYD_BUZZER_CHANNEL, 128);
    ledc_update_duty(CYD_BUZZER_MODE, CYD_BUZZER_CHANNEL);
    cyd_delay_ms(duration_ms);
  }
  ledc_set_duty(CYD_BUZZER_MODE, CYD_BUZZER_CHANNEL, 0);
  ledc_update_duty(CYD_BUZZER_MODE, CYD_BUZZER_CHANNEL);
}

void cyd_sound_click(void) {
  cyd_play_tone(1000, 20);
}

void cyd_sound_munch(void) {
  cyd_play_tone(350, 30); cyd_delay_ms(15);
  cyd_play_tone(500, 35); cyd_delay_ms(15);
  cyd_play_tone(650, 45);
}

void cyd_sound_treat_sent(void) {
  cyd_set_rgb(false, true, true);
  cyd_play_tone(523, 50); cyd_delay_ms(20);
  cyd_play_tone(659, 50); cyd_delay_ms(20);
  cyd_play_tone(784, 90);
  cyd_set_rgb(false, true, false);
}

void cyd_sound_treat_received(void) {
  cyd_set_rgb(false, false, true);
  cyd_play_tone(659, 70); cyd_delay_ms(30);
  cyd_play_tone(880, 120);
  cyd_set_rgb(false, true, false);
}

void cyd_sound_level_up(void) {
  cyd_set_rgb(true, true, false);
  static const uint16_t notes[5] = {523, 659, 784, 1046, 1318};
  for (int i = 0; i < 5; i++) {
    cyd_play_tone(notes[i], 70);
    cyd_delay_ms(20);
  }
  cyd_set_rgb(false, true, false);
}

void cyd_sound_hatch(void) {
  cyd_set_rgb(true, true, true);
  static const uint16_t notes[5] = {440, 554, 659, 880, 1108};
  for (int i = 0; i < 5; i++) {
    cyd_play_tone(notes[i], 80);
    cyd_delay_ms(25);
  }
  cyd_set_rgb(false, true, false);
}

void cyd_sound_warning(void) {
  cyd_set_rgb(true, false, false);
  cyd_play_tone(240, 90); cyd_delay_ms(30);
  cyd_play_tone(180, 130);
  cyd_set_rgb(false, true, false);
}
