#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t raw_x;
    uint16_t raw_y;
    float norm_x; // -1.0 .. +1.0 with deadzone
    float norm_y; // -1.0 .. +1.0 with deadzone
    bool button_pressed;      // debounced level
    bool button_just_pressed; // rising-edge event (active press)
} InputState;

#define JOYSTICK_DEADZONE        0.08f
#define JOYSTICK_UP_THRESHOLD   -0.35f
#define JOYSTICK_DOWN_THRESHOLD  0.35f

void input_init(void);
void input_update(InputState* s);
void input_test(void);

#endif // INPUT_H
