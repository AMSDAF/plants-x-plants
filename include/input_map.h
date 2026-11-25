/* Mapeia teclado, mouse e controle Xbox 360 para ações comuns da interface. */
#ifndef INPUT_MAP_H
#define INPUT_MAP_H

#include <stdbool.h>
#include "raylib.h"

typedef enum {
    INPUT_DIR_UP,
    INPUT_DIR_RIGHT,
    INPUT_DIR_DOWN,
    INPUT_DIR_LEFT,
    INPUT_DIR_COUNT
} InputDirection;

/* Atualiza o estado do controle uma vez por quadro, antes das telas. */
void input_map_begin_frame(float dt);

bool input_gamepad_active(void);
bool input_confirm_pressed(void);
bool input_primary_pressed(void);
bool input_confirm_down(void);
bool input_back_pressed(void);
bool input_gamepad_back_pressed(void);
bool input_gamepad_back_down(void);
bool input_cancel_pressed(void);
bool input_secondary_pressed(void);
bool input_gamepad_secondary_pressed(void);
bool input_extra_pressed(void);
bool input_interact_pressed(void);
bool input_start_pressed(void);
bool input_select_pressed(void);
bool input_fullscreen_toggle_pressed(void);
bool input_left_bumper_pressed(void);
bool input_right_bumper_pressed(void);
bool input_left_trigger_pressed(void);
bool input_right_trigger_pressed(void);
bool input_shoulders_pressed(void);
bool input_direction_pressed(InputDirection direction);
bool input_horizontal_pressed(void);
bool input_vertical_pressed(void);
bool input_any_down(void);
Vector2 input_move_axis(void);
bool input_running_down(void);

#endif /* INPUT_MAP_H */
