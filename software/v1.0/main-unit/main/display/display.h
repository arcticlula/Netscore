#pragma once

#include "math.h"
#include "display_init.h"
#include "misc.h"

const uint8_t numbers[11] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111,  // 9
    0
};

const uint8_t letters[28] = {
    0,
    0b01110111, // A
    0b01111100, // b
    0b01111111, // B
    0b00111001, // C
    0b01011110, // d
    0b01111001, // E
    0b01110001, // F
    0b00111101, // G
    0b01110110, // H
    0b00000110, // I
    0b00011110, // J
    0b01110110, // K: approximated as H
    0b00111000, // L
    0b00010101, // M: approximated
    0b00110111, // N
    0b00111111, // O
    0b01110011, // P
    0b01100111, // Q
    0b01010000, // r
    0b01101101, // S
    0b01111000, // T
    0b00111110, // U
    0b00111110, // V: approximated as U
    0b00011101, // W: approximated
    0b01110110, // X: approximated as H
    0b01101110, // Y
    0b01011011  // Z
};

enum { BLANK = 0, A, b, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };

void show_display();

void show_boot();
void show_boot_2();
void show_boot_3();
void show_boot_4();

void show_press();
void show_sport();
void show_sport_ping_pong();

void show_menu();
void show_menu_transition();

void show_set_max_score();
void show_set_padel_game_type();
void show_set_deuce_type();
void show_play();
void show_play_default();
void show_play_padel();
void show_play_padel_sets(uint8_t team);
void show_set_win(uint8_t team);
void show_set_lost(uint8_t team);
void show_brightness();
void show_battery();

void show_test();

void show_off();
void show_off_2();