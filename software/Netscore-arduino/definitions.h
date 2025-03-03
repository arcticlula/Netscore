#define OFF 10

#define MAX_BRIGHT 63
#define BRIGHT_INDEX 2
#define MAX_BRIGHT_INDEX 6
#define MAX_VALUE 4095
#define MUX_NUM 3

typedef void (*timer_callback_t)(void *arg);
typedef void (*callback_t)();

typedef struct {
    uint8_t home_points;
    uint8_t away_points;
    uint8_t home_sets;
    uint8_t away_sets;
} Score;

typedef struct {
  uint8_t character;
  uint8_t size;
  uint8_t positions[8];
} DigitCharacter;

typedef struct {
    DigitCharacter c;
    int8_t direction;
    double value;
    double min;
    double max;
    double background;
    uint32_t time_ms;
} DigitWave;

typedef struct {
    DigitCharacter c;
    int8_t channel;  
    int8_t direction;
    double min;
    double max;
    double background;
    uint32_t time_ms;
    uint16_t cnt = 0;
} DigitLoop;

typedef struct {
    DigitCharacter c;
    int8_t channel;  
    int8_t direction;
    double min;
    double max;
    double background;
    uint32_t time_ms;
    uint16_t cnt = 0;
} DigitZigzag;

typedef struct {
    DigitCharacter c;
    int8_t channel;  
    int8_t direction;
    double value;
    uint32_t time_ms;
    uint16_t cnt = 0;
    double positions_value[8];
} DigitFade;

typedef struct {
    DigitCharacter c1;
    DigitCharacter c2;
    double value;
    uint32_t time_ms;
    uint16_t cnt = 0;
    double positions_dir[8];
    double positions_value[8];
} DigitFadeInto;

const uint8_t digits[11] = {
    0b00111111, // 0: segments a, b, c, d, e, f
    0b00000110, // 1: segments b, c
    0b01011011, // 2: segments a, b, d, e, g
    0b01001111, // 3: segments a, b, c, d, g
    0b01100110, // 4: segments b, c, f, g
    0b01101101, // 5: segments a, c, d, f, g
    0b01111101, // 6: segments a, c, d, e, f, g
    0b00000111, // 7: segments a, b, c
    0b01111111, // 8: segments a, b, c, d, e, f, g
    0b01101111,  // 9: segments a, b, c, d, f, g
    0
};

const uint8_t letters[27] = {
    0,
    0b01110111, // A: segments a, b, c, e, f, g
    0b01111111, // B: segments c, d, e, f, g (like 'b' in lowercase)
    0b00111001, // C: segments a, d, e, f
    0b01011110, // D: segments b, c, d, e, g (like 'd' in lowercase)
    0b01111001, // E: segments a, d, e, f, g
    0b01110001, // F: segments a, e, f, g
    0b00111101, // G: segments a, c, d, e, f
    0b01110110, // H: segments b, c, e, f, g
    0b00000110, // I: segments b, c
    0b00011110, // J: segments b, c, d, e
    0b01110110, // K: approximated as H (7-segment can't distinguish)
    0b00111000, // L: segments d, e, f
    0b00010101, // M: approximated (7-segment can't display a proper M)
    0b01010100, // N: approximated (7-segment can't display a proper N)
    0b00111111, // O: segments a, b, c, d, e, f
    0b01110011, // P: segments a, b, e, f, g
    0b01100111, // Q: approximated as 'A' with g turned on
    0b01010000, // R: 'r' in lowercase
    0b01101101, // S: segments a, c, d, f, g
    0b01111000, // T: segments d, e, f, g
    0b00111110, // U: segments b, c, d, e, f
    0b00111110, // V: approximated as U (7-segment can't distinguish)
    0b00011101, // W: approximated (7-segment can't display a proper W)
    0b01110110, // X: approximated as H
    0b01101110, // Y: segments b, c, d, f, g
    0b01011011  // Z: segments a, b, d, e, g
};

uint8_t bitCountLUT[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

enum {
  BOOT_SCR = 0,
  BOOT2_SCR,
  BOOT3_SCR,
  BOOT4_SCR,
  MENU_SCR,
  SET_SIZE_SCR,
  PLAY_SCR,
  BRILHO_SCR,
  TEST_SCR,
  TEST2_SCR,
  OFF_SCR,
};

enum {
  MENU_PLAY = 0,
  MENU_BRILHO,
  MENU_TEST,
  MENU_OFF
};

enum {
  A = 1,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z
};


enum {
  NONE = 0,
  HOME,
  AWAY
};

enum {
  HOME_WIN = 0,
  AWAY_WIN,
  UNDO
};