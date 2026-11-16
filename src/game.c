/* Coordena telas, transições e o ciclo de vida dos recursos gráficos. */
#include "game.h"
#include "plant_data.h"

#include <stdlib.h>
#include <time.h>

static void change_screen(Game *game, GameScreen screen)
{
    game->screen = screen;
    game->input_blocked = true;
    menu_screen_reset_input(&game->menu);
    game->trainer_select.pressed = -1;
    game->versus.pressed = -1;
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

static Team random_team(int size)
{
    int species[SPECIES_COUNT];
    for (int i = 0; i < SPECIES_COUNT; i++) species[i] = i;

    Team team = {0};
    for (int i = 0; i < size; i++) {
        int pick = i + rand() % (SPECIES_COUNT - i);
        Team_SelectSpecies(&team, i, species[pick]);
        species[pick] = species[i];
    }
    return team;
}

static void start_battle(Game *game, bool configure)
{
    Plant cactus = Plant_CreateFromSpecies(SPECIES_CACTUS, 1);
    Plant sunflower = Plant_CreateFromSpecies(SPECIES_SUNFLOWER, 1);
    Team first = {.plants = {cactus, sunflower}, .count = 2};
    Team second = {.plants = {sunflower, cactus}, .count = 2};

    BattleScreen *screen = &game->battle_screen;
    if (!configure) {
        first = screen->battle.player_team;
        second = screen->battle.enemy_team;
        Team_Restore(&first);
        Team_Restore(&second);
    }
    LogicFormula left = screen->battle.formulas[0];
    LogicFormula right = screen->battle.formulas[1];
    Battle_Init(&screen->battle, first, second);
    screen->versus_cpu = game->versus_mode == VERSUS_CPU;
    screen->campaign_battle = false;
    screen->cpu_wait = 0;
    screen->setup_side = configure ? 0 : -1;
    screen->choosing_team = configure;
    screen->roster_cursor = 0;
    screen->team_slot = 0;
    screen->draft = (LogicFormula){LOGIC_OWN_LOW_HP, LOGIC_ENEMY_RESERVE, LOGIC_AND};
    if (configure) {
        if (screen->versus_cpu)
            screen->battle.formulas[1] = (LogicFormula){LOGIC_OWN_LOW_HP, LOGIC_ENEMY_LOW_HP, LOGIC_AND};
    } else {
        screen->battle.formulas[0] = left;
        screen->battle.formulas[1] = right;
    }
    change_screen(game, GAME_BATTLE);
}

static Team guardian_team(WorldEncounter encounter, int size)
{
    static const SpeciesId aloe[MAX_TEAM_SIZE] = {
        SPECIES_ALOE, SPECIES_IVY, SPECIES_BAMBOO, SPECIES_CLOVER
    };
    static const SpeciesId sunflower[MAX_TEAM_SIZE] = {
        SPECIES_SUNFLOWER, SPECIES_ROSE, SPECIES_LOTUS, SPECIES_LAVENDER
    };
    const SpeciesId *species = encounter == WORLD_ENCOUNTER_ALOE ? aloe : sunflower;
    Team team = {0};
    for (int i = 0; i < size; i++) Team_SelectSpecies(&team, i, species[i]);
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
        Team_SelectSpecies(&team, 1, SPECIES_SUNFLOWER);
    }

    BattleScreen *screen = &game->battle_screen;
    *screen = (BattleScreen){
        .versus_cpu = true,
        .campaign_battle = true,
        .setup_side = configured ? -1 : 0,
        .choosing_team = !configured,
        .draft = {LOGIC_OWN_LOW_HP, LOGIC_ENEMY_RESERVE, LOGIC_AND}
    };
    Battle_Init(&screen->battle, team, guardian_team(encounter, team.count));
    if (configured) screen->battle.formulas[0] = game->campaign.formula;
    LogicCondition condition = encounter == WORLD_ENCOUNTER_ALOE
        ? LOGIC_OWN_LOW_HP : LOGIC_ENEMY_LOW_HP;
    screen->battle.formulas[1] = (LogicFormula){condition, LOGIC_OWN_RESERVE, LOGIC_AND};
    game->encounter = encounter;
    change_screen(game, GAME_BATTLE);
}

static void leave_encounter(Game *game)
{
    const BattleScreen *screen = &game->battle_screen;
    if (screen->setup_side < 0) {
        game->campaign.team = screen->battle.player_team;
        Team_Restore(&game->campaign.team);
        game->campaign.formula = screen->battle.formulas[0];
        game->campaign.has_team = true;
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
    srand((unsigned int)time(NULL));
    menu_screen_init(&game->menu);
    player_assets_load(&game->player_assets);
    trainer_select_assets_load(&game->trainer_select_assets);
    versus_assets_load(&game->versus_assets);
    versus_screen_init(&game->versus, VERSUS_CPU);
    world_init(&game->world);
    world_assets_load(&game->world_assets);
    trainer_select_screen_init(&game->trainer_select);
    name_entry_init(&game->name_entry);
    player_init(&game->player, TRAINER_MALE, world_spawn());
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
            game->campaign.has_team=false;
            world_init(&game->world);
            player_init(&game->player,game->campaign.trainer,world_spawn());
            world_snap_camera(&game->world, game->player.feet);
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
        if (game->screen == GAME_VERSUS_SELECT)
            versus_screen_update(&game->versus, false, GetFrameTime());
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
            if (action == MENU_ACTION_VERSUS) {
                versus_screen_init(&game->versus, game->versus_mode);
                change_screen(game, GAME_VERSUS_SELECT);
            }
            break;
        }
        case GAME_VERSUS_SELECT: {
            VersusAction action = versus_screen_update(&game->versus, true, GetFrameTime());
            if (action == VERSUS_BACK) change_screen(game, GAME_MENU);
            else if (action == VERSUS_CONFIRM) {
                game->versus_mode = game->versus.selected;
                game->versus_mode_selected = true;
                start_battle(game, true);
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
            if (game->world.dialogue_title) {
                world_update_dialogue(&game->world);
            } else if (game->world.relay_panel_open) {
                world_update_relay_panel(&game->world);
            } else if (IsKeyPressed(KEY_ESCAPE) || (game->world.completed && IsKeyPressed(KEY_ENTER))) {
                change_screen(game, GAME_MENU);
            } else if (!game->world.completed) {
                float dt = GetFrameTime();
                player_update(&game->player, &game->world, dt);
                world_follow_camera(&game->world, game->player.feet, dt);
                if (IsKeyPressed(KEY_E)) {
                    WorldEncounter encounter = world_interact(&game->world, &game->player.feet);
                    if (game->world.relay_panel_open || game->world.dialogue_title)
                        game->player.moving = false;
                    if (encounter != WORLD_ENCOUNTER_NONE) start_encounter(game, encounter);
                }
            }
            break;
        case GAME_BATTLE: {
            BattleScreenAction action = battle_screen_update(&game->battle_screen, GetFrameTime());
            if (action == BATTLE_SCREEN_TEAM_READY && game->battle_screen.versus_cpu) {
                BattleState *battle = &game->battle_screen.battle;
                battle->enemy_team = game->battle_screen.campaign_battle
                    ? guardian_team(game->encounter, battle->player_team.count)
                    : random_team(battle->player_team.count);
            }
            if (game->battle_screen.campaign_battle) {
                if (action == BATTLE_SCREEN_MENU || action == BATTLE_SCREEN_RESTART) leave_encounter(game);
            } else {
                if (action == BATTLE_SCREEN_RESTART) start_battle(game, false);
                if (action == BATTLE_SCREEN_MENU) change_screen(game, GAME_MENU);
            }
            break;
        }
        case GAME_NAME_ENTRY: break; /* Atualizada acima para manter os relógios contínuos. */
    }
}

void game_draw(const Game *game)
{
    switch (game->screen) {
        case GAME_MENU: menu_screen_draw(&game->menu); break;
        case GAME_VERSUS_SELECT:
            versus_screen_draw(&game->versus, &game->versus_assets, game->menu.logo, game->menu.logo_source);
            break;
        case GAME_BATTLE:
            battle_screen_draw(&game->battle_screen);
            break;
        case GAME_TRAINER_SELECT:
            trainer_select_screen_draw(&game->trainer_select, &game->trainer_select_assets);
            break;
        case GAME_NAME_ENTRY:
            name_entry_draw(&game->name_entry,&game->trainer_select,&game->trainer_select_assets);
            break;
        case GAME_EXPLORATION: {
            ClearBackground((Color){24, 49, 43, 255});
            Camera2D camera = world_camera(&game->world);
            BeginMode2D(camera);
            world_draw(&game->world, &game->world_assets, game->player.feet);
            player_draw(&game->player, &game->player_assets, camera);
            EndMode2D();
            world_draw_interface(&game->world, game->player.feet);
            if (!game->world.completed && !game->world.relay_panel_open && !game->world.dialogue_title &&
                game->player.trainer == TRAINER_MALE && !game->player_assets.botanical_sheets[TRAINER_MALE].id) {
                const char *version = game->player.use_walk6 && game->player_assets.male_walk6.id
                    ? "6 poses" : "4 poses";
                DrawRectangle(1022, 668, 234, 32, Fade(BLACK, 0.82f));
                DrawText(TextFormat("F6  Caminhada (%s)", version), 1033, 676, 16, RAYWHITE);
            }
            break;
        }
    }
}

void game_shutdown(Game *game)
{
    world_assets_unload(&game->world_assets);
    player_assets_unload(&game->player_assets);
    trainer_select_assets_unload(&game->trainer_select_assets);
    versus_assets_unload(&game->versus_assets);
    menu_screen_shutdown(&game->menu);
}
