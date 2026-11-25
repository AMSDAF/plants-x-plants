/* Converte teclado, mouse e controle Xbox 360 nas entradas usadas pelas telas. */
#include "input_map.h"
#include <math.h>

enum {
    PAD_CONFIRM,
    PAD_BACK,
    PAD_SECONDARY,
    PAD_EXTRA,
    PAD_START,
    PAD_SELECT,
    PAD_LEFT_BUMPER,
    PAD_RIGHT_BUMPER,
    PAD_BUTTON_COUNT
};

typedef struct {
    bool pressed[PAD_BUTTON_COUNT];
    bool down[PAD_BUTTON_COUNT];
    bool direction[INPUT_DIR_COUNT];
    bool active;
    bool combo;
} InputFrame;

static InputFrame frame;
static int previous_axis_x;
static int previous_axis_y;
static float repeat_x;
static float repeat_y;

static const float repeat_delay = 0.36f;
static const float repeat_interval = 0.12f;
static const float axis_threshold = 0.55f;

static int gamepad_index(void)
{
    for (int pad = 0; pad < 4; pad++)
        if (IsGamepadAvailable(pad)) return pad;
    return -1;
}

static int button_of(int index)
{
    static const int buttons[PAD_BUTTON_COUNT] = {
        GAMEPAD_BUTTON_RIGHT_FACE_DOWN,  /* A: confirmar */
        GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, /* B: voltar */
        GAMEPAD_BUTTON_RIGHT_FACE_LEFT,  /* X: ação secundária */
        GAMEPAD_BUTTON_RIGHT_FACE_UP,    /* Y: ação alternativa */
        GAMEPAD_BUTTON_MIDDLE_RIGHT,     /* Start */
        GAMEPAD_BUTTON_MIDDLE_LEFT,      /* Back/Select */
        GAMEPAD_BUTTON_LEFT_TRIGGER_1,   /* LB */
        GAMEPAD_BUTTON_RIGHT_TRIGGER_1   /* RB */
    };
    return buttons[index];
}

static int axis_direction(float value)
{
    if (value < -axis_threshold) return -1;
    if (value > axis_threshold) return 1;
    return 0;
}

/* Repete o analógico depois de um atraso, sem exigir estado nas telas. */
static bool axis_step(int current, int *previous, float *timer, int direction, float dt)
{
    bool pressed = false;
    if (current != *previous) {
        *timer = current == direction ? repeat_delay : 0;
        pressed = current == direction;
        *previous = current;
    } else if (current == direction && current != 0) {
        *timer -= dt;
        if (*timer <= 0) {
            pressed = true;
            *timer += repeat_interval;
        }
    }
    return pressed;
}

void input_map_begin_frame(float dt)
{
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    frame = (InputFrame){0};
    int pad = gamepad_index();
    if (pad < 0) {
        previous_axis_x = previous_axis_y = 0;
        repeat_x = repeat_y = 0;
        return;
    }

    for (int i = 0; i < PAD_BUTTON_COUNT; i++) {
        int button = button_of(i);
        frame.pressed[i] = IsGamepadButtonPressed(pad, button);
        frame.down[i] = IsGamepadButtonDown(pad, button);
        if (frame.down[i]) frame.active = true;
    }
    frame.pressed[PAD_LEFT_BUMPER] |= IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_LEFT_TRIGGER_2);
    frame.pressed[PAD_RIGHT_BUMPER] |= IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
    frame.down[PAD_LEFT_BUMPER] |= IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_TRIGGER_2);
    frame.down[PAD_RIGHT_BUMPER] |= IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);

    static const int dpad[INPUT_DIR_COUNT] = {
        GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
        GAMEPAD_BUTTON_LEFT_FACE_DOWN, GAMEPAD_BUTTON_LEFT_FACE_LEFT
    };
    for (int i = 0; i < INPUT_DIR_COUNT; i++) {
        if (IsGamepadButtonPressed(pad, dpad[i])) frame.direction[i] = true;
        if (IsGamepadButtonDown(pad, dpad[i])) frame.active = true;
    }

    float axis_x = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_X);
    float axis_y = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_Y);
    int direction_x = axis_direction(axis_x);
    int direction_y = axis_direction(axis_y);
    if (axis_step(direction_x, &previous_axis_x, &repeat_x, -1, dt))
        frame.direction[INPUT_DIR_LEFT] = true;
    if (axis_step(direction_x, &previous_axis_x, &repeat_x, 1, dt))
        frame.direction[INPUT_DIR_RIGHT] = true;
    if (axis_step(direction_y, &previous_axis_y, &repeat_y, -1, dt))
        frame.direction[INPUT_DIR_UP] = true;
    if (axis_step(direction_y, &previous_axis_y, &repeat_y, 1, dt))
        frame.direction[INPUT_DIR_DOWN] = true;
    if (direction_x || direction_y) frame.active = true;
    /* Só marca atividade com deslocamento relevante; drift do analógico não pode travar transições. */
    if (fabsf(GetGamepadAxisMovement(pad, GAMEPAD_AXIS_RIGHT_X)) > axis_threshold ||
        fabsf(GetGamepadAxisMovement(pad, GAMEPAD_AXIS_RIGHT_Y)) > axis_threshold) frame.active = true;
    /* Back+Start alterna tela cheia sem disparar as ações individuais. */
    frame.combo = frame.pressed[PAD_START] && frame.pressed[PAD_SELECT];
    if (frame.combo) {
        frame.pressed[PAD_START] = false;
        frame.pressed[PAD_SELECT] = false;
        frame.down[PAD_START] = false;
        frame.down[PAD_SELECT] = false;
    }
}

bool input_gamepad_active(void)
{
    return frame.active;
}

bool input_confirm_pressed(void)
{
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || frame.pressed[PAD_CONFIRM];
}

bool input_primary_pressed(void)
{
    return IsKeyPressed(KEY_ENTER) || frame.pressed[PAD_CONFIRM];
}

bool input_confirm_down(void)
{
    return IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_SPACE) || frame.down[PAD_CONFIRM];
}

bool input_back_pressed(void)
{
    return IsKeyPressed(KEY_ESCAPE) || frame.pressed[PAD_BACK];
}

bool input_gamepad_back_pressed(void)
{
    return frame.pressed[PAD_BACK];
}

bool input_gamepad_back_down(void)
{
    return frame.down[PAD_BACK];
}

bool input_cancel_pressed(void)
{
    return IsKeyPressed(KEY_BACKSPACE) || frame.pressed[PAD_BACK];
}

bool input_secondary_pressed(void)
{
    return IsKeyPressed(KEY_TAB) || frame.pressed[PAD_SECONDARY];
}

bool input_gamepad_secondary_pressed(void)
{
    return frame.pressed[PAD_SECONDARY];
}

bool input_extra_pressed(void)
{
    return frame.pressed[PAD_EXTRA];
}

bool input_interact_pressed(void)
{
    return IsKeyPressed(KEY_E) || input_confirm_pressed();
}

bool input_start_pressed(void)
{
    return frame.pressed[PAD_START];
}

bool input_select_pressed(void)
{
    return frame.pressed[PAD_SELECT];
}

bool input_fullscreen_toggle_pressed(void)
{
    return frame.combo;
}

bool input_left_bumper_pressed(void)
{
    return frame.pressed[PAD_LEFT_BUMPER];
}

bool input_right_bumper_pressed(void)
{
    return frame.pressed[PAD_RIGHT_BUMPER];
}

bool input_left_trigger_pressed(void)
{
    int pad = gamepad_index();
    return pad >= 0 && (IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_LEFT_TRIGGER_1) ||
                        IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_LEFT_TRIGGER_2) ||
                        GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_TRIGGER) > 0.5f);
}

bool input_right_trigger_pressed(void)
{
    int pad = gamepad_index();
    return pad >= 0 && (IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) ||
                        IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
                        GetGamepadAxisMovement(pad, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.5f);
}

bool input_shoulders_pressed(void)
{
    int pad = gamepad_index();
    return pad >= 0 && IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_TRIGGER_1) &&
           IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
}

bool input_direction_pressed(InputDirection direction)
{
    switch (direction) {
        case INPUT_DIR_UP: return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || frame.direction[direction];
        case INPUT_DIR_RIGHT: return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || frame.direction[direction];
        case INPUT_DIR_DOWN: return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || frame.direction[direction];
        case INPUT_DIR_LEFT: return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || frame.direction[direction];
        default: return false;
    }
}

bool input_horizontal_pressed(void)
{
    return input_direction_pressed(INPUT_DIR_LEFT) || input_direction_pressed(INPUT_DIR_RIGHT);
}

bool input_vertical_pressed(void)
{
    return input_direction_pressed(INPUT_DIR_UP) || input_direction_pressed(INPUT_DIR_DOWN);
}

bool input_any_down(void)
{
    for (int key = 32; key <= 348; key++)
        if (IsKeyDown(key)) return true;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return true;
    return frame.active;
}

Vector2 input_move_axis(void)
{
    Vector2 axis = {0, 0};
    int pad = gamepad_index();
    if (pad >= 0) {
        axis.x = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_X);
        axis.y = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_Y);
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) axis.x -= 1;
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) axis.x += 1;
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_UP)) axis.y -= 1;
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) axis.y += 1;
    }
    axis.x += (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
    axis.y += (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) - (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));
    if (axis.x > 1) axis.x = 1;
    if (axis.x < -1) axis.x = -1;
    if (axis.y > 1) axis.y = 1;
    if (axis.y < -1) axis.y = -1;
    return axis;
}

bool input_running_down(void)
{
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) return true;
    int pad = gamepad_index();
    return pad >= 0 && (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
                        GetGamepadAxisMovement(pad, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.4f);
}
