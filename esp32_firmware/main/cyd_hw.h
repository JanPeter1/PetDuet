#ifndef CYD_HW_H
#define CYD_HW_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void cyd_hw_init(void);
void cyd_set_rgb(bool r, bool g, bool b);
bool cyd_boot_button_pressed(void);
uint32_t cyd_millis(void);
void cyd_delay_ms(uint32_t ms);

void cyd_play_tone(uint16_t freq, uint16_t duration_ms);
void cyd_sound_click(void);
void cyd_sound_munch(void);
void cyd_sound_treat_sent(void);
void cyd_sound_treat_received(void);
void cyd_sound_level_up(void);
void cyd_sound_hatch(void);
void cyd_sound_warning(void);

#ifdef __cplusplus
}
#endif

#endif // CYD_HW_H
