/* Coordena telas, transições e o ciclo de vida dos recursos gráficos. */
#include "game.h"
#include "ui_theme.h"
#include "plant_data.h"
#include "game_audio.h"

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

static void remember_battle_team(Game *game)
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
}

static void leave_encounter(Game *game)
{
    remember_battle_team(game);
    if (game->battle_screen.battle.state == BATTLE_STATE_PLAYER_WIN) {
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
    audio_init(&game->audio);
    srand((unsigned int)time(NULL));
    menu_screen_init(&game->menu);
    player_assets_load(&game->player_assets);
    trainer_select_assets_load(&game->trainer_select_assets);
    intro_assets_load(&game->intro_assets);
    battle_screen_assets_load(&game->battle_assets);
    relay_ui_load(&game->relay_assets);
    world_init(&game->world);
    world_assets_load(&game->world_assets);
    trainer_select_screen_init(&game->trainer_select);
    name_entry_init(&game->name_entry);
    player_init(&game->player, TRAINER_MALE, world_spawn());
}

/* Só intercepta saídas da partida; ESC ainda fecha diálogos e submenus primeiro. */
static bool escape_leaves_session(const Game *game)
{
    if (game->screen == GAME_EXPLORATION)
        return !game->world.dialogue_title && !game->world.logic_quiz_open &&
               !game->world.relay_panel_open && !game->world.completed;
    if (game->screen == GAME_BATTLE) {
        const BattleScreen *screen = &game->battle_screen;
        const BattleState *battle = &screen->battle;
        bool forced = battle->state == BATTLE_STATE_REPLACEMENT && battle->current_turn == 0;
        bool ended = battle->state == BATTLE_STATE_PLAYER_WIN || battle->state == BATTLE_STATE_ENEMY_WIN;
        return !ended && (screen->setup_side >= 0 || screen->menu == BATTLE_MENU_ROOT || forced);
    }
    return false;
}

static Rectangle exit_confirm_button(int index)
{
    return (Rectangle){index == 0 ? 245 : 655,410,380,64};
}

/* O mundo e os temporizadores de combate ficam pausados enquanto o modal está aberto. */
static void update_exit_confirm(Game *game)
{
    float scale = fminf(GetRenderWidth()/1280.0f,GetRenderHeight()/720.0f);
    selection_ui_update_fonts(&game->trainer_select_assets.ui,scale);
    if (game->input_blocked) {
        if (!transition_input_down()) game->input_blocked = false;
        return;
    }
    int previous = game->exit_confirm_cursor;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_A) ||
        IsKeyPressed(KEY_D) || IsKeyPressed(KEY_TAB)) game->exit_confirm_cursor = 1-previous;
    Vector2 mouse = GetMousePosition(), motion = GetMouseDelta();
    int action = -1;
    for (int i = 0; i < 2; i++) {
        if (!CheckCollisionPointRec(mouse,exit_confirm_button(i))) continue;
        if (motion.x != 0 || motion.y != 0) game->exit_confirm_cursor = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) action = i;
    }
    if (IsKeyPressed(KEY_ESCAPE)) action = 0;
    else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) action = game->exit_confirm_cursor;
    if (previous != game->exit_confirm_cursor) audio_play(&game->audio,SFX_UI_MOVE);
    if (action < 0) return;
    game->exit_confirm_open = false;
    if (action == 1) {
        audio_play(&game->audio,SFX_UI_CONFIRM);
        change_screen(game,GAME_MENU);
    } else {
        audio_play(&game->audio,SFX_UI_BACK);
        game->input_blocked = true;
    }
}

static void draw_exit_confirm(const Game *game)
{
    const SelectionUiAssets *fonts = &game->trainer_select_assets.ui;
    const UiTheme *ui = &game->ui;
    DrawRectangle(0,0,1280,720,Fade((Color){12,25,17,255},0.80f));
    ui_botanical_panel(ui,(Rectangle){185,210,910,300});
    selection_ui_pixel_box(ui,fonts,"Sair da partida?",(Rectangle){225,242,830,52},36,SELECTION_INK);
    selection_ui_pixel_box(ui,fonts,"Você voltará ao menu principal.",
        (Rectangle){225,312,830,30},28,SELECTION_INK);
    selection_ui_pixel_box(ui,fonts,"O progresso não será salvo.",
        (Rectangle){225,354,830,30},28,(Color){143,66,42,255});
    for (int i = 0; i < 2; i++)
        selection_ui_button(ui,fonts,exit_confirm_button(i),
            i == 0 ? "Continuar jogando" : "Sair para o menu",game->exit_confirm_cursor == i,false,1);
    selection_ui_pixel_box(ui,fonts,"Setas: escolher · ENTER: confirmar · ESC: continuar",
        (Rectangle){190,535,900,30},24,SELECTION_CREAM);
}

static void update_screens(Game *game)
{
    if (game->screen == GAME_MENU || game->screen == GAME_TRAINER_SELECT ||
        game->screen == GAME_NAME_ENTRY || game->screen == GAME_BATTLE ||
        game->screen == GAME_ENDING || game->screen == GAME_DEFEAT ||
        (game->screen == GAME_EXPLORATION && (game->world.logic_quiz_open || game->world.relay_panel_open))) {
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
    if (IsKeyPressed(KEY_ESCAPE) && escape_leaves_session(game)) {
        game->exit_confirm_open = true;
        game->exit_confirm_cursor = 0;
        game->input_blocked = true;
        audio_play(&game->audio,SFX_UI_CONFIRM);
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
            } else if (game->world.completed) {
                result_screen_begin(&game->result);
                change_screen(game, GAME_ENDING);
            } else if (IsKeyPressed(KEY_ESCAPE)) {
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
            if (game->battle_screen.setup_side < 0 &&
                game->battle_screen.battle.state == BATTLE_STATE_ENEMY_WIN) {
                result_screen_begin(&game->result);
                change_screen(game, GAME_DEFEAT);
                break;
            }
            BattleScreenAction action = battle_screen_update(&game->battle_screen, GetFrameTime());
            if (action == BATTLE_SCREEN_TEAM_READY) {
                game->battle_screen.setup_side = -1;
                game->battle_screen.battle.enemy_team = guardian_team(game->encounter);
            }
            if (action == BATTLE_SCREEN_RETURN) leave_encounter(game);
            break;
        }
        case GAME_ENDING:
        case GAME_DEFEAT: {
            ResultAction action = result_screen_update(&game->result,true,GetFrameTime());
            if (game->screen == GAME_DEFEAT) {
                if (action == RESULT_PRIMARY) {
                    WorldEncounter encounter = game->encounter;
                    remember_battle_team(game);
                    start_encounter(game,encounter);
                } else if (action == RESULT_SECONDARY) leave_encounter(game);
            } else if (action == RESULT_PRIMARY) {
                trainer_select_screen_init(&game->trainer_select);
                change_screen(game,GAME_TRAINER_SELECT);
            } else if (action == RESULT_SECONDARY) change_screen(game,GAME_MENU);
            break;
        }
        case GAME_INTRO:
        case GAME_NAME_ENTRY: break; /* Atualizadas acima para manter os relógios contínuos. */
    }
}

/* Compara o estado antes e depois das telas para escolher sons sem alterar regras. */
void game_update(Game *game)
{
    GameAudioSnapshot before = game_audio_capture(game);
    /* O bloqueio de transição também protege os botões das telas de resultado. */
    if (game->input_blocked && (game->screen == GAME_ENDING || game->screen == GAME_DEFEAT))
        result_screen_update(&game->result,false,GetFrameTime());
    if (game->exit_confirm_open) update_exit_confirm(game);
    else update_screens(game);
    game_audio_react(game, &before);
}

void game_draw(const Game *game)
{
    const UiTheme *ui = &game->ui;
    switch (game->screen) {
        case GAME_ENDING:
        case GAME_DEFEAT:
            result_screen_draw(ui,&game->trainer_select_assets.ui,&game->trainer_select_assets.background,
                &game->intro_assets,&game->result,
                game->screen == GAME_DEFEAT ? &game->battle_screen.battle.player_team : &game->campaign.team,
                game->campaign.player_name,game->screen == GAME_DEFEAT,
                game->encounter == WORLD_ENCOUNTER_ALOE ? "Depósito" : "Viveiro");
            break;
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
            if (game->world.relay_panel_open)
                relay_ui_draw(ui,&game->trainer_select_assets.ui,&game->relay_assets,&game->world);
            if (game->world.logic_quiz_open)
                logic_setup_draw(ui,&game->trainer_select_assets.ui,&game->intro_assets,
                    &game->logic_setup,&game->campaign.team,game->world.logic_move_mask);
            break;
        }
    }
    if (game->exit_confirm_open) draw_exit_confirm(game);
}

void game_shutdown(Game *game)
{
    audio_shutdown(&game->audio);
    ui_theme_unload(&game->ui);
    world_assets_unload(&game->world_assets);
    player_assets_unload(&game->player_assets);
    trainer_select_assets_unload(&game->trainer_select_assets);
    intro_assets_unload(&game->intro_assets);
    battle_screen_assets_unload(&game->battle_assets);
    relay_ui_unload(&game->relay_assets);
    menu_screen_shutdown(&game->menu);
}
