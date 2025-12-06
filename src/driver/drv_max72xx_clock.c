#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_deviceclock.h"
#include "drv_max72xx_internal.h"
//#include <time.h>
#include "../libraries/obktime/obktime.h"	// for time functions

char *my_strcat(char *p, const char *s) {
	strcat(p, s);
	return p + strlen(s);
}
char *add_padded(char *o, int i) {
	if (i < 10) {
		*o = '0';
		o++;
	}
	sprintf(o, "%i", i);
	return o + strlen(o);
}
static bool g_bdisptime = false;	// keep track, if displaying time - then allow QuickTick to avoid missing seconds dicplay
static uint8_t g_QuickTickCount=0;
static uint8_t g_TimeSentCount=0;

enum {
	CLOCK_DATE,
	CLOCK_TIME,
	CLOCK_HUMIDITY,
	CLOCK_TEMPERATURE,
};


typedef struct {
    int width;                // Width in pixels
    unsigned char *bitmap;    // Pointer to dynamically allocated bitmap data
} FontCharacter;

// For defining directly in your code
FontCharacter font[] = {
	{0, NULL},                                                  // 0 - Unused
	{0, NULL},                                                  // 1 - Unused
	{0, NULL},                                                  // 2 - Unused
	{0, NULL},                                                  // 3 - Unused
	{0, NULL},                                                  // 4 - Unused
	{0, NULL},                                                  // 5 - Unused
	{0, NULL},                                                  // 6 - Unused
	{0, NULL},                                                  // 7 - Unused
	{0, NULL},                                                  // 8 - Unused
	{0, NULL},                                                  // 9 - Unused
	{3,(unsigned char[]){ 124, 68, 124}},                       // 10 - 0 mini
	{3,(unsigned char[]){ 0, 124, 0}},                          // 11 - 1 mini
	{3,(unsigned char[]){ 116, 84, 92}},                        // 12 - 2 mini
	{3,(unsigned char[]){ 84, 84, 124}},                        // 13 - 3 mini
	{3,(unsigned char[]){ 28, 16, 124}},                        // 14 - 4 mini
	{3,(unsigned char[]){ 92, 84, 116}},                        // 15 - 5 mini
	{3,(unsigned char[]){ 124, 84, 116}},                       // 16 - 6 mini
	{3,(unsigned char[]){ 4, 116, 12}},                         // 17 - 7 mini
	{3,(unsigned char[]){ 124, 84, 124}},                       // 18 - 8 mini
	{3,(unsigned char[]){ 92, 84, 124}},                        // 19 - 9 mini
	{0, NULL},                                                  // 20 - Unused
	{0, NULL},                                                  // 21 - Unused
	{0, NULL},                                                  // 22 - Unused
	{0, NULL},                                                  // 23 - Unused
	{0, NULL},                                                  // 24 - Unused
	{0, NULL},                                                  // 25 - Unused
	{0, NULL},                                                  // 26 - Unused
	{0, NULL},                                                  // 27 - Unused
	{0, NULL},                                                  // 28 - Unused
	{0, NULL},                                                  // 29 - Unused
	{0, NULL},                                                  // 30 - Unused
	{0, NULL},                                                  // 31 - Unused
	{2,(unsigned char[]){ 0, 0}},                               // 32 - Space
	{1,(unsigned char[]){ 95}},                                 // 33 - !
	{3,(unsigned char[]){ 7, 0, 7}},                            // 34 - "
	{5,(unsigned char[]){ 20, 127, 20, 127, 20}},               // 35 - 
	{5,(unsigned char[]){ 36, 42, 127, 42, 18}},                // 36 - $
	{5,(unsigned char[]){ 35, 19, 8, 100, 98}},                 // 37 - %
	{5,(unsigned char[]){ 54, 73, 86, 32, 80}},                 // 38 - &
	{2,(unsigned char[]){ 4, 3}},                               // 39
	{3,(unsigned char[]){ 28, 34, 65}},                         // 40 - (
	{3,(unsigned char[]){ 65, 34, 28}},                         // 41 - )
	{5,(unsigned char[]){ 42, 28, 127, 28, 42}},                // 42 - *
	{5,(unsigned char[]){ 8, 8, 62, 8, 8}},                     // 43 - +
	{2,(unsigned char[]){ 128, 96}},                            // 44 - ,
	{5,(unsigned char[]){ 8, 8, 8, 8, 8}},                      // 45 - -
	{1,(unsigned char[]){ 64}},                                 // 46 - .	(smaller . )
	{5,(unsigned char[]){ 32, 16, 8, 4, 2}},                    // 47 - /
	{4,(unsigned char[]){ 62, 65, 65, 62}},                     // 48 - 0  (4-column small) - rounded
	{4,(unsigned char[]){ 0, 66, 127, 64}},                     // 49 - 1  (4-column small)
	{4,(unsigned char[]){ 114, 73, 73, 70}},                    // 50 - 2  (4-column small) - rounded
	{4,(unsigned char[]){ 34, 73, 73, 62}},                     // 51 - 3  (4-column small) - rounded
	{4,(unsigned char[]){ 12, 10, 127, 8}},                     // 52 - 4  (4-column small)
	{4,(unsigned char[]){ 39, 73, 73, 49}},                     // 53 - 5  (4-column small) - rounded
	{4,(unsigned char[]){ 62, 73, 73, 50}},                     // 54 - 6  (4-column small) - rounded
	{4,(unsigned char[]){ 1, 121, 5, 3}},                       // 55 - 7  (4-column small)
	{4,(unsigned char[]){ 54, 73, 73, 54}},                     // 56 - 8  (4-column small) - rounded
	{4,(unsigned char[]){ 38, 73, 73, 62}},                     // 57 - 9  (4-column small) - rounded
	{1,(unsigned char[]){ 20}},                                 // 58 - :
	{2,(unsigned char[]){ 128, 104}},                           // 59 - ;
	{4,(unsigned char[]){ 8, 20, 34, 65}},                      // 60 - <
	{5,(unsigned char[]){ 20, 20, 20, 20, 20}},                 // 61 - =
	{4,(unsigned char[]){ 65, 34, 20, 8}},                      // 62 - >
	{5,(unsigned char[]){ 2, 1, 89, 9, 6}},                     // 63 - ?
	{5,(unsigned char[]){ 62, 65, 93, 89, 78}},                 // 64 - @
	{5,(unsigned char[]){ 124, 18, 17, 18, 124}},               // 65 - A
	{5,(unsigned char[]){ 127, 73, 73, 73, 54}},                // 66 - B
	{5,(unsigned char[]){ 62, 65, 65, 65, 34}},                 // 67 - C
	{5,(unsigned char[]){ 127, 65, 65, 65, 62}},                // 68 - D
	{5,(unsigned char[]){ 127, 73, 73, 73, 65}},                // 69 - E
	{5,(unsigned char[]){ 127, 9, 9, 9, 1}},                    // 70 - F
	{5,(unsigned char[]){ 62, 65, 65, 81, 115}},                // 71 - G
	{5,(unsigned char[]){ 127, 8, 8, 8, 127}},                  // 72 - H
	{3,(unsigned char[]){ 65, 127, 65}},                        // 73 - I
	{5,(unsigned char[]){ 32, 64, 65, 63, 1}},                  // 74 - J
	{5,(unsigned char[]){ 127, 8, 20, 34, 65}},                 // 75 - K
	{5,(unsigned char[]){ 127, 64, 64, 64, 64}},                // 76 - L
	{5,(unsigned char[]){ 127, 2, 28, 2, 127}},                 // 77 - M
	{5,(unsigned char[]){ 127, 4, 8, 16, 127}},                 // 78 - N
	{5,(unsigned char[]){ 62, 65, 65, 65, 62}},                 // 79 - O
	{5,(unsigned char[]){ 127, 9, 9, 9, 6}},                    // 80 - P
	{5,(unsigned char[]){ 62, 65, 81, 33, 94}},                 // 81 - Q
	{5,(unsigned char[]){ 127, 9, 25, 41, 70}},                 // 82 - R
	{5,(unsigned char[]){ 38, 73, 73, 73, 50}},                 // 83 - S
	{5,(unsigned char[]){ 3, 1, 127, 1, 3}},                    // 84 - T
	{5,(unsigned char[]){ 63, 64, 64, 64, 63}},                 // 85 - U
	{5,(unsigned char[]){ 31, 32, 64, 32, 31}},                 // 86 - V
	{5,(unsigned char[]){ 63, 64, 56, 64, 63}},                 // 87 - W
	{5,(unsigned char[]){ 99, 20, 8, 20, 99}},                  // 88 - X
	{5,(unsigned char[]){ 3, 4, 120, 4, 3}},                    // 89 - Y
	{5,(unsigned char[]){ 97, 89, 73, 77, 67}},                 // 90 - Z
	{3,(unsigned char[]){ 127, 65, 65}},                        // 91 - [
	{5,(unsigned char[]){ 2, 4, 8, 16, 32}},                    // 92 - \'
	{3,(unsigned char[]){ 65, 65, 127}},                        // 93 - ]
	{5,(unsigned char[]){ 4, 2, 1, 2, 4}},                      // 94 - ^
	{5,(unsigned char[]){ 64, 64, 64, 64, 64}},                 // 95 - _
	{2,(unsigned char[]){ 3, 4}},                               // 96 - `
	{5,(unsigned char[]){ 32, 84, 84, 120, 64}},                // 97 - a
	{5,(unsigned char[]){ 127, 40, 68, 68, 56}},                // 98 - b
	{5,(unsigned char[]){ 56, 68, 68, 68, 40}},                 // 99 - c
	{5,(unsigned char[]){ 56, 68, 68, 40, 127}},                // 100 - d
	{5,(unsigned char[]){ 56, 84, 84, 84, 24}},                 // 101 - e
	{4,(unsigned char[]){ 8, 126, 9, 2}},                       // 102 - f
	{5,(unsigned char[]){ 24, 164, 164, 156, 120}},             // 103 - g
	{5,(unsigned char[]){ 127, 8, 4, 4, 120}},                  // 104 - h
	{3,(unsigned char[]){ 68, 125, 64}},                        // 105 - i
	{4,(unsigned char[]){ 64, 128, 128, 122}},                  // 106 - j
	{4,(unsigned char[]){ 127, 16, 40, 68}},                    // 107 - k
	{3,(unsigned char[]){ 65, 127, 64}},                        // 108 - l
	{5,(unsigned char[]){ 124, 4, 120, 4, 120}},                // 109 - m
	{5,(unsigned char[]){ 124, 8, 4, 4, 120}},                  // 110 - n
	{5,(unsigned char[]){ 56, 68, 68, 68, 56}},                 // 111 - o
	{5,(unsigned char[]){ 252, 24, 36, 36, 24}},                // 112 - p
	{5,(unsigned char[]){ 24, 36, 36, 24, 252}},                // 113 - q
	{5,(unsigned char[]){ 124, 8, 4, 4, 8}},                    // 114 - r
	{5,(unsigned char[]){ 72, 84, 84, 84, 36}},                 // 115 - s
	{4,(unsigned char[]){ 4, 63, 68, 36}},                      // 116 - t
	{5,(unsigned char[]){ 60, 64, 64, 32, 124}},                // 117 - u
	{5,(unsigned char[]){ 28, 32, 64, 32, 28}},                 // 118 - v
	{5,(unsigned char[]){ 60, 64, 48, 64, 60}},                 // 119 - w
	{5,(unsigned char[]){ 68, 40, 16, 40, 68}},                 // 120 - x
	{5,(unsigned char[]){ 76, 144, 144, 144, 124}},             // 121 - y
	{5,(unsigned char[]){ 68, 100, 84, 76, 68}},                // 122 - z
	{3,(unsigned char[]){ 8, 54, 65}},                          // 123 - {
	{1,(unsigned char[]){ 119}},                                // 124 - |
	{3,(unsigned char[]){ 65, 54, 8}},                          // 125 - }
	{5,(unsigned char[]){ 2, 1, 2, 4, 2}},                      // 126 - ~
	{0, NULL},                                                  // 127 - Unused
	{6,(unsigned char[]){ 20, 62, 85, 85, 65, 34}},             // 128 - Euro sign
	{1,(unsigned char[]){ 0}},                                  // 129 - one single empty "line" - was: Not used
	{2,(unsigned char[]){ 128, 96}},                            // 130 - Single low 9 quotation mark
	{5,(unsigned char[]){ 192, 136, 126, 9, 3}},                // 131 - f with hook
	{4,(unsigned char[]){ 128, 96, 128, 96}},                   // 132 - Single low 9 quotation mark
	{8,(unsigned char[]){ 96, 96, 0, 96, 96, 0, 96, 96}},       // 133 - Horizontal ellipsis
	{3,(unsigned char[]){ 4, 126, 4}},                          // 134 - Dagger
	{3,(unsigned char[]){ 20, 126, 20}},                        // 135 - Double dagger
	{4,(unsigned char[]){ 2, 1, 1, 2}},                         // 136 - Modifier circumflex
	{7,(unsigned char[]){ 35, 19, 104, 100, 2, 97, 96}},        // 137 - Per mille sign
	{5,(unsigned char[]){ 72, 85, 86, 85, 36}},                 // 138 - S with caron
	{3,(unsigned char[]){ 8, 20, 34}},                          // 139 - < quotation
	{6,(unsigned char[]){ 62, 65, 65, 127, 73, 73}},            // 140 - OE
	{0, NULL},                                                  // 141 - Not used
	{5,(unsigned char[]){ 68, 101, 86, 77, 68}},                // 142 - z with caron
	{0, NULL},                                                  // 143 - Not used
	{0, NULL},                                                  // 144 - Not used
	{2,(unsigned char[]){ 3, 4}},                               // 145 - Left single quote mark
	{2,(unsigned char[]){ 4, 3}},                               // 146 - Right single quote mark
	{4,(unsigned char[]){ 3, 4, 3, 4}},                         // 147 - Left double quote marks
	{4,(unsigned char[]){ 4, 3, 4, 3}},                         // 148 - Right double quote marks
	{4,(unsigned char[]){ 0, 24, 60, 24}},                      // 149 - Bullet Point
	{3,(unsigned char[]){ 8, 8, 8}},                            // 150 - En dash
	{5,(unsigned char[]){ 8, 8, 8, 8, 8}},                      // 151 - Em dash
	{4,(unsigned char[]){ 2, 1, 2, 1}},                         // 152 - Small ~
	{7,(unsigned char[]){ 1, 15, 1, 0, 15, 2, 15}},             // 153 - TM
	{5,(unsigned char[]){ 72, 85, 86, 85, 36}},                 // 154 - s with caron
	{3,(unsigned char[]){ 34, 20, 8}},                          // 155 - > quotation
	{7,(unsigned char[]){ 56, 68, 68, 124, 84, 84, 8}},         // 156 - oe
	{0, NULL},                                                  // 157 - Not used
	{5,(unsigned char[]){ 68, 101, 86, 77, 68}},                // 158 - z with caron
	{5,(unsigned char[]){ 12, 17, 96, 17, 12}},                 // 159 - Y diaresis
	{2,(unsigned char[]){ 0, 0}},                               // 160 - Non-breaking space
	{1,(unsigned char[]){ 125}},                                // 161 - Inverted !
	{5,(unsigned char[]){ 60, 36, 126, 36, 36}},                // 162 - Cent sign
	{5,(unsigned char[]){ 72, 126, 73, 65, 102}},               // 163 - Pound sign
	{5,(unsigned char[]){ 34, 28, 20, 28, 34}},                 // 164 - Currency sign
	{5,(unsigned char[]){ 43, 47, 252, 47, 43}},                // 165 - Yen
	{1,(unsigned char[]){ 119}},                                // 166 - |
	{4,(unsigned char[]){ 102, 137, 149, 106}},                 // 167 - Section sign
	{3,(unsigned char[]){ 1, 0, 1}},                            // 168 - Spacing diaresis
	{7,(unsigned char[]){ 62, 65, 93, 85, 85, 65, 62}},         // 169 - Copyright
	{3,(unsigned char[]){ 13, 13, 15}},                         // 170 - Feminine Ordinal Ind.
	{5,(unsigned char[]){ 8, 20, 42, 20, 34}},                  // 171 - <<
	{5,(unsigned char[]){ 8, 8, 8, 8, 56}},                     // 172 - Not sign
	{0, NULL},                                                  // 173 - Soft Hyphen
	{7,(unsigned char[]){ 62, 65, 127, 75, 117, 65, 62}},       // 174 - Registered Trademark
	{5,(unsigned char[]){ 1, 1, 1, 1, 1}},                      // 175 - Spacing Macron Overline
	{3,(unsigned char[]){ 2, 5, 2}},                            // 176 - Degree
	{5,(unsigned char[]){ 68, 68, 95, 68, 68}},                 // 177 - +/-
	{3,(unsigned char[]){ 25, 21, 19}},                         // 178 - Superscript 2
	{3,(unsigned char[]){ 17, 21, 31}},                         // 179 - Superscript 3
	{2,(unsigned char[]){ 2, 1}},                               // 180 - Acute accent
	{4,(unsigned char[]){ 252, 64, 64, 60}},                    // 181 - micro (mu)
	{5,(unsigned char[]){ 6, 9, 127, 1, 127}},                  // 182 - Paragraph Mark
	{2,(unsigned char[]){ 24, 24}},                             // 183 - Middle Dot
	{3,(unsigned char[]){ 128, 128, 96}},                       // 184 - Spacing sedilla
	{2,(unsigned char[]){ 2, 31}},                              // 185 - Superscript 1
	{4,(unsigned char[]){ 6, 9, 9, 6}},                         // 186 - Masculine Ordinal Ind.
	{5,(unsigned char[]){ 34, 20, 42, 20, 8}},                  // 187 - >>
	{6,(unsigned char[]){ 64, 47, 16, 40, 52, 250}},            // 188 - 1/4
	{6,(unsigned char[]){ 64, 47, 16, 200, 172, 186}},          // 189 - 1/2
	{6,(unsigned char[]){ 85, 53, 31, 40, 52, 250}},            // 190 - 3/4
	{5,(unsigned char[]){ 48, 72, 77, 64, 32}},                 // 191 - Inverted ?
	{5,(unsigned char[]){ 120, 20, 21, 22, 120}},               // 192 - A grave
	{5,(unsigned char[]){ 120, 22, 21, 20, 120}},               // 193 - A acute
	{5,(unsigned char[]){ 122, 21, 21, 21, 122}},               // 194 - A circumflex
	{5,(unsigned char[]){ 120, 22, 21, 22, 121}},               // 195 - A tilde
	{5,(unsigned char[]){ 120, 21, 20, 21, 120}},               // 196 - A diaresis
	{5,(unsigned char[]){ 120, 20, 21, 20, 120}},               // 197 - A ring above
	{6,(unsigned char[]){ 124, 10, 9, 127, 73, 73}},            // 198 - AE
	{5,(unsigned char[]){ 30, 161, 161, 97, 18}},               // 199 - C sedilla
	{4,(unsigned char[]){ 124, 85, 86, 68}},                    // 200 - E grave
	{4,(unsigned char[]){ 124, 86, 85, 68}},                    // 201 - E acute
	{4,(unsigned char[]){ 126, 85, 85, 70}},                    // 202 - E circumflex
	{4,(unsigned char[]){ 124, 85, 84, 69}},                    // 203 - E diaresis
	{3,(unsigned char[]){ 68, 125, 70}},                        // 204 - I grave
	{3,(unsigned char[]){ 68, 126, 69}},                        // 205 - I acute
	{3,(unsigned char[]){ 70, 125, 70}},                        // 206 - I circumplex
	{3,(unsigned char[]){ 69, 124, 69}},                        // 207 - I diaresis
	{6,(unsigned char[]){ 4, 127, 69, 65, 65, 62}},             // 208 - Capital Eth
	{5,(unsigned char[]){ 124, 10, 17, 34, 125}},               // 209 - N tilde
	{5,(unsigned char[]){ 56, 68, 69, 70, 56}},                 // 210 - O grave
	{5,(unsigned char[]){ 56, 70, 69, 68, 56}},                 // 211 - O acute
	{5,(unsigned char[]){ 58, 69, 69, 69, 58}},                 // 212 - O circumflex
	{5,(unsigned char[]){ 56, 70, 69, 70, 57}},                 // 213 - O tilde
	{5,(unsigned char[]){ 56, 69, 68, 69, 56}},                 // 214 - O diaresis
	{5,(unsigned char[]){ 34, 20, 8, 20, 34}},                  // 215 - Multiplication sign
	{7,(unsigned char[]){ 64, 62, 81, 73, 69, 62, 1}},          // 216 - O slashed
	{5,(unsigned char[]){ 60, 65, 66, 64, 60}},                 // 217 - U grave
	{5,(unsigned char[]){ 60, 64, 66, 65, 60}},                 // 218 - U acute
	{5,(unsigned char[]){ 58, 65, 65, 65, 58}},                 // 219 - U circumflex
	{5,(unsigned char[]){ 60, 65, 64, 65, 60}},                 // 220 - U diaresis
	{5,(unsigned char[]){ 12, 16, 98, 17, 12}},                 // 221 - Y acute
	{4,(unsigned char[]){ 127, 18, 18, 12}},                    // 222 - Capital thorn
	{4,(unsigned char[]){ 254, 37, 37, 26}},                    // 223 - Small letter sharp S
	{5,(unsigned char[]){ 32, 84, 85, 122, 64}},                // 224 - a grave
	{5,(unsigned char[]){ 32, 84, 86, 121, 64}},                // 225 - a acute
	{5,(unsigned char[]){ 34, 85, 85, 121, 66}},                // 226 - a circumflex
	{5,(unsigned char[]){ 32, 86, 85, 122, 65}},                // 227 - a tilde
	{5,(unsigned char[]){ 32, 85, 84, 121, 64}},                // 228 - a diaresis
	{5,(unsigned char[]){ 32, 84, 85, 120, 64}},                // 229 - a ring above
	{7,(unsigned char[]){ 32, 84, 84, 124, 84, 84, 8}},         // 230 - ae
	{5,(unsigned char[]){ 24, 36, 164, 228, 40}},               // 231 - c sedilla
	{5,(unsigned char[]){ 56, 84, 85, 86, 88}},                 // 232 - e grave
	{5,(unsigned char[]){ 56, 84, 86, 85, 88}},                 // 233 - e acute
	{5,(unsigned char[]){ 58, 85, 85, 85, 90}},                 // 234 - e circumflex
	{5,(unsigned char[]){ 56, 85, 84, 85, 88}},                 // 235 - e diaresis
	{3,(unsigned char[]){ 68, 125, 66}},                        // 236 - i grave
	{3,(unsigned char[]){ 68, 126, 65}},                        // 237 - i acute
	{3,(unsigned char[]){ 70, 125, 66}},                        // 238 - i circumflex
	{3,(unsigned char[]){ 69, 124, 65}},                        // 239 - i diaresis
	{4,(unsigned char[]){ 48, 75, 74, 61}},                     // 240 - Small eth
	{4,(unsigned char[]){ 122, 9, 10, 113}},                    // 241 - n tilde
	{5,(unsigned char[]){ 56, 68, 69, 70, 56}},                 // 242 - o grave
	{5,(unsigned char[]){ 56, 70, 69, 68, 56}},                 // 243 - o acute
	{5,(unsigned char[]){ 58, 69, 69, 69, 58}},                 // 244 - o circumflex
	{5,(unsigned char[]){ 56, 70, 69, 70, 57}},                 // 245 - o tilde
	{5,(unsigned char[]){ 56, 69, 68, 69, 56}},                 // 246 - o diaresis
	{5,(unsigned char[]){ 8, 8, 42, 8, 8}},                     // 247 - Division sign
	{6,(unsigned char[]){ 64, 56, 84, 76, 68, 58}},             // 248 - o slashed
	{5,(unsigned char[]){ 60, 65, 66, 32, 124}},                // 249 - u grave
	{5,(unsigned char[]){ 60, 64, 66, 33, 124}},                // 250 - u acute
	{5,(unsigned char[]){ 58, 65, 65, 33, 122}},                // 251 - u circumflex
	{5,(unsigned char[]){ 60, 65, 64, 33, 124}},                // 252 - u diaresis
	{4,(unsigned char[]){ 156, 162, 161, 124}},                 // 253 - y acute
	{4,(unsigned char[]){ 252, 72, 72, 48}},                    // 254 - small thorn
	{4,(unsigned char[]){ 157, 160, 160, 125}},                 // 255 - y diaresis
};	                                               

char disparr[64]={0};




/*
void Clock_Send(int type) {
	char time[64];
	struct tm *ltm;
	float val;
	char *p;
	time_t ntpTime;

	ntpTime=(time_t)TIME_GetCurrentTime();
	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&ntpTime);

	if (ltm == 0) {
		return;
	}
	time[0] = 0;
	p = time;
	if (type == CLOCK_TIME) {
		p = my_strcat(p, " ");
		p = add_padded(p, ltm->tm_hour);
		p = my_strcat(p, ":");
		p = add_padded(p, ltm->tm_min);
		strcat(p, "   ");
	}
	else if (type == CLOCK_DATE) {
		p = my_strcat(p, " ");
		p = add_padded(p, ltm->tm_mday);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mon + 1);
		//p = my_strcat(p, ".");
		//p = add_padded(p, ltm->tm_year);
		strcat(p, "   ");
	}
	else if (type == CLOCK_HUMIDITY) {
		if (false==CHANNEL_GetGenericHumidity(&val)) {
			// failed - exit early, do not change string
			return;
		}
		sprintf(time, "H: %i%%   ", (int)val);
	} 
	else if (type == CLOCK_TEMPERATURE) {
		if (false == CHANNEL_GetGenericTemperature(&val)) {
			// failed - exit early, do not change string
			return;
		}
		sprintf(time, "T: %iC    ", (int)val);
	}

	CMD_ExecuteCommandArgs("MAX72XX_Print", time, 0);
}
*/



void transposeString(char *input, int shift) {
    for (int i = 0; i < strlen(input); i++) {
        input[i] += shift;  // Shift each character by the specified amount
    }
}

/*
void print2arr(char *text, FontCharacter *f){
	char *p=text;
	if (!f) return;
	uint8_t ofs = 0;
	memset(disparr, 0, sizeof(disparr));	// set to 0
	while(p){
		FontCharacter c=f[(int)*p];
		uint8_t w = c.width ;
//		ADDLOG_INFO(LOG_FEATURE_RAW, "MAX72xx_clock - print2arr, \"printing\" char \"%c\" - width=%i  -- x%02x x%02x x%02x x%02x x%02x",*p, w, c.bitmap[0], c.bitmap[1], c.bitmap[2], c.bitmap[3], c.bitmap[4]);
		if ( (ofs + w) < sizeof(disparr)) memcpy(disparr + ofs, c.bitmap, w);
//		for (int i=0; ofs < sizeof(disparr) && i<c.width ; i++){		// todo: check for max width
//			disparr[ofs] = c.bitmap[i];
//			ofs++ 
//		}
		ofs += w+1;	// add +1, to (possibly) add an empty column (only used, if there is another char im p)
		if (ofs >=sizeof(disparr)) return;	// break, if array is full
		p++;
	}

}
*/
void print2arr(char *text, FontCharacter *f){
    char *p = text;
    if (!f) return;
    uint8_t ofs = 0;
    memset(disparr, 0, sizeof(disparr)); // set to 0

    while (*p != '\0') {
        int index = (int)*p; // get the ASCII index
        if (index < 0 || index >= 256) { 
            p++; // Skip invalid characters
            continue;
        }
        FontCharacter c = f[index];
        uint8_t w = c.width;
        if ((ofs + w) < sizeof(disparr) && c.bitmap) {
            memcpy(disparr + ofs, c.bitmap, w);
            ofs += w; // adjust offset after the copy
        }
        // Break if the next position would exceed the size of disparr
        if ((ofs + 1) >= sizeof(disparr)) {
            return; // break, if array is full
        }
        ofs += 1; // add an empty column (if needed)
        p++;
    }
}


void Clock_Send(int type) {
	char time[64];
	TimeComponents tc;
	float val;
	char *p;
	time_t ntpTime;

	ntpTime=(time_t)TIME_GetCurrentTime();
	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	tc=calculateComponents((uint32_t)ntpTime);

	time[0] = 0;
	p = time;
	char tempstr[5];	// temp string for seconds or year -> converted to "small" digits
	if (type == CLOCK_TIME) {
/*
//		p = my_strcat(p, " ");
		p = add_padded(p, tc.hour);
		p = my_strcat(p, " ");
//		p = my_strcat(p, ":");
		p = add_padded(p, tc.minute);
		p = my_strcat(p, " ");
//		p = my_strcat(p, ":");
		p = add_padded(p, tc.second);
		strcat(p, "   ");
*/
		g_TimeSentCount++;
		sprintf(tempstr,"%02i",tc.second);
		transposeString(tempstr, -38);		// move second digits from 48 - 57  --> 10 - 19 (small numbers)
		sprintf(time,"%02i:%02i%c%s", tc.hour,tc.minute, 127, tempstr);
	}
	else if (type == CLOCK_DATE) {
/*
//		p = my_strcat(p, " ");
		p = add_padded(p, tc.day);
		p = my_strcat(p, ".");
		p = add_padded(p, tc.month);
		p = my_strcat(p, ".");
		//p = add_padded(p, tc.year);
		strcat(p, "   ");
*/
		sprintf(tempstr,"%02i",(tc.year%100));
		transposeString(tempstr, -38);		// move year digits from 48 - 57  --> 10 - 19 (small numbers)		
		sprintf(time,"%02i.%02i.%c%s", tc.day,tc.month, 127, tempstr);
	}
	else if (type == CLOCK_HUMIDITY) {
		if (false==CHANNEL_GetGenericHumidity(&val)) {
			// failed - exit early, do not change string
			return;
		}
		sprintf(time, "H: %i%%   ", (int)val);
	} 
	else if (type == CLOCK_TEMPERATURE) {
		if (false == CHANNEL_GetGenericTemperature(&val)) {
			// failed - exit early, do not change string
			return;
		}
//		sprintf(time, "T: %iC    ", (int)val);
		sprintf(time, "0123456789");
	}

//	CMD_ExecuteCommandArgs("MAX72XX_Print", time, 0);
	print2arr(time, font);
	MAX72XX_printRaw(disparr, 64); 

}

void Clock_SendTime() {
	Clock_Send(CLOCK_TIME);
}
void Clock_SendDate() {
	Clock_Send(CLOCK_DATE);
}
void Clock_SendHumidity() {
	Clock_Send(CLOCK_HUMIDITY);
}
void Clock_SendTemperature() {
	Clock_Send(CLOCK_TEMPERATURE);
}
static unsigned int cycle = 0;

void Run_NoAnimation() {
	cycle+=4;
	if (cycle < 0) {
		g_bdisptime = false;
		Clock_SendDate();
	}
//	else if(cycle < 20) {
	else {
		g_bdisptime = true;
		Clock_SendTime();
	}
/*
	else if (cycle < 30) {
		Clock_SendHumidity();
	}
	else {
		Clock_SendTemperature();
	}
*/
	CMD_ExecuteCommandArgs("MAX72XX_refresh", "", 0);
	cycle %= 40;
}
static int g_del = 0;
/*
void Run_Animated() {
	cycle++;
	if (cycle < 4) {
		return;
	}
	cycle = 0;
	char time[64];
	struct tm *ltm;
	char *p;
	time_t ntpTime;

	ntpTime=(time_t)TIME_GetCurrentTime();
	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&ntpTime);

	if (ltm == 0) {
		return;
	}
	int scroll = MAX72XXSingle_GetScrollCount();
	//scroll_cycle = 0;
	if (scroll == 0 && g_del == 0) {
		time[0] = 0;
		p = time;
		p = my_strcat(p, "  ");

		p = add_padded(p, ltm->tm_hour);
		p = my_strcat(p, ":");
		p = add_padded(p, ltm->tm_min);
		strcat(p, " ");

		p = my_strcat(p, " ");
		p = add_padded(p, ltm->tm_year+1900);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mon + 1);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mday);
		strcat(p, "   ");

		CMD_ExecuteCommandArgs("MAX72XX_Clear", NULL, 0);
		CMD_ExecuteCommandArgs("MAX72XX_Print", time, 0);
		CMD_ExecuteCommandArgs("MAX72XX_refresh", "", 0);
		g_del = 10;
	}
	if (g_del > 0) {
		g_del--;
		return;
	}
	CMD_ExecuteCommandArgs("MAX72XX_Scroll", "-1", 0);
	CMD_ExecuteCommandArgs("MAX72XX_refresh", "", 0);
}
*/
void Run_Animated() {
	cycle++;
	if (cycle < 4) {
		return;
	}
	cycle = 0;
	char time[64];
	TimeComponents tc;
	char *p;
	time_t ntpTime;

	ntpTime=(time_t)TIME_GetCurrentTime();
	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	tc=calculateComponents((uint32_t)ntpTime);

	int scroll = MAX72XXSingle_GetScrollCount();
	//scroll_cycle = 0;
	if (scroll == 0 && g_del == 0) {
		time[0] = 0;
		p = time;
		p = my_strcat(p, "  ");

		p = add_padded(p, tc.hour);
		p = my_strcat(p, ":");
		p = add_padded(p, tc.minute);
		strcat(p, " ");

		p = my_strcat(p, " ");
		p = add_padded(p, tc.year);
		p = my_strcat(p, ".");
		p = add_padded(p, tc.month);
		p = my_strcat(p, ".");
		p = add_padded(p, tc.day);
		strcat(p, "   ");

		CMD_ExecuteCommandArgs("MAX72XX_Clear", NULL, 0);
		CMD_ExecuteCommandArgs("MAX72XX_Print", time, 0);
		CMD_ExecuteCommandArgs("MAX72XX_refresh", "", 0);
		g_del = 10;
	}
	if (g_del > 0) {
		g_del--;
		return;
	}
	CMD_ExecuteCommandArgs("MAX72XX_Scroll", "-1", 0);
	CMD_ExecuteCommandArgs("MAX72XX_refresh", "", 0);
}
bool g_animated = false;
void DRV_MAX72XX_Clock_OnEverySecond() {
	if (g_animated == false) {
		Run_NoAnimation();
	}
	ADDLOG_INFO(LOG_FEATURE_RAW, "MAX72xx_clock - DRV_MAX72XX_Clock_OnEverySecond() -- g_QuickTickCount=%i  (g_TimeSentCount=%i)\n",g_QuickTickCount, g_TimeSentCount);
}

void DRV_MAX72XX_Clock_RunFrame() {
	if (g_animated) {
		Run_Animated();
	}
	if (g_bdisptime && ! (g_QuickTickCount++ % 3) ){
		Clock_Send(CLOCK_TIME);
	}
}
/*
Config for my clock with IR

startDriver MAX72XX
MAX72XX_Setup 0 1 26
startDriver NTP
startDriver MAX72XX_Clock
ntp_timeZoneOfs 2

alias my_ledStrip_off SendGet http://192.168.0.200/cm?cmnd=POWER%20OFF
alias my_ledStrip_plus SendGet http://192.168.0.200/cm?cmnd=backlog%20POWER%20ON;add_dimmer%2010
alias my_ledStrip_minus SendGet http://192.168.0.200/cm?cmnd=backlog%20POWER%20ON;add_dimmer%20-10
alias main_switch_off SendGet http://192.168.0.159/cm?cmnd=POWER%20OFF
alias main_switch_on SendGet http://192.168.0.159/cm?cmnd=POWER%20ON

alias my_send_style_cold SendGet http://192.168.0.210/cm?cmnd=backlog%20CT%20153;%20POWER%20ON
alias my_send_style_warm SendGet http://192.168.0.210/cm?cmnd=backlog%20CT%20500%20POWER%20ON
alias my_send_style_red SendGet http://192.168.0.210/cm?cmnd=backlog%20color%20FF0000%20POWER%20ON
alias my_send_style_green SendGet http://192.168.0.210/cm?cmnd=backlog%20color%2000FF00%20POWER%20ON
alias my_send_style_blue SendGet http://192.168.0.210/cm?cmnd=backlog%20color%200000FF%20POWER%20ON
alias my_send_style_plus SendGet http://192.168.0.210/cm?cmnd=backlog%20POWER%20ON;add_dimmer%2010
alias my_send_style_minus SendGet http://192.168.0.210/cm?cmnd=backlog%20POWER%20ON;add_dimmer%20-10

// 1
addEventHandler2 IR_Samsung 0x707 0x4 my_ledStrip_off
// 2
addEventHandler2 IR_Samsung 0x707 0x5 my_ledStrip_plus
// 3
addEventHandler2 IR_Samsung 0x707 0x6 my_ledStrip_minus
// 4
addEventHandler2 IR_Samsung 0x707 0x8 main_switch_off
// 5
addEventHandler2 IR_Samsung 0x707 0x9 main_switch_on
// 6
addEventHandler2 IR_Samsung 0x707 0xA startScript autoexec.bat next_light_style
// 7
addEventHandler2 IR_Samsung 0x707 0xC my_send_style_plus
// 8
addEventHandler2 IR_Samsung 0x707 0xD my_send_style_minus

// stop autoexec execution here
return

// this is function for scrolling light styles
next_light_style:

// add to ch 10, wrap within range [0,5]
addChannel 10 1 0 5 1

// start main switch
//main_switch_on

if $CH10==0 then my_send_style_cold
if $CH10==1 then my_send_style_warm
if $CH10==2 then my_send_style_red
if $CH10==3 then my_send_style_green
if $CH10==4 then my_send_style_blue






*/
static commandResult_t DRV_MAX72XX_Clock_Animate(const void *context, const char *cmd, const char *args, int flags) {


	Tokenizer_TokenizeString(args, 0);

	g_animated = Tokenizer_GetArgInteger(0);


	return CMD_RES_OK;
}
void DRV_MAX72XX_Clock_Init() {

	CMD_RegisterCommand("MAX72XXClock_Animate", DRV_MAX72XX_Clock_Animate, NULL);
}





