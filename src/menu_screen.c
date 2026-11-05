/*
 * menu_screen.c
 * Carrega e desenha os assets da tela inicial e do menu.
 * Anima o logo, a transicao entre as telas, as folhas e os ramos dos cantos.
 * Processa teclado e mouse e informa quando o jogador pede para sair.
 */

#include "menu_screen.h"
#include "menu_wind_test_data.h"

#include <math.h>

/* Experimento reversivel: 0 restaura o fundo e os ramos originais. */
#ifndef MENU_WIND_TEST_ENABLED
#define MENU_WIND_TEST_ENABLED 1
#endif

#define BUTTON_FEEDBACK_DURATION 0.12f
#define BUTTON_SELECTED_SCALE 1.03f
#define BUTTON_PRESSED_SCALE 0.98f
#define BUTTON_PRESSED_OFFSET 2.0f

/* Duracao, em segundos, da transicao entre tela inicial e menu. */
#define TRANSITION_DURATION 0.45f

/* Maior intervalo de tempo aceito por quadro, evitando saltos. */
#define MAX_FRAME_TIME 0.1f

/* Posicoes e alturas do logo na tela inicial e no menu. */
#define LOGO_TITLE_Y 65.0f
#define LOGO_TITLE_HEIGHT 440.0f
#define LOGO_MENU_Y 35.0f
#define LOGO_MENU_HEIGHT 325.0f

/* Flutuacao vertical do logo. */
#define LOGO_FLOAT_AMPLITUDE 4.0f
#define LOGO_FLOAT_SPEED 1.8f

/* Cada botao comeca um pouco depois do anterior. */
#define BUTTON_STAGGER 0.15f
#define BUTTON_ANIMATION_SHARE 0.7f
#define BUTTON_SLIDE_DISTANCE 24.0f

#define PROMPT_PULSE_SPEED 2.6f

#define LEAF_SWAY_SPEED 1.3f
#define LEAF_SCREEN_MARGIN 30.0f

/* Tamanho e fixacao dos ramos, um pouco alem dos cantos superiores. */
#define FOLIAGE_WIDTH 330.0f
#define FOLIAGE_ATTACHMENT_OUTSET_X 12.0f
#define FOLIAGE_ATTACHMENT_Y -10.0f

/* Amplitude em graus; velocidades em radianos por segundo. */
#define FOLIAGE_SWAY_AMPLITUDE 1.5f
#define FOLIAGE_LEFT_SWAY_SPEED 0.65f
#define FOLIAGE_RIGHT_SWAY_SPEED 0.58f
#define FOLIAGE_LEFT_SWAY_PHASE 0.0f
#define FOLIAGE_RIGHT_SWAY_PHASE 1.7f

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

/* Tons de oliva e cobre usados nas folhas. */
static const Color LEAF_COLORS[] = {
    {112, 116, 62, 255},
    {134, 132, 76, 255},
    {164, 100, 58, 255},
    {138, 84, 48, 255}
};

#define LEAF_COLOR_COUNT (int)(sizeof(LEAF_COLORS) / sizeof(LEAF_COLORS[0]))

/* Os modos serao habilitados quando estiverem implementados. */
static bool option_is_enabled(int option)
{
    return option == 2;
}

static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }

    if (value > 1.0f) {
        return 1.0f;
    }

    return value;
}

static float lerp(float start, float end, float amount)
{
    return start + (end - start) * amount;
}

/* Suaviza o inicio e o fim de um progresso entre 0 e 1. */
static float ease_in_out(float t)
{
    t = clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

/* Gera um valor decimal aleatorio entre min e max. */
static float random_float(float min, float max)
{
    return min + (max - min) * GetRandomValue(0, 1000) / 1000.0f;
}

/* Progresso suavizado de um botao, considerando seu atraso. */
static float get_button_progress(const MenuScreen *screen, int option)
{
    float start = option * BUTTON_STAGGER;
    float local = (screen->transition - start) / BUTTON_ANIMATION_SHARE;

    return ease_in_out(local);
}

/* Area de layout, sem o feedback visual, usada nos testes de clique. */
static Rectangle get_button_bounds(const MenuScreen *screen, int option)
{
    float progress = get_button_progress(screen, option);
    float slide = roundf((1.0f - progress) * BUTTON_SLIDE_DISTANCE);

    return (Rectangle){
        (GetScreenWidth() - 400.0f) / 2.0f,
        385.0f + option * 96.0f + slide,
        400.0f,
        91.0f
    };
}

static float get_logo_float_offset(const MenuScreen *screen)
{
    return roundf(
        sinf(screen->elapsed_time * LOGO_FLOAT_SPEED)
        * LOGO_FLOAT_AMPLITUDE
    );
}

/* Coloca uma folha em posicao e movimento aleatorios. */
static void reset_leaf(MenuLeaf *leaf, bool anywhere_on_screen)
{
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();

    if (anywhere_on_screen) {
        leaf->position.x = random_float(0.0f, width);
        leaf->position.y = random_float(0.0f, height);
    } else {
        /* Comeca acima da tela, inclusive a esquerda, por causa da diagonal. */
        leaf->position.x = random_float(-0.3f * width, width);
        leaf->position.y = random_float(-80.0f, -LEAF_SCREEN_MARGIN);
    }

    leaf->velocity.x = random_float(14.0f, 32.0f);
    leaf->velocity.y = random_float(28.0f, 50.0f);
    leaf->sway_phase = random_float(0.0f, 2.0f * PI);
    leaf->sway_amplitude = random_float(6.0f, 14.0f);
    leaf->rotation = random_float(0.0f, 360.0f);
    leaf->rotation_speed = random_float(20.0f, 60.0f);

    if (GetRandomValue(0, 1) == 0) {
        leaf->rotation_speed = -leaf->rotation_speed;
    }

    leaf->size = random_float(5.0f, 9.0f);
    leaf->color_index = GetRandomValue(0, LEAF_COLOR_COUNT - 1);
}

static void update_leaves(MenuScreen *screen, float delta_time)
{
    float max_x = GetScreenWidth() + LEAF_SCREEN_MARGIN;
    float max_y = GetScreenHeight() + LEAF_SCREEN_MARGIN;

    for (int i = 0; i < MENU_LEAF_COUNT; i++) {
        MenuLeaf *leaf = &screen->leaves[i];

        leaf->position.x += leaf->velocity.x * delta_time;
        leaf->position.y += leaf->velocity.y * delta_time;
        leaf->rotation += leaf->rotation_speed * delta_time;

        if (leaf->rotation >= 360.0f) {
            leaf->rotation -= 360.0f;
        } else if (leaf->rotation < 0.0f) {
            leaf->rotation += 360.0f;
        }

        if (leaf->position.x > max_x || leaf->position.y > max_y) {
            reset_leaf(leaf, false);
        }
    }
}

/* Avanca a transicao em direcao ao menu aberto ou a tela inicial. */
static void update_transition(MenuScreen *screen, float delta_time)
{
    float step = delta_time / TRANSITION_DURATION;

    if (screen->menu_open) {
        screen->transition = clamp01(screen->transition + step);
    } else {
        screen->transition = clamp01(screen->transition - step);
    }
}

/* Cancela entradas pendentes sem interromper a interpolacao visual. */
static void clear_button_activation(MenuScreen *screen)
{
    screen->pressed_option = -1;
    screen->pending_option = -1;
    screen->activation_time = 0.0f;

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        screen->button_animations[i].pressed = false;
    }
}

static void update_button_animations(MenuScreen *screen, float delta_time)
{
    /* Atinge aproximadamente 95% do alvo em 0,12 segundo, em qualquer FPS. */
    float amount = 1.0f - expf(-3.0f * delta_time / BUTTON_FEEDBACK_DURATION);
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        MenuButtonAnimation *animation = &screen->button_animations[i];
        bool enabled = option_is_enabled(i);
        bool hovered = CheckCollisionPointRec(mouse, get_button_bounds(screen, i));

        animation->focused = enabled && screen->menu_open
            && (screen->mouse_focus ? hovered : screen->selected_option == i);
        animation->pressed = enabled && screen->menu_open
            && (screen->pending_option == i
                || (screen->pressed_option == i && hovered
                    && IsMouseButtonDown(MOUSE_BUTTON_LEFT)));

        float scale = animation->pressed ? BUTTON_PRESSED_SCALE
            : (animation->focused ? BUTTON_SELECTED_SCALE : 1.0f);
        float offset = animation->pressed ? BUTTON_PRESSED_OFFSET : 0.0f;

        animation->scale = lerp(animation->scale, scale, amount);
        animation->offset_y = lerp(animation->offset_y, offset, amount);
    }
}

static void draw_centered_text(
    const char *text,
    int y,
    int font_size,
    Color color,
    float alpha
)
{
    int width = MeasureText(text, font_size);
    int x = (GetScreenWidth() - width) / 2;

    DrawText(text, x + 2, y + 2, font_size, Fade(BLACK, alpha));
    DrawText(text, x, y, font_size, Fade(color, alpha));
}

/* Transformacao compartilhada pelo fundo e pelas camadas extraidas. */
static Rectangle get_background_destination(Texture2D texture)
{
    float scale_x = (float)GetScreenWidth() / texture.width;
    float scale_y = (float)GetScreenHeight() / texture.height;
    float scale = scale_x > scale_y ? scale_x : scale_y;

    float width = texture.width * scale;
    float height = texture.height * scale;

    return (Rectangle){
        (GetScreenWidth() - width) / 2.0f,
        (GetScreenHeight() - height) / 2.0f,
        width,
        height
    };

}

static void draw_background(Texture2D texture)
{
    if (texture.id == 0) {
        return;
    }

    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    Rectangle destination = get_background_destination(texture);
    DrawTexturePro(
        texture, source, destination, (Vector2){0, 0}, 0, WHITE
    );
}

/* As coordenadas e a escala acompanham o recorte cover do fundo. */
static void draw_wind_regions(const MenuScreen *screen)
{
    Rectangle background = get_background_destination(screen->wind_background);
    float scale = background.width / screen->wind_background.width;
    for (int i = 0; i < WIND_REGION_COUNT; i++) {
        const MenuWindRegion *region = &WIND_REGIONS[i];
        Rectangle source = {
            region->atlas_origin.x
                + (screen->wind_frame % WIND_FRAME_COLUMNS) * region->frame_size.x,
            region->atlas_origin.y
                + (screen->wind_frame / WIND_FRAME_COLUMNS) * region->frame_size.y,
            region->frame_size.x, region->frame_size.y
        };
        Rectangle destination = {
            background.x + region->origin.x * scale,
            background.y + region->origin.y * scale,
            region->frame_size.x * scale, region->frame_size.y * scale
        };
        DrawTexturePro(
            screen->wind_atlas, source, destination, (Vector2){0, 0}, 0, WHITE
        );
    }
}

/* Cada ramo gira pela fixacao no canto, inclusive o recorte espelhado. */
static void draw_foliage(const MenuScreen *screen)
{
    Texture2D texture = screen->foliage;

    if (texture.id == 0) {
        return;
    }

    float height = FOLIAGE_WIDTH * texture.height / texture.width;
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    Rectangle destination = {
        -FOLIAGE_ATTACHMENT_OUTSET_X, FOLIAGE_ATTACHMENT_Y,
        FOLIAGE_WIDTH, height
    };
    float left_rotation = sinf(
        screen->elapsed_time * FOLIAGE_LEFT_SWAY_SPEED + FOLIAGE_LEFT_SWAY_PHASE
    ) * FOLIAGE_SWAY_AMPLITUDE;
    float right_rotation = sinf(
        screen->elapsed_time * FOLIAGE_RIGHT_SWAY_SPEED + FOLIAGE_RIGHT_SWAY_PHASE
    ) * FOLIAGE_SWAY_AMPLITUDE;

    DrawTexturePro(
        texture, source, destination, (Vector2){0, 0}, left_rotation, WHITE
    );

    source.width = -source.width;
    destination.x = GetScreenWidth() + FOLIAGE_ATTACHMENT_OUTSET_X;
    DrawTexturePro(
        texture, source, destination, (Vector2){FOLIAGE_WIDTH, 0},
        right_rotation, WHITE
    );
}

/* Gira um ponto do formato da folha e o leva ate a posicao na tela. */
static Vector2 leaf_point(
    Vector2 center,
    float x,
    float y,
    float angle
)
{
    float cosine = cosf(angle);
    float sine = sinf(angle);

    return (Vector2){
        center.x + x * cosine - y * sine,
        center.y + x * sine + y * cosine
    };
}

/* Desenha uma folha angular com dois triangulos e uma nervura. */
static void draw_leaf(const MenuLeaf *leaf, float elapsed_time)
{
    float sway = sinf(elapsed_time * LEAF_SWAY_SPEED + leaf->sway_phase)
        * leaf->sway_amplitude;

    Vector2 center = {
        roundf(leaf->position.x + sway),
        roundf(leaf->position.y)
    };

    float angle = leaf->rotation * DEG2RAD;
    float size = leaf->size;

    Vector2 tip = leaf_point(center, 0.0f, -size, angle);
    Vector2 left = leaf_point(center, -0.45f * size, 0.1f * size, angle);
    Vector2 base = leaf_point(center, 0.0f, 0.7f * size, angle);
    Vector2 right = leaf_point(center, 0.45f * size, 0.1f * size, angle);

    Color base_color = LEAF_COLORS[leaf->color_index];
    Color color = Fade(base_color, 0.85f);
    Color vein = Fade(ColorBrightness(base_color, -0.35f), 0.85f);

    /* A raylib espera os vertices em sentido anti-horario. */
    DrawTriangle(tip, left, base, color);
    DrawTriangle(tip, base, right, color);
    DrawLineV(tip, base, vein);
}

static void draw_logo(const MenuScreen *screen)
{
    Texture2D texture = screen->logo;
    float float_offset = get_logo_float_offset(screen);

    if (texture.id == 0) {
        draw_centered_text(
            "Plants x Plants", 150 + (int)float_offset, 48, RAYWHITE, 1.0f
        );
        return;
    }

    float progress = ease_in_out(screen->transition);
    float height = lerp(LOGO_TITLE_HEIGHT, LOGO_MENU_HEIGHT, progress);
    float width = height * texture.width / texture.height;
    float y = lerp(LOGO_TITLE_Y, LOGO_MENU_Y, progress) + float_offset;

    Rectangle source = {
        0, 0, (float)texture.width, (float)texture.height
    };

    /* Posicoes inteiras reduzem o tremido da pixel art. */
    Rectangle destination = {
        roundf((GetScreenWidth() - width) / 2.0f),
        roundf(y),
        roundf(width),
        roundf(height)
    };

    DrawTexturePro(
        texture, source, destination, (Vector2){0, 0}, 0, WHITE
    );
}

static void draw_start_prompt(const MenuScreen *screen)
{
    /* O aviso some na primeira metade da abertura do menu. */
    float visibility = 1.0f - ease_in_out(screen->transition * 2.0f);

    if (visibility <= 0.0f) {
        return;
    }

    float pulse = 0.5f + 0.5f * sinf(
        screen->elapsed_time * PROMPT_PULSE_SPEED
    );
    float alpha = (0.35f + 0.65f * pulse) * visibility;

    draw_centered_text("Pressione ENTER", 565, 28, RAYWHITE, alpha);
}

static void draw_buttons(const MenuScreen *screen)
{
    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        float alpha = get_button_progress(screen, i);

        if (alpha <= 0.0f) {
            continue;
        }

        Rectangle bounds = get_button_bounds(screen, i);
        const MenuButtonAnimation *animation = &screen->button_animations[i];
        float width = bounds.width * animation->scale;
        float height = bounds.height * animation->scale;

        bounds.x += (bounds.width - width) / 2.0f;
        bounds.y += (bounds.height - height) / 2.0f + animation->offset_y;
        bounds.width = width;
        bounds.height = height;
        ButtonState state = BUTTON_NORMAL;

        if (!option_is_enabled(i)) {
            state = BUTTON_DISABLED;
        } else if (animation->pressed) {
            state = BUTTON_PRESSED;
        } else if (animation->focused) {
            state = BUTTON_SELECTED;
        }

        if (screen->buttons.id != 0) {
            DrawTexturePro(
                screen->buttons,
                BUTTON_SOURCES[state],
                bounds,
                (Vector2){0, 0},
                0,
                Fade(WHITE, alpha)
            );
        } else {
            DrawRectangleRec(bounds, Fade(DARKBROWN, alpha));
        }

        Color text_color = option_is_enabled(i)
            ? RAYWHITE : LIGHTGRAY;

        float font_size = 25.0f * animation->scale;
        float spacing = 2.0f * animation->scale;
        Font font = GetFontDefault();
        Vector2 text_size = MeasureTextEx(font, OPTION_LABELS[i], font_size, spacing);
        Vector2 position = {
            bounds.x + (bounds.width - text_size.x) / 2.0f,
            bounds.y + (bounds.height - text_size.y) / 2.0f
        };
        Vector2 shadow = {
            position.x + 2.0f * animation->scale,
            position.y + 2.0f * animation->scale
        };

        DrawTextEx(
            font, OPTION_LABELS[i], shadow, font_size, spacing, Fade(BLACK, alpha)
        );
        DrawTextEx(
            font, OPTION_LABELS[i], position, font_size, spacing, Fade(text_color, alpha)
        );
    }
}

void menu_screen_init(MenuScreen *screen)
{
    *screen = (MenuScreen){0};

    screen->logo = LoadTexture("assets/sprites/logo.png");
    screen->buttons = LoadTexture("assets/sprites/menu_buttons.png");

    if (MENU_WIND_TEST_ENABLED) {
        screen->wind_background = LoadTexture("assets/sprites/menu_wind_test/background.png");
        screen->wind_atlas = LoadTexture("assets/sprites/menu_wind_test/canopy_atlas.png");
        screen->wind_test_ready = screen->wind_background.id != 0
            && screen->wind_background.width == WIND_SOURCE_WIDTH
            && screen->wind_background.height == WIND_SOURCE_HEIGHT
            && screen->wind_atlas.id != 0
            && screen->wind_atlas.width == WIND_ATLAS_WIDTH
            && screen->wind_atlas.height == WIND_ATLAS_HEIGHT;

        if (screen->wind_test_ready) {
            SetTextureFilter(screen->wind_background, TEXTURE_FILTER_POINT);
            SetTextureFilter(screen->wind_atlas, TEXTURE_FILTER_POINT);
        } else {
            /* Uma falha descarta o conjunto inteiro antes de restaurar a cena. */
            if (screen->wind_background.id != 0) {
                UnloadTexture(screen->wind_background);
            }
            if (screen->wind_atlas.id != 0) {
                UnloadTexture(screen->wind_atlas);
            }
            screen->wind_background = (Texture2D){0};
            screen->wind_atlas = (Texture2D){0};
        }
    }

    if (!screen->wind_test_ready) {
        screen->background = LoadTexture("assets/sprites/menu_background.png");
        screen->foliage = LoadTexture("assets/sprites/menu_foliage.png");
    }

    screen->selected_option = 2;
    clear_button_activation(screen);

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        screen->button_animations[i].scale = 1.0f;
    }

    for (int i = 0; i < MENU_LEAF_COUNT; i++) {
        reset_leaf(&screen->leaves[i], true);
    }

    if (screen->background.id != 0) {
        SetTextureFilter(screen->background, TEXTURE_FILTER_POINT);
    }

    if (screen->logo.id != 0) {
        SetTextureFilter(screen->logo, TEXTURE_FILTER_POINT);
    }

    if (screen->buttons.id != 0) {
        SetTextureFilter(screen->buttons, TEXTURE_FILTER_POINT);
    }

    if (screen->foliage.id != 0) {
        SetTextureFilter(screen->foliage, TEXTURE_FILTER_POINT);
    }
}

static bool update_menu_input(MenuScreen *screen, float delta_time)
{
    if (!screen->menu_open) {
        clear_button_activation(screen);
        /* ENTER tambem inverte uma transicao de fechamento em andamento. */
        if (IsKeyPressed(KEY_ENTER)) {
            screen->menu_open = true;
            screen->mouse_focus = false;
        }

        return false;
    }

    /* ESC pode inverter a abertura sem reiniciar o progresso. */
    if (IsKeyPressed(KEY_ESCAPE)) {
        screen->menu_open = false;
        clear_button_activation(screen);
        return false;
    }

    /* Os botoes so respondem depois que a abertura termina. */
    if (screen->transition < 1.0f) {
        clear_button_activation(screen);
        return false;
    }

    /* O teclado exibe a pressao antes de executar inclusive a saida. */
    if (screen->pending_option >= 0) {
        screen->activation_time += delta_time;

        if (screen->activation_time >= BUTTON_FEEDBACK_DURATION) {
            int option = screen->pending_option;
            clear_button_activation(screen);
            return option_is_enabled(option) && option == 2;
        }

        return false;
    }

    int direction = 0;

    if (IsKeyPressed(KEY_DOWN)) {
        direction = 1;
    } else if (IsKeyPressed(KEY_UP)) {
        direction = -1;
    }

    if (direction != 0) {
        screen->mouse_focus = false;
        do {
            screen->selected_option =
                (screen->selected_option + direction
                 + MENU_OPTION_COUNT) % MENU_OPTION_COUNT;
        } while (!option_is_enabled(screen->selected_option));
    }

    Vector2 mouse = GetMousePosition();
    Vector2 mouse_delta = GetMouseDelta();
    bool mouse_moved = mouse_delta.x != 0 || mouse_delta.y != 0;

    if (mouse_moved && direction == 0) {
        screen->mouse_focus = true;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        screen->pressed_option = -1;
    }

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        if (!option_is_enabled(i)) {
            continue;
        }

        bool hovered = CheckCollisionPointRec(
            mouse, get_button_bounds(screen, i)
        );

        if (hovered && mouse_moved && direction == 0) {
            screen->selected_option = i;
        }

        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            screen->mouse_focus = true;
            screen->selected_option = i;
            screen->pressed_option = i;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        int pressed = screen->pressed_option;
        screen->pressed_option = -1;

        if (pressed >= 0
            && option_is_enabled(pressed)
            && CheckCollisionPointRec(
                mouse, get_button_bounds(screen, pressed)
            )) {
            return pressed == 2;
        }
    }

    if (IsKeyPressed(KEY_ENTER)
        && option_is_enabled(screen->selected_option)) {
        screen->mouse_focus = false;
        screen->pressed_option = -1;
        screen->pending_option = screen->selected_option;
        screen->activation_time = 0.0f;
    }

    return false;
}

/* Retorna true apenas quando houver uma solicitacao de saida. */
bool menu_screen_update(MenuScreen *screen)
{
    float delta_time = GetFrameTime();

    if (delta_time > MAX_FRAME_TIME) {
        delta_time = MAX_FRAME_TIME;
    }

    screen->elapsed_time += delta_time;
    if (screen->wind_test_ready) {
        screen->wind_playback_time = fmodf(
            screen->wind_playback_time + delta_time,
            WIND_FRAME_COUNT / WIND_PLAYBACK_FPS
        );
        screen->wind_frame = (int)(screen->wind_playback_time * WIND_PLAYBACK_FPS)
            % WIND_FRAME_COUNT;
    }
    update_leaves(screen, delta_time);
    update_transition(screen, delta_time);

    bool exit_requested = update_menu_input(screen, delta_time);
    update_button_animations(screen, delta_time);
    return exit_requested;
}

void menu_screen_draw(const MenuScreen *screen)
{
    ClearBackground((Color){24, 32, 28, 255});
    draw_background(screen->wind_test_ready ? screen->wind_background : screen->background);
    if (screen->wind_test_ready) {
        draw_wind_regions(screen);
    }

    DrawRectangle(
        0, 0,
        GetScreenWidth(), GetScreenHeight(),
        Fade(BLACK, 0.18f)
    );

    /* As folhas ficam atras do logo e dos botoes. */
    for (int i = 0; i < MENU_LEAF_COUNT; i++) {
        draw_leaf(&screen->leaves[i], screen->elapsed_time);
    }

    if (!screen->wind_test_ready) {
        draw_foliage(screen);
    }
    draw_logo(screen);
    draw_start_prompt(screen);
    draw_buttons(screen);

    float hint_alpha = ease_in_out(screen->transition);

    if (hint_alpha > 0.0f) {
        draw_centered_text(
            "Modos em desenvolvimento | ESC: voltar",
            685, 18, RAYWHITE, hint_alpha
        );
    }
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

    if (screen->foliage.id != 0) {
        UnloadTexture(screen->foliage);
    }

    if (screen->wind_background.id != 0) {
        UnloadTexture(screen->wind_background);
    }

    if (screen->wind_atlas.id != 0) {
        UnloadTexture(screen->wind_atlas);
    }

    *screen = (MenuScreen){0};
}
