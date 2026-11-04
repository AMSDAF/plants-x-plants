/*
 * menu_screen.c
 * Carrega e desenha os assets da tela inicial e do menu.
 * Processa teclado e mouse e informa quando o jogador pede para sair.
 */

#include "menu_screen.h"

#define MENU_OPTION_COUNT 3

typedef enum {
    BUTTON_NORMAL,
    BUTTON_SELECTED,
    BUTTON_PRESSED,
    BUTTON_DISABLED
} ButtonState;

/* Recortes em pixels da sprite sheet original de 1254 x 1254. */
static const Rectangle BUTTON_SOURCES[] = {
    {70, 60, 1116, 254},
    {70, 367, 1116, 254},
    {70, 677, 1116, 254},
    {70, 978, 1116, 254}
};

static const char *OPTION_LABELS[] = {
    "Campanha",
    "Versus local",
    "Sair"
};

/* Os modos serao habilitados quando estiverem implementados. */
static bool option_is_enabled(int option)
{
    return option == 2;
}

static Rectangle get_button_bounds(int option)
{
    return (Rectangle){
        (GetScreenWidth() - 400.0f) / 2.0f,
        385.0f + option * 96.0f,
        400.0f,
        91.0f
    };
}

static void draw_centered_text(
    const char *text,
    int y,
    int font_size,
    Color color
)
{
    int width = MeasureText(text, font_size);
    int x = (GetScreenWidth() - width) / 2;

    DrawText(text, x + 2, y + 2, font_size, BLACK);
    DrawText(text, x, y, font_size, color);
}

static void draw_background(Texture2D texture)
{
    if (texture.id == 0) {
        return;
    }

    float scale_x = (float)GetScreenWidth() / texture.width;
    float scale_y = (float)GetScreenHeight() / texture.height;
    float scale = scale_x > scale_y ? scale_x : scale_y;

    float width = texture.width * scale;
    float height = texture.height * scale;

    Rectangle source = {
        0, 0, (float)texture.width, (float)texture.height
    };

    Rectangle destination = {
        (GetScreenWidth() - width) / 2.0f,
        (GetScreenHeight() - height) / 2.0f,
        width,
        height
    };

    DrawTexturePro(
        texture, source, destination, (Vector2){0, 0}, 0, WHITE
    );
}

static void draw_logo(Texture2D texture, bool menu_open)
{
    if (texture.id == 0) {
        draw_centered_text("Plants x Plants", 150, 48, RAYWHITE);
        return;
    }

    float height = menu_open ? 325.0f : 440.0f;
    float width = height * texture.width / texture.height;

    Rectangle source = {
        0, 0, (float)texture.width, (float)texture.height
    };

    Rectangle destination = {
        (GetScreenWidth() - width) / 2.0f,
        menu_open ? 35.0f : 65.0f,
        width,
        height
    };

    DrawTexturePro(
        texture, source, destination, (Vector2){0, 0}, 0, WHITE
    );
}

void menu_screen_init(MenuScreen *screen)
{
    *screen = (MenuScreen){0};

    screen->background = LoadTexture(
        "assets/sprites/menu_background.png"
    );

    screen->logo = LoadTexture("assets/sprites/logo.png");
    screen->buttons = LoadTexture("assets/sprites/menu_buttons.png");

    screen->selected_option = 2;
    screen->pressed_option = -1;

    if (screen->background.id != 0) {
        SetTextureFilter(screen->background, TEXTURE_FILTER_POINT);
    }

    if (screen->logo.id != 0) {
        SetTextureFilter(screen->logo, TEXTURE_FILTER_POINT);
    }

    if (screen->buttons.id != 0) {
        SetTextureFilter(screen->buttons, TEXTURE_FILTER_POINT);
    }
}

/* Retorna true apenas quando houver uma solicitacao de saida. */
bool menu_screen_update(MenuScreen *screen)
{
    screen->elapsed_time += GetFrameTime();

    if (!screen->menu_open) {
        if (IsKeyPressed(KEY_ENTER)) {
            screen->menu_open = true;
            screen->pressed_option = -1;
        }

        return false;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        screen->menu_open = false;
        screen->pressed_option = -1;
        return false;
    }

    int direction = 0;

    if (IsKeyPressed(KEY_DOWN)) {
        direction = 1;
    } else if (IsKeyPressed(KEY_UP)) {
        direction = -1;
    }

    if (direction != 0) {
        do {
            screen->selected_option =
                (screen->selected_option + direction
                 + MENU_OPTION_COUNT) % MENU_OPTION_COUNT;
        } while (!option_is_enabled(screen->selected_option));
    }

    Vector2 mouse = GetMousePosition();
    Vector2 mouse_delta = GetMouseDelta();
    bool mouse_moved = mouse_delta.x != 0 || mouse_delta.y != 0;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        screen->pressed_option = -1;
    }

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        if (!option_is_enabled(i)) {
            continue;
        }

        bool hovered = CheckCollisionPointRec(
            mouse, get_button_bounds(i)
        );

        if (hovered && mouse_moved) {
            screen->selected_option = i;
        }

        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            screen->selected_option = i;
            screen->pressed_option = i;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        int pressed = screen->pressed_option;
        screen->pressed_option = -1;

        if (pressed >= 0
            && option_is_enabled(pressed)
            && CheckCollisionPointRec(mouse, get_button_bounds(pressed))) {
            return pressed == 2;
        }
    }

    if (IsKeyPressed(KEY_ENTER)
        && option_is_enabled(screen->selected_option)) {
        return screen->selected_option == 2;
    }

    return false;
}

void menu_screen_draw(const MenuScreen *screen)
{
    ClearBackground((Color){24, 32, 28, 255});
    draw_background(screen->background);

    DrawRectangle(
        0, 0,
        GetScreenWidth(), GetScreenHeight(),
        Fade(BLACK, 0.18f)
    );

    draw_logo(screen->logo, screen->menu_open);

    if (!screen->menu_open) {
        if ((int)(screen->elapsed_time * 2.0f) % 2 == 0) {
            draw_centered_text(
                "Pressione ENTER", 565, 28, RAYWHITE
            );
        }

        return;
    }

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        Rectangle bounds = get_button_bounds(i);
        ButtonState state = BUTTON_NORMAL;

        if (!option_is_enabled(i)) {
            state = BUTTON_DISABLED;
        } else if (screen->pressed_option == i
                   && IsMouseButtonDown(MOUSE_BUTTON_LEFT)
                   && CheckCollisionPointRec(GetMousePosition(), bounds)) {
            state = BUTTON_PRESSED;
        } else if (screen->selected_option == i) {
            state = BUTTON_SELECTED;
        }

        if (screen->buttons.id != 0) {
            DrawTexturePro(
                screen->buttons,
                BUTTON_SOURCES[state],
                bounds,
                (Vector2){0, 0},
                0,
                WHITE
            );
        } else {
            DrawRectangleRec(bounds, DARKBROWN);
        }

        Color text_color = option_is_enabled(i)
            ? RAYWHITE : LIGHTGRAY;

        int font_size = 25;
        int text_width = MeasureText(OPTION_LABELS[i], font_size);
        int x = (int)(bounds.x + (bounds.width - text_width) / 2);
        int y = (int)(bounds.y + (bounds.height - font_size) / 2);

        DrawText(OPTION_LABELS[i], x + 2, y + 2, font_size, BLACK);
        DrawText(OPTION_LABELS[i], x, y, font_size, text_color);
    }

    draw_centered_text(
        "Modos em desenvolvimento | ESC: voltar",
        685, 18, RAYWHITE
    );
}

void menu_screen_shutdown(MenuScreen *screen)
{
    if (screen->background.id != 0) {
        UnloadTexture(screen->background);
    }

    if (screen->logo.id != 0) {
        UnloadTexture(screen->logo);
    }

    if (screen->buttons.id != 0) {
        UnloadTexture(screen->buttons);
    }

    *screen = (MenuScreen){0};
}