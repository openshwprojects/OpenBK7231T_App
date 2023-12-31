
//  Characters
#define HT16K33_0                0
#define HT16K33_1                1
#define HT16K33_2                2
#define HT16K33_3                3
#define HT16K33_4                4
#define HT16K33_5                5
#define HT16K33_6                6
#define HT16K33_7                7
#define HT16K33_8                8
#define HT16K33_9                9
#define HT16K33_A                10
#define HT16K33_B                11
#define HT16K33_C                12
#define HT16K33_D                13
#define HT16K33_E                14
#define HT16K33_F                15
#define HT16K33_SPACE            16
#define HT16K33_MINUS            17
#define HT16K33_TOP_C            18     //  c
#define HT16K33_DEGREE           19     //  °
#define HT16K33_NONE             99


//  Commands
#define HT16K33_ON              0x21  //  0 = off   1 = on
#define HT16K33_STANDBY         0x20  //  bit xxxxxxx0

//  bit pattern 1000 0xxy
//  y    =  display on / off
//  xx   =  00=off     01=2Hz     10 = 1Hz     11 = 0.5Hz
#define HT16K33_DISPLAYON       0x81
#define HT16K33_DISPLAYOFF      0x80
#define HT16K33_BLINKON0_5HZ    0x87
#define HT16K33_BLINKON1HZ      0x85
#define HT16K33_BLINKON2HZ      0x83
#define HT16K33_BLINKOFF        0x81

//  bit pattern 1110 xxxx
//  xxxx    =  0000 .. 1111 (0 - F)
#define HT16K33_BRIGHTNESS      0xE0
