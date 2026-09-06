#include <assert.h>
#include <string.h>
#include "../geometry.h"

static void expect(QsRect a, int x, int y, int w, int h)
{
 assert(a.x == x && a.y == y && a.width == w && a.height == h);
}

int main(void)
{
 QsRect left = {0, 0, 1920, 1080}, right = {1920, 0, 1920, 1080}, a;
 uint32_t s[12] = {0};
 s[2] = 30; s[8] = 1920; s[9] = 3839;
 a = left; qs_strut_apply(left, 3840, 1080, s, &a); expect(a, 0, 0, 1920, 1080);
 a = right; qs_strut_apply(right, 3840, 1080, s, &a); expect(a, 1920, 30, 1920, 1050);
 /* Inclusive endpoint touches the last pixel of the left monitor. */
 s[8] = 1919;
 a = left; qs_strut_apply(left, 3840, 1080, s, &a); expect(a, 0, 30, 1920, 1050);
 /* Overlapping reservations use the maximum, not their sum. */
 s[2] = 20; qs_strut_apply(left, 3840, 1080, s, &a); expect(a, 0, 30, 1920, 1050);
 s[2] = 40; qs_strut_apply(left, 3840, 1080, s, &a); expect(a, 0, 40, 1920, 1040);
 memset(s, 0, sizeof s);
 s[0] = 20; s[1] = 25; s[3] = 40;
 s[5] = s[7] = 1079; s[11] = 3839;
 a = left; qs_strut_apply(left, 3840, 1080, s, &a); expect(a, 20, 0, 1900, 1040);
 a = right; qs_strut_apply(right, 3840, 1080, s, &a); expect(a, 1920, 0, 1895, 1040);
 /* A top strut on an offset monitor includes the root gap. */
 left = (QsRect){0, 100, 1920, 980}; memset(s, 0, sizeof s);
 s[2] = 130; s[9] = 1919;
 a = left; qs_strut_apply(left, 3840, 1080, s, &a); expect(a, 0, 130, 1920, 950);
 s[8] = 200; s[9] = 100;
 a = left; qs_strut_apply(left, 3840, 1080, s, &a); expect(a, 0, 100, 1920, 980);
 s[8] = 0; s[9] = UINT32_MAX; s[2] = UINT32_MAX;
 a = left; qs_strut_apply(left, 3840, 1080, s, &a); expect(a, 0, 1079, 1920, 1);
 return 0;
}
