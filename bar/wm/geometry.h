#ifndef DWM_QS_GEOMETRY_H
#define DWM_QS_GEOMETRY_H
#include <stdint.h>
typedef struct { int x, y, width, height; } QsRect;
/* EWMH order: left, right, top, bottom, then each inclusive span. */
void qs_strut_apply(QsRect monitor, int rootw, int rooth,
                    const uint32_t strut[12], QsRect *area);
#endif
