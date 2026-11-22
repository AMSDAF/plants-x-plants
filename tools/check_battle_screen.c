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
    const int starters[] = {SPECIES_PEPPER,SPECIES_CLOVER,SPECIES_POISON_IVY,SPECIES_VENUS_FLYTRAP};
    const int rivals[] = {SPECIES_ALOE,SPECIES_CACTUS,SPECIES_ORCHID,SPECIES_SUNFLOWER};
    for (int i = 0; i < 4; i++) {
        assert(Team_SelectSpecies(&player,i,starters[i]));
        assert(Team_SelectSpecies(&enemy,i,rivals[i]));
    }
    game->screen = GAME_BATTLE;
    game->battle_screen = (BattleScreen){.setup_side=-1};
    Battle_Init(&game->battle_screen.battle,player,enemy);
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
    assert(key(&game,KEY_ESCAPE) == BATTLE_SCREEN_NONE && game.battle_screen.menu == BATTLE_MENU_ROOT);
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    click(&game,1050,545);
    assert(game.battle_screen.menu == BATTLE_MENU_TEAM);
    capture(&game,1920,1080,"team_four_1080p");
    click(&game,650,545); /* A ativa não pode ser trocada por ela mesma. */
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    click(&game,1050,545);
    assert(game.battle_screen.battle.player_team.active_index == 1 && game.battle_screen.battle.current_turn == 1);
    assert(game.battle_screen.scene == BATTLE_SCENE_DEPOT);
    capture(&game,1280,720,"cpu_turn");
    before = game.battle_screen.battle;
    key(&game,KEY_ONE);
    assert(memcmp(&before,&game.battle_screen.battle,sizeof(before)) == 0);
    game.battle_screen.cpu_wait = .65f; tick(&game);
    assert(game.battle_screen.battle.current_turn == 0 || game.battle_screen.battle.state == BATTLE_STATE_ENEMY_WIN);

    reset(&game);
    click(&game,650,545); click(&game,700,545);
    assert(game.battle_screen.battle.enemy_team.plants[0].current_hp < 110);
    assert(game.battle_screen.menu == BATTLE_MENU_ROOT && game.battle_screen.battle.current_turn == 1);

    reset(&game);
    BattleState *battle = &game.battle_screen.battle;
    battle->player_team.plants[0].current_hp = 0;
    battle->state = BATTLE_STATE_REPLACEMENT;
    before = *battle;
    key(&game,KEY_ONE);
    assert(game.battle_screen.menu == BATTLE_MENU_TEAM && memcmp(&before,battle,sizeof(before)) == 0);
    capture(&game,1920,1080,"forced_replacement_1080p");
    key(&game,KEY_W);
    assert(battle->state == BATTLE_STATE_ONGOING && battle->current_turn == 0 && battle->player_team.active_index == 1);
    before = *battle;
    key(&game,KEY_Q);
    assert(memcmp(&before,battle,sizeof(before)) == 0);

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
    capture(&game,1920,1080,"formula_1080p");
    key(&game,KEY_Q); key(&game,KEY_E); key(&game,KEY_TWO); key(&game,KEY_ENTER);
    assert(game.battle_screen.setup_side == -1 && game.battle_screen.battle.boosted_move_indices[0] == 1);
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
