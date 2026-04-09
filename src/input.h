/*
 * input.h — Joystick and button input public interface.
 *
 * The Adafruit analog joystick #512 provides two analog axes (X, Y)
 * and a normally-open pushbutton (active-low with internal pull-up).
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>

/* Normalised joystick direction after deadzone processing. */
typedef enum {
    JOY_NONE  = 0,
    JOY_LEFT  = 1,
    JOY_RIGHT = 2,
    JOY_UP    = 3,
    JOY_DOWN  = 4
} joy_dir_t;

/* Snapshot of the current input state. */
typedef struct {
    joy_dir_t direction;      /* dominant axis after deadzone            */
    bool      button_pressed; /* true on the *edge* press this poll     */
    bool      button_held;    /* true while the button is held down     */
    int16_t   raw_x;          /* 0 ... 4095 straight from ADC           */
    int16_t   raw_y;          /* 0 ... 4095 straight from ADC           */
} input_state_t;

/* Initialise ADC channels and button GPIO. */
void input_init(void);

/* Poll joystick + button.  Call once per frame (~30-60 Hz).
 * Handles debounce internally. */
input_state_t input_read(void);

/* Print joystick/button values to stdio for debugging. */
void input_test(void);

#endif /* INPUT_H */
