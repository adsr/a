#ifndef __UTIL_H
#define __UTIL_H

#include "atto.h"

void tb_print(int x, int y, uint16_t fg, uint16_t bg, char *str);
void tb_printf(bview_rect_t rect, int x, int y, uint16_t fg, uint16_t bg, const char *fmt, ...);

#endif
