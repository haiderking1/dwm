#include <stdio.h>
#include "../json.h"
int main(void)
{
 const char controls[] = {34, 92, 10, 9, 13, 1, 0};
 const char truncated[] = {(char)0xf0, (char)0x9f, 0};
 const char surrogate[] = {(char)0xed, (char)0xa0, (char)0x80, 0};
 qs_json_string(stdout, controls); puts("");
 qs_json_string(stdout, "日本語 😀"); puts("");
 qs_json_string(stdout, truncated); puts("");
 qs_json_string(stdout, surrogate); puts("");
 qs_json_string(stdout, ""); puts("");
 return 0;
}
