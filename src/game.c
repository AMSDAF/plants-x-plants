/* Coordena telas, transições e o ciclo de vida dos recursos gráficos. */
#include "game.h"

static void change_screen(Game *game, GameScreen screen)
{
    game->screen = screen;
    game->input_blocked = true;
    menu_screen_reset_input(&game->menu);
    game->trainer_select.pressed = -1;
    game->player.moving = false;
    game->player.walk_time = 0;
    /* Descarta eventos enfileirados; entradas mantidas aguardam soltura. */
    while (GetKeyPressed() != 0) {}
    while (GetCharPressed() != 0) {}
}

static bool transition_input_down(void)
{
    const int keys[] = {KEY_ENTER, KEY_ESCAPE, KEY_SPACE, KEY_W, KEY_A, KEY_S,
                        KEY_D, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
    for (unsigned int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        if (IsKeyDown(keys[i])) return true;
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

void game_init(Game *game)
{
    *game = (Game){0};
    menu_screen_init(&game->menu);
    player_assets_load(&game->player_assets);
    trainer_select_assets_load(&game->trainer_select_assets);
    world_init(&game->world);
    trainer_select_screen_init(&game->trainer_select);
    name_entry_init(&game->name_entry);
    player_init(&game->player, TRAINER_MALE, (Vector2){640, 420});
}

void game_update(Game *game)
{
    if (game->screen == GAME_NAME_ENTRY) {
        trainer_select_screen_update(&game->trainer_select, false, GetFrameTime());
        NameEntryAction action=name_entry_update(&game->name_entry, !game->input_blocked, GetFrameTime());
        if (action==NAME_ENTRY_BACK) name_entry_begin(&game->name_entry,false);
        else if (action==NAME_ENTRY_SELECTION) change_screen(game,GAME_TRAINER_SELECT);
        else if (action==NAME_ENTRY_CONFIRM && name_entry_confirm(&game->name_entry,game->campaign.player_name)) {
            game->campaign.trainer=game->trainer_select.selected;
            game->campaign.ready=true;
            player_init(&game->player,game->campaign.trainer,(Vector2){640,420});
            change_screen(game,GAME_EXPLORATION);
        }
        if (!transition_input_down()) game->input_blocked=false;
        return;
    }
    if (game->input_blocked) {
        /* A seleção continua animando enquanto aguarda a soltura das entradas. */
        if (game->screen == GAME_MENU) menu_screen_update(&game->menu, false);
        if (game->screen == GAME_TRAINER_SELECT)
            trainer_select_screen_update(&game->trainer_select, false, GetFrameTime());
        if (!transition_input_down()) game->input_blocked = false;
        return;
    }
    switch (game->screen) {
        case GAME_MENU: {
            MenuAction action = menu_screen_update(&game->menu, true);
            if (action == MENU_ACTION_EXIT) game->exit_requested = true;
            if (action == MENU_ACTION_CAMPAIGN) {
                trainer_select_screen_init(&game->trainer_select);
                change_screen(game, GAME_TRAINER_SELECT);
            }
            break;
        }
        case GAME_TRAINER_SELECT: {
            TrainerSelectAction action = trainer_select_screen_update(&game->trainer_select, true, GetFrameTime());
            if (action == TRAINER_SELECT_BACK) change_screen(game, GAME_MENU);
            else if (action == TRAINER_SELECT_CONFIRM) {
                name_entry_begin(&game->name_entry,true);
                change_screen(game, GAME_NAME_ENTRY);
            }
            break;
        }
        case GAME_EXPLORATION:
            if (IsKeyPressed(KEY_ESCAPE)) change_screen(game, GAME_MENU);
            else player_update(&game->player, &game->world, GetFrameTime());
            break;
        case GAME_NAME_ENTRY: break; /* Atualizada acima para manter os relógios contínuos. */
    }
}

void game_draw(const Game *game)
{
    switch (game->screen) {
        case GAME_MENU: menu_screen_draw(&game->menu); break;
        case GAME_TRAINER_SELECT:
            trainer_select_screen_draw(&game->trainer_select, &game->trainer_select_assets);
            break;
        case GAME_NAME_ENTRY:
            name_entry_draw(&game->name_entry,&game->trainer_select,&game->trainer_select_assets);
            break;
        case GAME_EXPLORATION:
            world_draw(&game->world);
            player_draw(&game->player, &game->player_assets);
            if (game->player.trainer == TRAINER_MALE) {
                const char *version = game->player.use_walk6 && game->player_assets.male_walk6.id
                    ? "6 poses (experimental)" : "4 poses (aprovado)";
                DrawText(TextFormat("F6: comparar caminhada | %s | repouso aprovado", version),
                         70, 85, 20, LIGHTGRAY);
            }
            break;
    }
}

void game_shutdown(Game *game)
{
    player_assets_unload(&game->player_assets);
    trainer_select_assets_unload(&game->trainer_select_assets);
    menu_screen_shutdown(&game->menu);
}
