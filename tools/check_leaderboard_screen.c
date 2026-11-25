/* Verifica navegação do ranking e gravação da partida com o coordenador real. */
#include "game.h"
#include "leaderboard.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void input(unsigned int type, int a, int b)
{
    PlayAutomationEvent((AutomationEvent){.type = type, .params = {a, b, 0, 0}});
}

static void tick(Game *game)
{
    game_update(game);
    BeginDrawing();
    game_draw(game);
    EndDrawing();
}

static void key(Game *game, int code)
{
    input(2, code, 0);
    tick(game);
    input(1, code, 0);
    tick(game);
    tick(game);
}

static void click(Game *game, Vector2 point)
{
    input(7, (int)point.x, (int)point.y);
    input(6, MOUSE_BUTTON_LEFT, 0);
    tick(game);
    input(5, MOUSE_BUTTON_LEFT, 0);
    tick(game);
    tick(game);
}

static Vector2 center(Rectangle bounds)
{
    return (Vector2){bounds.x + bounds.width / 2, bounds.y + bounds.height / 2};
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "Verificação dos rankings");
    assert(IsWindowReady());
    SetExitKey(KEY_NULL);
    SetTargetFPS(120);

    char root[1024];
    snprintf(root, sizeof(root), "%s", GetWorkingDirectory());
    assert(DirectoryExists("build/leaderboard_screen_preview") ||
           MakeDirectory("build/leaderboard_screen_preview") == 0);

    Game game;
    game_init(&game);
    key(&game, KEY_ENTER);
    for (int i = 0; i < 20; i++) tick(&game);
    assert(game.screen == GAME_LEADERBOARD && !game.input_blocked);
    assert(game.leaderboard.order == LEADERBOARD_ORDER_TIME);
    assert(game.leaderboard.time_count == game.leaderboard.damage_count);

    /* Recordes fixos para validar a seleção sem depender do CSV do desenvolvedor. */
    for (int i = 0; i < 3; i++) {
        snprintf(game.leaderboard.time_entries[i].name, LEADERBOARD_NAME_CAPACITY, "Jogador %d", i + 1);
        game.leaderboard.time_entries[i].time_ms = (i + 1) * 1000;
        strcpy(game.leaderboard.time_entries[i].date, "2026-11-24");
        game.leaderboard.damage_entries[i] = game.leaderboard.time_entries[i];
        game.leaderboard.damage_entries[i].damage = (i + 1) * 10;
    }
    game.leaderboard.time_count = 3;
    game.leaderboard.damage_count = 3;

    /* Setas esquerda/direita alternam o ranking; cima/baixo navegam pelos recordes. */
    key(&game, KEY_RIGHT);
    assert(game.leaderboard.order == LEADERBOARD_ORDER_DAMAGE);
    key(&game, KEY_DOWN);
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_DAMAGE] == 1);
    key(&game, KEY_LEFT);
    assert(game.leaderboard.order == LEADERBOARD_ORDER_TIME);
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_TIME] == 0);
    key(&game, KEY_DOWN);
    key(&game, KEY_DOWN);
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_TIME] == 2);
    key(&game, KEY_UP);
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_TIME] == 1);

    click(&game, center(leaderboard_screen_tab_bounds(1)));
    assert(game.leaderboard.order == LEADERBOARD_ORDER_DAMAGE);
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_DAMAGE] == 1);
    click(&game, center(leaderboard_screen_tab_bounds(0)));
    assert(game.leaderboard.order == LEADERBOARD_ORDER_TIME);
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_TIME] == 1);
    click(&game, center(leaderboard_screen_row_bounds(2)));
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_TIME] == 2);

    /* Mais linhas do que a janela: a seleção rola o ranking sem perder o recorde. */
    for (int i = 3; i < 12; i++) {
        game.leaderboard.time_entries[i] = game.leaderboard.time_entries[2];
        snprintf(game.leaderboard.time_entries[i].name, LEADERBOARD_NAME_CAPACITY, "Jogador %d", i + 1);
    }
    game.leaderboard.time_count = 12;
    game.leaderboard.selected[LEADERBOARD_ORDER_TIME] = 0;
    game.leaderboard.scroll[LEADERBOARD_ORDER_TIME] = 0;
    for (int i = 0; i < 11; i++) key(&game, KEY_DOWN);
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_TIME] == 11);
    assert(game.leaderboard.scroll[LEADERBOARD_ORDER_TIME] == 2);
    key(&game, KEY_UP);
    assert(game.leaderboard.selected[LEADERBOARD_ORDER_TIME] == 10);

    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_MENU);
    key(&game, KEY_ENTER);
    click(&game, center(leaderboard_screen_button_bounds(0)));
    assert(game.screen == GAME_MENU);

    key(&game, KEY_ENTER);
    click(&game, center(leaderboard_screen_button_bounds(1)));
    assert(game.screen == GAME_TRAINER_SELECT);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_MENU);
    key(&game, KEY_ENTER);
    key(&game, KEY_ENTER);
    assert(game.screen == GAME_TRAINER_SELECT);

    strcpy(game.campaign.player_name, "Lia");
    game.run = (RunStats){.active = true, .elapsed = 12.345, .damage_taken = 77};
    game.screen = GAME_EXPLORATION;
    game.input_blocked = false;
    game.world.completed = true;
    game.world.dialogue_title = NULL;

    assert(ChangeDirectory("build/leaderboard_screen_preview"));
    assert(DirectoryExists("data") || MakeDirectory("data") == 0);
    remove("data/leaderboard.csv");
    tick(&game);
    assert(game.screen == GAME_ENDING && !game.run.active);
    double frozen = game.run.elapsed;
    for (int i = 0; i < 10; i++) tick(&game);
    assert(game.run.elapsed == frozen);

    Leaderboard board;
    leaderboard_load(&board);
    assert(board.count == 1);
    assert(strcmp(board.entries[0].name, "Lia") == 0);
    assert(board.entries[0].time_ms >= 12345 && board.entries[0].time_ms < 12400);
    assert(board.entries[0].damage == 77);
    assert(ChangeDirectory(root));

    game_shutdown(&game);
    CloseWindow();
    puts("PASS: menu → rankings com abas, setas, mouse, início da partida e gravação sem tempo da conclusão.");
    return 0;
}
