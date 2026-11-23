/* Exercita os menus da arena com entrada raylib, motor real e capturas OpenGL. */
#include "game.h"
#include "game_canvas.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void event(unsigned int type, int first, int second)
{
    PlayAutomationEvent((AutomationEvent){.type=type,.params={first,second,0,0}});
}

static BattleScreenAction tick(Game *game)
{
    BattleScreenAction action = battle_screen_update(&game->battle_screen,1.0f/120);
    BeginDrawing(); game_draw(game); EndDrawing();
    return action;
}

static BattleScreenAction key(Game *game, int code)
{
    event(2,code,0); BattleScreenAction action = tick(game);
    event(1,code,0); tick(game);
    return action;
}

static void click(Game *game, int x, int y)
{
    event(7,x,y); event(6,0,0); tick(game);
    event(5,0,0); tick(game);
}

static void reset(Game *game)
{
    Team player = {0}, enemy = {0};
    const int starters[] = {SPECIES_PEPPER,SPECIES_CLOVER,SPECIES_POISON_IVY,SPECIES_CARNIVOROUS_PLANT};
    const int rivals[] = {SPECIES_ALOE,SPECIES_CACTUS,SPECIES_ORCHID,SPECIES_SUNFLOWER};
    for (int i = 0; i < 4; i++) {
        assert(Team_SelectSpecies(&player,i,starters[i]));
        assert(Team_SelectSpecies(&enemy,i,rivals[i]));
    }
    game->screen = GAME_BATTLE;
    game->battle_screen = (BattleScreen){.setup_side=-1};
    Battle_Init(&game->battle_screen.battle,player,enemy);
    for (int side = 0; side < 2; side++) for (int i = 0; i < MAX_TEAM_SIZE; i++) {
        game->battle_screen.battle.boosted_move_indices[side][i] = 0;
        game->battle_screen.battle.formulas[side][i] =
            (LogicFormula){LOGIC_OWN_RESERVE,LOGIC_ENEMY_STATUS,LOGIC_OR};
    }
    game->battle_screen.battle.player_team.plants[0].moves[0].accuracy = 100;
}

/* Avança o relógio simulado sem esperar dois segundos reais. */
static void resolve_action(Game *game)
{
    assert(game->battle_screen.pending_action != BATTLE_PENDING_NONE);
    for (int i = 0; i < 600 && game->battle_screen.pending_action != BATTLE_PENDING_NONE; i++)
        battle_screen_update(&game->battle_screen,.1f);
    assert(game->battle_screen.pending_action == BATTLE_PENDING_NONE);
}

/* Executa a primeira ação da rodada e para no beat da mensagem resultante. */
static void advance_to_result(Game *game)
{
    for (int i = 0; i < 40 && game->battle_screen.pending_action == BATTLE_PENDING_ROUND; i++)
        battle_screen_update(&game->battle_screen,.1f);
    assert(game->battle_screen.pending_action == BATTLE_PENDING_ROUND_RESULT);
}

static void capture(Game *game, int width, int height, const char *name)
{
    char path[256];
    snprintf(path,sizeof(path),"build/battle_ui_preview/%s.png",name);
    GameCanvas canvas = {0};
    assert(game_canvas_resize(&canvas,width,height,width,height));
    float previous = game->trainer_select_assets.ui.font_scale;
    selection_ui_update_fonts(&game->trainer_select_assets.ui,canvas.target.texture.width/1280.0f);
    Game before = *game;
    BeginDrawing(); game_canvas_begin(&canvas); game_draw(game); game_canvas_present(&canvas); EndDrawing();
    assert(memcmp(&before,game,sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas.target.texture);
    ImageFlipVertical(&image);
    assert(ExportImage(image,path));
    UnloadImage(image); game_canvas_unload(&canvas);
    selection_ui_update_fonts(&game->trainer_select_assets.ui,previous);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Verificação da arena botânica");
    SetTargetFPS(120); SetExitKey(KEY_NULL);
    assert(IsWindowReady());
    assert(DirectoryExists("build/battle_ui_preview") || MakeDirectory("build/battle_ui_preview") == 0);
    Game game; game_init(&game); reset(&game);
    for (int i = 0; i < BATTLE_ART_COUNT; i++) assert(game.battle_assets.textures[i].id);
    for (int i = 0; i < 2; i++) assert(game.battle_assets.bosses[i].id);
    game.battle_screen.battle.player_team.plants[0].current_hp = 68;
    capture(&game,1280,720,"depot_720p");
    capture(&game,1920,1080,"depot_1080p");
    capture(&game,1024,768,"depot_letterbox");
    snprintf(game.battle_screen.battle.last_message,sizeof(game.battle_screen.battle.last_message),
        "Hera Venenosa usou Cipó Ágil fortalecido: 94 de dano. Hera Venenosa não envenenou o atacante desta vez.");
    capture(&game,1920,1080,"long_message_1080p");
    BattleState before = game.battle_screen.battle;
    click(&game,700,615); click(&game,1050,615);
    assert(game.battle_screen.menu == BATTLE_MENU_ROOT);
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    key(&game,KEY_RIGHT); /* Retorna do último botão desabilitado ao primeiro. */
    key(&game,KEY_DOWN); key(&game,KEY_ENTER);
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    key(&game,KEY_UP); key(&game,KEY_ENTER);
    assert(game.battle_screen.menu == BATTLE_MENU_MOVES);
    capture(&game,1920,1080,"moves_1080p");
    LogicFormula saved_formula = game.battle_screen.battle.formulas[0][0];
    game.battle_screen.battle.formulas[0][0] = (LogicFormula){LOGIC_OWN_LOW_HP,LOGIC_ENEMY_LOW_HP,LOGIC_AND};
    capture(&game,1280,720,"logical_move_blocked");
    game.battle_screen.battle.formulas[0][0] = saved_formula;
    click(&game,450,534);
    assert(game.battle_screen.menu == BATTLE_MENU_ROOT);
    click(&game,650,545);
    assert(game.battle_screen.menu == BATTLE_MENU_MOVES);
    assert(key(&game,KEY_ESCAPE) == BATTLE_SCREEN_NONE && game.battle_screen.menu == BATTLE_MENU_ROOT);
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    click(&game,1050,545);
    assert(game.battle_screen.menu == BATTLE_MENU_TEAM);
    capture(&game,1920,1080,"team_four_1080p");
    click(&game,650,545); /* A ativa não pode ser trocada por ela mesma. */
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    click(&game,1050,545);
    assert(game.battle_screen.pending_action == BATTLE_PENDING_ROUND);
    assert(game.battle_screen.pending_player_action.type == BATTLE_ACTION_SWITCH);
    assert(game.battle_screen.pending_player_action.index == 1);
    assert(game.battle_screen.pending_enemy_action.type == BATTLE_ACTION_MOVE);
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    resolve_action(&game);
    assert(game.battle_screen.battle.player_team.active_index == 1 && game.battle_screen.battle.current_turn == 0);
    assert(game.battle_screen.scene == BATTLE_SCENE_DEPOT);
    before = game.battle_screen.battle;
    key(&game,KEY_ONE);
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    assert(game.battle_screen.pending_action == BATTLE_PENDING_ROUND);
    capture(&game,1280,720,"simultaneous_round");
    resolve_action(&game);
    assert(game.battle_screen.battle.current_turn == 0 || game.battle_screen.battle.state == BATTLE_STATE_ENEMY_WIN);

    reset(&game);
    click(&game,650,545); click(&game,700,545);
    assert(game.battle_screen.pending_action == BATTLE_PENDING_ROUND);
    assert(game.battle_screen.pending_player_action.type == BATTLE_ACTION_MOVE);
    assert(game.battle_screen.pending_player_action.index == 0);
    capture(&game,1280,720,"pending_move");
    before = game.battle_screen.battle;
    key(&game,KEY_TWO); key(&game,KEY_W);
    assert(game.battle_screen.pending_player_action.index == 0);
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    resolve_action(&game);
    assert(game.battle_screen.battle.enemy_team.plants[0].current_hp < game.battle_screen.battle.enemy_team.plants[0].max_hp);
    assert(game.battle_screen.menu == BATTLE_MENU_ROOT && game.battle_screen.battle.current_turn == 0);

    /* Golpe físico avança e volta; golpe de status pula; à distância fica parado. */
    reset(&game);
    key(&game,KEY_ONE);
    assert(game.battle_screen.animation_side == -1);
    advance_to_result(&game);
    assert(game.battle_screen.animation_side == 0);
    assert(game.battle_screen.animation_plant_index == 0);
    assert(!game.battle_screen.animation_status);
    for (int i = 0; i < 10; i++) battle_screen_update(&game.battle_screen,.1f);
    capture(&game,1280,720,"physical_lunge");
    resolve_action(&game);

    reset(&game);
    key(&game,KEY_TWO);
    advance_to_result(&game);
    assert(game.battle_screen.animation_side == 0);
    assert(game.battle_screen.animation_status);
    for (int i = 0; i < 10; i++) battle_screen_update(&game.battle_screen,.1f);
    capture(&game,1280,720,"status_hop");
    resolve_action(&game);

    reset(&game);
    game.battle_screen.battle.player_team.plants[0].fruit_count = 1;
    key(&game,KEY_THREE);
    advance_to_result(&game);
    assert(game.battle_screen.animation_side == -1);
    capture(&game,1280,720,"ranged_still");
    resolve_action(&game);

    reset(&game);
    BattleState *battle = &game.battle_screen.battle;
    battle->player_team.plants[0].current_hp = 0;
    battle->state = BATTLE_STATE_REPLACEMENT;
    before = *battle;
    key(&game,KEY_ONE);
    assert(game.battle_screen.menu == BATTLE_MENU_TEAM && memcmp(&before,battle,sizeof(before)) == 0);
    capture(&game,1920,1080,"forced_replacement_1080p");
    key(&game,KEY_W);
    resolve_action(&game);
    assert(battle->state == BATTLE_STATE_ONGOING && battle->current_turn == 0 && battle->player_team.active_index == 1);
    before = *battle;
    key(&game,KEY_Q);
    assert(memcmp(&before,battle,sizeof(before)) == 0);

    reset(&game);
    battle = &game.battle_screen.battle;
    battle->boosted_move_indices[0][0] = 2;
    before = *battle;
    key(&game,KEY_THREE); /* Sem fruto, não enfileira nem consome o turno. */
    assert(game.battle_screen.pending_action == BATTLE_PENDING_NONE);
    assert(memcmp(&before,battle,sizeof(before)) == 0);
    battle->player_team.plants[0].fruit_count = 1;
    key(&game,KEY_THREE);
    assert(game.battle_screen.pending_player_action.index == 2);
    resolve_action(&game);
    assert(battle->player_team.plants[0].fruit_count == 0);
    reset(&game);
    battle = &game.battle_screen.battle;
    battle->formulas[0][0] = (LogicFormula){LOGIC_OWN_LOW_HP,LOGIC_ENEMY_LOW_HP,LOGIC_AND};
    before = *battle;
    key(&game,KEY_ONE);
    assert(game.battle_screen.pending_action == BATTLE_PENDING_NONE);
    assert(memcmp(&before,battle,sizeof(before)) == 0);
    assert(Battle_ApplyStatus(battle,0,0,PLANT_STATUS_PARALYZED,1,1));
    tick(&game);
    assert(game.battle_screen.pending_action == BATTLE_PENDING_PARALYSIS);
    key(&game,KEY_TAB);
    assert(game.battle_screen.show_statuses);
    capture(&game,1920,1080,"status_overlay");
    resolve_action(&game);
    assert(battle->current_turn == 0 && !Plant_HasStatus(&battle->player_team.plants[0],PLANT_STATUS_PARALYZED));

    reset(&game);
    game.battle_screen.scene = BATTLE_SCENE_NURSERY;
    game.battle_screen.battle.enemy_team.plants[0] = Plant_CreateFromSpecies(SPECIES_SUNFLOWER);
    capture(&game,1920,1080,"nursery_1080p");
    for (int id = 0; id < SPECIES_COUNT; id++) {
        game.battle_screen.battle.player_team.plants[0] = Plant_CreateFromSpecies(id);
        game.battle_screen.battle.enemy_team.plants[0] = Plant_CreateFromSpecies(id);
        capture(&game,1280,720,TextFormat("species_%d",id));
    }
    game.battle_screen.setup_side = 0;
    game.battle_screen.draft = game.battle_screen.battle.formulas[0][0];
    capture(&game,1920,1080,"formula_1080p");
    int seen_status = 0, seen_faster = 0, seen_attack = 0, seen_defense = 0;
    game.battle_screen.draft.right = LOGIC_OWN_LOW_HP;
    for (int i = 0; i <= LOGIC_CONDITION_COUNT; i++) {
        key(&game,KEY_Q);
        if (game.battle_screen.draft.left == LOGIC_ENEMY_STATUS) seen_status = 1;
        if (game.battle_screen.draft.left == LOGIC_ENEMY_FASTER) seen_faster = 1;
        if (game.battle_screen.draft.left == LOGIC_OWN_HIGHER_ATTACK) seen_attack = 1;
        if (game.battle_screen.draft.left == LOGIC_OWN_HIGHER_DEFENSE) seen_defense = 1;
    }
    assert(seen_status && seen_faster && seen_attack && seen_defense);
    game.battle_screen.draft = game.battle_screen.battle.formulas[0][0];
    key(&game,KEY_Q); key(&game,KEY_E); key(&game,KEY_TWO);
    LogicFormula first = game.battle_screen.draft;
    key(&game,KEY_RIGHT);
    assert(game.battle_screen.draft_plant_index == 1);
    assert(memcmp(&first,&game.battle_screen.battle.formulas[0][0],sizeof(first)) == 0);
    key(&game,KEY_Q); key(&game,KEY_LEFT);
    assert(memcmp(&first,&game.battle_screen.draft,sizeof(first)) == 0);
    key(&game,KEY_ENTER);
    assert(game.battle_screen.setup_side == -1 && game.battle_screen.battle.boosted_move_indices[0][0] == 1);
    game.battle_screen.choosing_team = true; game.battle_screen.setup_side = 0;
    capture(&game,1280,720,"legacy_team_setup");
    game.battle_screen.choosing_team = false; game.battle_screen.setup_side = -1;
    BattleScreenAssets saved = game.battle_assets;
    memset(&game.battle_assets,0,sizeof(game.battle_assets));
    Texture2D pepper = game.intro_assets.plants[SPECIES_PEPPER];
    game.intro_assets.plants[SPECIES_PEPPER] = (Texture2D){0};
    game.battle_screen.battle.player_team.plants[0] = Plant_CreateFromSpecies(SPECIES_PEPPER);
    capture(&game,1280,720,"missing_assets");
    game.intro_assets.plants[SPECIES_PEPPER] = pepper;
    game.battle_assets = saved;
    SelectionUiAssets fonts = game.trainer_select_assets.ui;
    memset(game.trainer_select_assets.ui.fonts,0,sizeof(game.trainer_select_assets.ui.fonts));
    /* Não recriar os atlas ao capturar: usa a mesma escala lógica de 720p. */
    Game copy = game;
    BeginDrawing(); game_draw(&game); EndDrawing();
    assert(memcmp(&copy,&game,sizeof(game)) == 0);
    game.trainer_select_assets.ui = fonts;
    /* Nocaute encerra a fila de dois segundos sem atrasar o retorno. */
    reset(&game);
    battle = &game.battle_screen.battle;
    battle->boosted_move_indices[0][0] = -1;
    battle->player_team.plants[0].moves[0].accuracy = 100;
    battle->player_team.plants[0].moves[0].power = 10000;
    battle->enemy_team.count = 1;
    battle->enemy_team.active_index = 0;
    battle->enemy_team.plants[0].current_hp = 1;
    key(&game,KEY_ONE);
    int frames_after_end = -1;
    for (int i = 0; i < 80; i++) {
        battle_screen_update(&game.battle_screen,.1f);
        if (frames_after_end >= 0) frames_after_end++;
        else if (battle->state == BATTLE_STATE_PLAYER_WIN ||
                 battle->state == BATTLE_STATE_ENEMY_WIN) frames_after_end = 0;
        if (frames_after_end > 1) break;
    }
    assert(battle->state == BATTLE_STATE_PLAYER_WIN);
    assert(frames_after_end >= 0 && frames_after_end <= 2);
    assert(game.battle_screen.pending_action == BATTLE_PENDING_NONE);
    assert(!game.battle_screen.round_active);
    assert(key(&game,KEY_ENTER) == BATTLE_SCREEN_RETURN);
    for (int state = BATTLE_STATE_PLAYER_WIN; state <= BATTLE_STATE_ENEMY_WIN; state++) {
        game.battle_screen.battle.state = (BattleStateEnum)state;
        capture(&game,1280,720,state == BATTLE_STATE_PLAYER_WIN ? "victory" : "defeat");
        before = game.battle_screen.battle;
        key(&game,KEY_ONE); key(&game,KEY_Q);
        assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
        assert(key(&game,KEY_ENTER) == BATTLE_SCREEN_RETURN);
    }
    game_shutdown(&game);
    for (int i = 0; i < BATTLE_ART_COUNT; i++) assert(!game.battle_assets.textures[i].id);
    for (int i = 0; i < 2; i++) assert(!game.battle_assets.bosses[i].id);
    CloseWindow();
    puts("PASS: arena, dois locais, oito espécies, menus, placeholders, CPU, golpes, trocas, substituição, resultado, resoluções, fallback e desenho imutável.");
    return 0;
}
