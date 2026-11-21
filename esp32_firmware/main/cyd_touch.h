#ifndef CYD_TOUCH_H
#define CYD_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void cyd_touch_init(void);
bool cyd_touch_read(uint16_t *out_x, uint16_t *out_y);

#ifdef __cplusplus
}
#endif

#endif // CYD_TOUCH_H
