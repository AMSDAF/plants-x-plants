/* Compõe a tela de recordes com abas e seleção de linha navegável. */
#include "leaderboard_screen.h"
#include "input_map.h"
#include <math.h>

static Rectangle panel_bounds(void) { return (Rectangle){100, 196, 1080, 414}; }

Rectangle leaderboard_screen_tab_bounds(int index)
{
    return (Rectangle){index == 0 ? 220 : 660, 132, 400, 54};
}

Rectangle leaderboard_screen_row_bounds(int row)
{
    return (Rectangle){118, 270 + row * 27, 1044, 27};
}

Rectangle leaderboard_screen_button_bounds(int index)
{
    return (Rectangle){index == 0 ? 160 : 680, 628, 440, 64};
}

static Rectangle column_bounds(int column, float y)
{
    static const float x[] = {124, 190, 570, 800, 1020};
    static const float width[] = {50, 360, 200, 200, 140};
    return (Rectangle){x[column], y, width[column], 27};
}

static int current_count(const LeaderboardScreen *screen)
{
    return screen->order == LEADERBOARD_ORDER_TIME ? screen->time_count : screen->damage_count;
}

static const LeaderboardEntry *current_entries(const LeaderboardScreen *screen)
{
    return screen->order == LEADERBOARD_ORDER_TIME ? screen->time_entries : screen->damage_entries;
}

static void select_index(LeaderboardScreen *screen, int index)
{
    int count = current_count(screen);
    if (count <= 0) {
        screen->selected[screen->order] = 0;
        screen->scroll[screen->order] = 0;
        return;
    }
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;
    int max_scroll = count - LEADERBOARD_VISIBLE_ROWS;
    if (max_scroll < 0) max_scroll = 0;
    screen->selected[screen->order] = index;
    if (index < screen->scroll[screen->order]) screen->scroll[screen->order] = index;
    if (index >= screen->scroll[screen->order] + LEADERBOARD_VISIBLE_ROWS)
        screen->scroll[screen->order] = index - LEADERBOARD_VISIBLE_ROWS + 1;
    if (screen->scroll[screen->order] > max_scroll) screen->scroll[screen->order] = max_scroll;
}

static void set_order(LeaderboardScreen *screen, LeaderboardOrder order)
{
    screen->order = order;
    select_index(screen, screen->selected[order]);
}

static int hovered_control(const LeaderboardScreen *screen, Vector2 mouse)
{
    for (int i = 0; i < 2; i++)
        if (CheckCollisionPointRec(mouse, leaderboard_screen_tab_bounds(i))) return i;
    for (int i = 0; i < 2; i++)
        if (CheckCollisionPointRec(mouse, leaderboard_screen_button_bounds(i))) return i + 2;
    for (int i = 0; i < LEADERBOARD_VISIBLE_ROWS; i++)
        if (CheckCollisionPointRec(mouse, leaderboard_screen_row_bounds(i))) return i + 4;
    return -1;
}

void leaderboard_screen_begin(LeaderboardScreen *screen)
{
    *screen = (LeaderboardScreen){.pressed = -1, .order = LEADERBOARD_ORDER_TIME};
    Leaderboard board;
    leaderboard_load(&board);
    leaderboard_sort(&board, LEADERBOARD_ORDER_TIME, screen->time_entries, &screen->time_count);
    leaderboard_sort(&board, LEADERBOARD_ORDER_DAMAGE, screen->damage_entries, &screen->damage_count);
}

LeaderboardScreenAction leaderboard_screen_update(LeaderboardScreen *screen, bool accept_input, float dt)
{
    dt = fminf(0.1f, fmaxf(0, dt));
    screen->elapsed += dt;
    if (!accept_input) {
        screen->pressed = -1;
        return LEADERBOARD_SCREEN_NONE;
    }

    Vector2 mouse = GetMousePosition();
    int hovered = hovered_control(screen, mouse);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) screen->pressed = hovered;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && screen->pressed == hovered && hovered >= 0) {
        screen->pressed = -1;
        if (hovered < 2) set_order(screen, (LeaderboardOrder)hovered);
        else if (hovered == 2) return LEADERBOARD_SCREEN_BACK;
        else if (hovered == 3) return LEADERBOARD_SCREEN_PLAY;
        else select_index(screen, hovered - 4 + screen->scroll[screen->order]);
    }

    if (input_direction_pressed(INPUT_DIR_LEFT) || input_left_bumper_pressed())
        set_order(screen, LEADERBOARD_ORDER_TIME);
    else if (input_direction_pressed(INPUT_DIR_RIGHT) || input_right_bumper_pressed())
        set_order(screen, LEADERBOARD_ORDER_DAMAGE);
    if (input_direction_pressed(INPUT_DIR_UP))
        select_index(screen, screen->selected[screen->order] - 1);
    if (input_direction_pressed(INPUT_DIR_DOWN))
        select_index(screen, screen->selected[screen->order] + 1);
    if (input_back_pressed()) return LEADERBOARD_SCREEN_BACK;
    if (input_confirm_pressed()) return LEADERBOARD_SCREEN_PLAY;
    return LEADERBOARD_SCREEN_NONE;
}

static void draw_row(const UiTheme *ui, const SelectionUiAssets *assets, int rank,
                     const LeaderboardEntry *entry, float y, bool damage_first)
{
    const Color ink = SELECTION_INK;
    char time_text[32];
    leaderboard_format_time(entry->time_ms, time_text, sizeof(time_text));

    selection_ui_pixel_box(ui, assets, TextFormat("%d.", rank), column_bounds(0, y), 20, ink);
    selection_ui_pixel_box(ui, assets, entry->name, column_bounds(1, y), 20, ink);
    selection_ui_pixel_box(ui, assets, damage_first ? TextFormat("%lld", entry->damage) : time_text,
                           column_bounds(2, y), 20, ink);
    selection_ui_pixel_box(ui, assets, damage_first ? time_text : TextFormat("%lld", entry->damage),
                           column_bounds(3, y), 20, ink);
    selection_ui_pixel_box(ui, assets, entry->date, column_bounds(4, y), 20, ink);
}

static void draw_ranking(const UiTheme *ui, const SelectionUiAssets *assets,
                         const LeaderboardScreen *screen)
{
    const Color ink = SELECTION_INK;
    Rectangle bounds = panel_bounds();
    ui_botanical_panel(ui, bounds);
    selection_ui_pixel_box(ui, assets,
                           screen->order == LEADERBOARD_ORDER_TIME ? "MAIS RÁPIDOS" : "MENOS DANO",
                           (Rectangle){bounds.x + 24, bounds.y + 10, bounds.width - 48, 30}, 26, ink);

    float header_y = bounds.y + 44;
    bool damage_first = screen->order == LEADERBOARD_ORDER_DAMAGE;
    selection_ui_pixel_box(ui, assets, "#", column_bounds(0, header_y), 20, ink);
    selection_ui_pixel_box(ui, assets, "Nome", column_bounds(1, header_y), 20, ink);
    selection_ui_pixel_box(ui, assets, damage_first ? "Dano" : "Tempo", column_bounds(2, header_y), 20, ink);
    selection_ui_pixel_box(ui, assets, damage_first ? "Tempo" : "Dano", column_bounds(3, header_y), 20, ink);
    selection_ui_pixel_box(ui, assets, "Data", column_bounds(4, header_y), 20, ink);

    int count = current_count(screen);
    if (count <= 0) {
        selection_ui_pixel_box(ui, assets, "Nenhuma aventura concluída ainda",
                               (Rectangle){bounds.x + 120, bounds.y + 130, bounds.width - 240, 40},
                               26, ink);
        return;
    }

    const LeaderboardEntry *entries = current_entries(screen);
    int first = screen->scroll[screen->order];
    int last = first + LEADERBOARD_VISIBLE_ROWS;
    if (last > count) last = count;
    for (int row = 0; row < last - first; row++) {
        int index = first + row;
        Rectangle line = leaderboard_screen_row_bounds(row);
        bool selected = index == screen->selected[screen->order];
        DrawRectangleRec(line, selected ? Fade(GOLD, 0.28f)
                                        : row % 2 == 0 ? Fade(ink, 0.08f) : Fade(ink, 0.03f));
        if (selected) DrawRectangleLinesEx(line, 2, Fade(GOLD, 0.9f));
        draw_row(ui, assets, index + 1, &entries[index], line.y, damage_first);
    }
}

void leaderboard_screen_draw(const UiTheme *ui, const SelectionUiAssets *assets,
                             const TrainerBackgroundAssets *background,
                             const LeaderboardScreen *screen)
{
    trainer_background_draw_dimmed(background, screen->elapsed);
    const Color cream = SELECTION_CREAM;
    Vector2 mouse = GetMousePosition();

    selection_ui_pixel_box(ui, assets, "RANKINGS DA ESTUFA", (Rectangle){100, 24, 1080, 64}, 64, cream);
    selection_ui_pixel_box(ui, assets, "Recordes de quem concluiu a aventura",
                           (Rectangle){100, 94, 1080, 28}, 24, cream);

    for (int i = 0; i < 2; i++) {
        Rectangle bounds = leaderboard_screen_tab_bounds(i);
        bool hovered = CheckCollisionPointRec(mouse, bounds);
        selection_ui_button(ui, assets, bounds, i == 0 ? "MAIS RÁPIDOS" : "MENOS DANO",
                            screen->order == i || hovered, hovered && screen->pressed == i, 1);
    }
    draw_ranking(ui, assets, screen);

    for (int i = 0; i < 2; i++) {
        Rectangle bounds = leaderboard_screen_button_bounds(i);
        bool hovered = CheckCollisionPointRec(mouse, bounds);
        selection_ui_button(ui, assets, bounds, i == 0 ? "Voltar · ESC" : "Jogar · ENTER",
                            hovered, hovered && screen->pressed == i + 2, 1);
    }
    selection_ui_pixel_box(ui, assets,
                           "Setas/WASD: alternar ranking e navegar · Enter: jogar · Esc: voltar",
                           (Rectangle){100, 700, 1080, 18}, 18, cream);
}
