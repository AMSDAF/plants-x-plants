/* Coordena telas, transições e o ciclo de vida dos recursos gráficos. */
#include "game.h"
#include "ui_theme.h"
#include "plant_data.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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
                        KEY_D, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
                        KEY_ONE, KEY_TWO, KEY_THREE, KEY_Q, KEY_E, KEY_R, KEY_M, KEY_BACKSPACE};
    for (unsigned int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        if (IsKeyDown(keys[i])) return true;
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

static Team guardian_team(WorldEncounter encounter)
{
    Team team = {0};
    Team_SelectSpecies(&team,0,encounter == WORLD_ENCOUNTER_ALOE ? SPECIES_ALOE : SPECIES_SUNFLOWER);
    return team;
}

static void start_encounter(Game *game, WorldEncounter encounter)
{
    Team team = {0};
    bool configured = game->campaign.has_team;
    if (configured) {
        team = game->campaign.team;
        Team_Restore(&team);
    } else {
        Team_SelectSpecies(&team, 0, SPECIES_CACTUS);
        Team_SelectSpecies(&team, 1, SPECIES_CLOVER);
    }

    BattleScreen *screen = &game->battle_screen;
    *screen = (BattleScreen){
        .scene = encounter == WORLD_ENCOUNTER_ALOE ? BATTLE_SCENE_DEPOT : BATTLE_SCENE_NURSERY,
        .setup_side = configured ? -1 : 0,
        .choosing_team = !configured,
        .animation_side = -1,
        .draft = {LOGIC_OWN_LOW_HP, LOGIC_ENEMY_STATUS, LOGIC_AND}
    };
    Battle_Init(&screen->battle, team, guardian_team(encounter));
    LogicFormula initial_formula = {LOGIC_OWN_LOW_HP, LOGIC_ENEMY_STATUS, LOGIC_AND};
    for (int i = 0; i < team.count; i++) {
        screen->battle.formulas[0][i] = game->campaign.has_formula
            ? game->campaign.formulas[i] : initial_formula;
        if ((game->world.logic_move_mask & (1u << i)) && game->campaign.has_formula)
            screen->battle.boosted_move_indices[0][i] = game->campaign.boosted_move_indices[i];
    }
    game->encounter = encounter;
    change_screen(game, GAME_BATTLE);
}

static void leave_encounter(Game *game)
{
    const BattleScreen *screen = &game->battle_screen;
    if (screen->setup_side < 0) {
        game->campaign.team = screen->battle.player_team;
        Team_Restore(&game->campaign.team);
        for (int i = 0; i < screen->battle.player_team.count; i++)
            game->campaign.formulas[i] = screen->battle.formulas[0][i];
        for (int i = 0; i < screen->battle.player_team.count; i++)
            game->campaign.boosted_move_indices[i] = screen->battle.boosted_move_indices[0][i];
        game->campaign.has_team = true;
        game->campaign.has_formula = false;
        for (int i = 0; i < screen->battle.player_team.count; i++)
            if (game->campaign.boosted_move_indices[i] >= 0) game->campaign.has_formula = true;
    }
    if (screen->battle.state == BATTLE_STATE_PLAYER_WIN) {
        world_finish_encounter(&game->world, game->encounter);
    } else {
        player_init(&game->player, game->campaign.trainer, world_spawn());
        world_return_to_courtyard(&game->world, &game->player.feet);
    }
    game->encounter = WORLD_ENCOUNTER_NONE;
    change_screen(game, GAME_EXPLORATION);
}

void game_init(Game *game)
{
    *game = (Game){0};
    for (int i = 0; i < MAX_TEAM_SIZE; i++) game->campaign.boosted_move_indices[i] = -1;
    ui_theme_load(&game->ui);
    srand((unsigned int)time(NULL));
    menu_screen_init(&game->menu);
    player_assets_load(&game->player_assets);
    trainer_select_assets_load(&game->trainer_select_assets);
    intro_assets_load(&game->intro_assets);
    battle_screen_assets_load(&game->battle_assets);
    world_init(&game->world);
    world_assets_load(&game->world_assets);
    trainer_select_screen_init(&game->trainer_select);
    name_entry_init(&game->name_entry);
    player_init(&game->player, TRAINER_MALE, world_spawn());
}

void game_update(Game *game)
{
    if (game->screen == GAME_MENU || game->screen == GAME_TRAINER_SELECT ||
        game->screen == GAME_NAME_ENTRY || game->screen == GAME_BATTLE ||
        (game->screen == GAME_EXPLORATION && game->world.logic_quiz_open)) {
        float scale = fminf(GetRenderWidth() / 1280.0f, GetRenderHeight() / 720.0f);
        selection_ui_update_fonts(&game->trainer_select_assets.ui, scale);
    }
    if (game->screen == GAME_EXPLORATION) ui_theme_update(&game->ui, GetFrameTime());
    if (game->screen == GAME_NAME_ENTRY) {
        trainer_select_screen_update(&game->trainer_select, false, GetFrameTime());
        NameEntryAction action=name_entry_update(&game->name_entry, !game->input_blocked, GetFrameTime());
        if (action==NAME_ENTRY_BACK) name_entry_begin(&game->name_entry,false);
        else if (action==NAME_ENTRY_SELECTION) change_screen(game,GAME_TRAINER_SELECT);
        else if (action==NAME_ENTRY_CONFIRM && name_entry_confirm(&game->name_entry,game->campaign.player_name)) {
            game->campaign.trainer=game->trainer_select.selected;
            game->campaign.has_team=false;
            game->campaign.has_formula=false;
            game->campaign.team=(Team){0};
            game->logic_setup = (LogicSetupScreen){0};
            for (int i = 0; i < MAX_TEAM_SIZE; i++) game->campaign.boosted_move_indices[i] = -1;
            intro_screen_init(&game->intro);
            world_init(&game->world);
            player_init(&game->player,game->campaign.trainer,world_spawn());
            world_snap_camera(&game->world, game->player.feet);
            change_screen(game,GAME_INTRO);
        }
        if (!transition_input_down()) game->input_blocked=false;
        return;
    }
    if (game->screen == GAME_INTRO) {
        float scale = fminf(GetRenderWidth() / 1280.0f, GetRenderHeight() / 720.0f);
        intro_assets_update_fonts(&game->intro_assets, scale);
        IntroAction action = intro_screen_update(&game->intro, !game->input_blocked, GetFrameTime());
        if (action == INTRO_BACK) {
            name_entry_begin(&game->name_entry, true);
            change_screen(game, GAME_NAME_ENTRY);
        } else if (action == INTRO_FINISH && intro_screen_team(&game->intro, &game->campaign.team)) {
            game->campaign.has_team = true;
            change_screen(game, GAME_EXPLORATION);
        }
        if (!transition_input_down()) game->input_blocked = false;
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
            if (IsKeyPressed(KEY_F8)) game->world.detail_enabled = !game->world.detail_enabled;
            if (game->world.dialogue_title) {
                world_update_dialogue(&game->world, GetFrameTime());
            } else if (game->world.logic_quiz_open) {
                if (logic_setup_update(&game->logic_setup,&game->campaign.team,&game->world.logic_move_mask,
                    game->campaign.formulas,game->campaign.boosted_move_indices)) game->world.logic_quiz_open = false;
                game->campaign.has_formula = false;
                for (int i = 0; i < game->campaign.team.count; i++)
                    if (game->campaign.boosted_move_indices[i] >= 0) game->campaign.has_formula = true;
            } else if (game->world.relay_panel_open) {
                world_update_relay_panel(&game->world, game->campaign.player_name);
            } else if (IsKeyPressed(KEY_ESCAPE) || (game->world.completed && IsKeyPressed(KEY_ENTER))) {
                change_screen(game, GAME_MENU);
            } else if (!game->world.completed) {
                float dt = GetFrameTime();
                player_update(&game->player, &game->world, dt);
                world_follow_camera(&game->world, game->player.feet, dt);
                if (IsKeyPressed(KEY_E)) {
                    WorldEncounter encounter = world_interact(&game->world, &game->player.feet, game->campaign.player_name);
                    if (game->world.logic_quiz_open)
                        logic_setup_begin(&game->logic_setup,&game->campaign.team,game->campaign.formulas,
                            game->campaign.boosted_move_indices);
                    if (game->world.logic_quiz_open || game->world.relay_panel_open || game->world.dialogue_title)
                        game->player.moving = false;
                    if (encounter != WORLD_ENCOUNTER_NONE) start_encounter(game, encounter);
                }
            }
            break;
        case GAME_BATTLE: {
            BattleScreenAction action = battle_screen_update(&game->battle_screen, GetFrameTime());
            if (action == BATTLE_SCREEN_TEAM_READY) {
                game->battle_screen.setup_side = -1;
                game->battle_screen.battle.enemy_team = guardian_team(game->encounter);
            }
            if (action == BATTLE_SCREEN_RETURN) leave_encounter(game);
            break;
        }
        case GAME_INTRO:
        case GAME_NAME_ENTRY: break; /* Atualizadas acima para manter os relógios contínuos. */
    }
}

void game_draw(const Game *game)
{
    const UiTheme *ui = &game->ui;
    switch (game->screen) {
        case GAME_MENU: menu_screen_draw(ui, &game->menu, &game->trainer_select_assets.ui); break;
        case GAME_BATTLE:
            battle_screen_draw(ui, &game->battle_screen, &game->battle_assets, &game->trainer_select_assets.ui,
                                game->intro_assets.plants, game->intro_assets.sources);
            break;
        case GAME_TRAINER_SELECT:
            trainer_select_screen_draw(ui, &game->trainer_select, &game->trainer_select_assets);
            break;
        case GAME_NAME_ENTRY:
            name_entry_draw(ui, &game->name_entry,&game->trainer_select,&game->trainer_select_assets);
            break;
        case GAME_INTRO:
            intro_screen_draw(ui, &game->intro, &game->trainer_select_assets,
                              &game->intro_assets, game->campaign.player_name);
            break;
        case GAME_EXPLORATION: {
            ClearBackground(BLACK);
            Camera2D camera = world_camera(&game->world);
            BeginMode2D(camera);
            world_draw(ui, &game->world, &game->world_assets);
            world_draw_npcs(&game->world, &game->world_assets, game->player.feet, camera, false);
            player_draw(ui, &game->player, &game->player_assets, camera);
            world_draw_interior_foreground(&game->world, &game->world_assets,
                                           game->player.feet);
            world_draw_tree_canopy(&game->world, &game->world_assets, game->player.feet);
            world_draw_npcs(&game->world, &game->world_assets, game->player.feet, camera, true);
            EndMode2D();
            world_draw_interface(ui, &game->world, game->player.feet);
            if (game->world.logic_quiz_open)
                logic_setup_draw(ui,&game->trainer_select_assets.ui,&game->intro_assets,
                    &game->logic_setup,&game->campaign.team,game->world.logic_move_mask);
            break;
        }
    }
}

void game_shutdown(Game *game)
{
    ui_theme_unload(&game->ui);
    world_assets_unload(&game->world_assets);
    player_assets_unload(&game->player_assets);
    trainer_select_assets_unload(&game->trainer_select_assets);
    intro_assets_unload(&game->intro_assets);
    battle_screen_assets_unload(&game->battle_assets);
    menu_screen_shutdown(&game->menu);
}
