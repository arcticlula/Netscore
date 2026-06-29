#pragma once

#include <cstdint>

const uint8_t numbers[11] = {
    0b00111111,  // 0
    0b00000110,  // 1
    0b01011011,  // 2
    0b01001111,  // 3
    0b01100110,  // 4
    0b01101101,  // 5
    0b01111101,  // 6
    0b00000111,  // 7
    0b01111111,  // 8
    0b01101111,  // 9
    0};

const uint8_t letters[37] = {
    0,
    0b01110111,  // A
    0b01111100,  // b
    0b01111111,  // B
    0b01011000,  // c
    0b00111001,  // C
    0b01011110,  // d
    0b00111111,  // D
    0b01111001,  // E
    0b01110001,  // F
    0b00111101,  // G
    0b01110100,  // h
    0b01110110,  // H
    0b00000100,  // i
    0b00000110,  // I
    0b00011110,  // J
    0b01110110,  // K: approximated as H
    0b00111000,  // L
    0b00010101,  // M: approximated
    0b01010100,  // n
    0b00110111,  // N
    0b01011100,  // o
    0b00111111,  // O
    0b01110011,  // P
    0b01100111,  // Q
    0b01010000,  // r
    0b00100001,  // r2 top r
    0b00110001,  // R
    0b01101101,  // S
    0b01111000,  // t
    0b00011100,  // u
    0b00111110,  // U
    0b00111110,  // V: approximated as U
    0b00011101,  // W: approximated
    0b01110110,  // X: approximated as H
    0b01101110,  // Y
    0b01011011   // Z
};

const uint8_t symbols[8] = {
    0,           // blank
    0b00001000,  // _
    0b01001000,  // =
    0b01001001,  // ≡
    0b01000000,  // -
    0b00111001,  // [
    0b00001111,  // ]
    0b11010010   // %
};

enum { BLANK = 0,
       A,
       b,
       B,
       c,
       C,
       d,
       D,
       E,
       F,
       G,
       h,
       H,
       i,
       I,
       J,
       K,
       L,
       M,
       n,
       N,
       o,
       O,
       P,
       Q,
       r,
       r2,
       R,
       S,
       t,
       u,
       U,
       V,
       W,
       X,
       Y,
       Z };

enum { BLANK_SYM = 0,
       UNDERSCORE,
       EQUAL,
       THREE_EQUAL,
       DASH,
       BRACKET_LEFT,
       BRACKET_RIGHT,
       PERC_SYM };

extern volatile bool inf_patern_a;
extern volatile bool inf_patern_b;

// ==========================================
// CORE & SYSTEM SCREENS
// ==========================================
void show_display();
void show_transition();
void show_oops();
void show_time();
void show_game_time();
void show_boot();
void show_connecting();
void show_brightness();
void show_brightness_overlay();
void show_volume_overlay();
void show_battery();
void show_device_battery();
void show_clock();
void show_off();
void show_off_2();
void show_sleep();

// ==========================================
// MENU NAVIGATION
// ==========================================
void show_menu();
void show_menu_transition();

// ==========================================
// SPORT CONFIGURATION
// ==========================================
void show_sport();
void show_sport_ping_pong();
void show_sport_football();
void show_set_sport_mode();
void show_set_sport_mode_volley();
void show_set_sport_mode_padel();
void show_set_max_score();
void show_set_padel_game_type();
void show_set_deuce_type();

// ==========================================
// PLAY & PRACTICE
// ==========================================

void show_play_serve_select();
void show_play();
void show_play_menu();
void show_play_menu_painel();
void show_play_volley();
void show_play_tennis();
void show_play_football();
void show_play_ping_pong();
void show_play_basketball();
void show_play_result();
void show_play_result_default();
void show_play_result_padel();
void show_sets(uint8_t side);
void show_practice_transition_scr();

// ==========================================
// TEST SCREENS
// ==========================================
void show_test_menu();
void show_test_counter();
void show_test_all();
void show_test_bomb();
