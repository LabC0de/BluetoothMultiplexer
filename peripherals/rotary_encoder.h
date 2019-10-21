#include "driver/gpio.h"

extern uint64_t ROTARY_POSITION;
extern bool BUTTON;

/**
 * Sets up the rotary encoder
 * param clk: pin for rotary output a
 * param dt: pin for rotary output b
 * param sw: pin for integrated button
 */
void init_rotary_encoder(int clk, int dt, int sw);