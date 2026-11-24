/* Verifica telas finais, entradas reais e preservação do progresso ao perder. */
#include "game.h"
#include "game_canvas.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void event(unsigned int type, int a, int b)
{
    PlayAutomationEvent((AutomationEvent){.type=type,.params={a,b,0,0}});
}
static void tick(Game *game)
{
    game_update(game);
    BeginDrawing(); ClearBackground(BLACK); EndDrawing();
}
static void key(Game *game, int code)
{
    event(2,code,0); tick(game);
    event(1,code,0); tick(game); tick(game);
}
static void capture(Game *game, int width, int height, const char *name)
{
    GameCanvas canvas = {0};
    assert(game_canvas_resize(&canvas,width,height,width,height));
    selection_ui_update_fonts(&game->trainer_select_assets.ui,canvas.target.texture.width/1280.0f);
    Game before = *game;
    BeginDrawing(); game_canvas_begin(&canvas); game_draw(game);
    game_canvas_present(&canvas); EndDrawing();
    assert(memcmp(&before,game,sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas.target.texture); ImageFlipVertical(&image);
    assert(ExportImage(image,TextFormat("build/result_preview/%s.png",name)));
    UnloadImage(image); game_canvas_unload(&canvas);
}
static void lost_battle(Game *game, WorldEncounter encounter)
{
    game->screen = GAME_BATTLE; game->input_blocked = false; game->encounter = encounter;
    game->battle_screen = (BattleScreen){.setup_side=-1};
    Team enemy = {0};
    Team_SelectSpecies(&enemy,0,encounter == WORLD_ENCOUNTER_ALOE ? SPECIES_ALOE : SPECIES_SUNFLOWER);
    Battle_Init(&game->battle_screen.battle,game->campaign.team,enemy);
    BattleState *battle = &game->battle_screen.battle;
    for (int i = 0; i < battle->player_team.count; i++) battle->player_team.plants[i].current_hp = 0;
    battle->state = BATTLE_STATE_ENEMY_WIN;
    battle->formulas[0][0] = (LogicFormula){LOGIC_OWN_LOW_HP,LOGIC_ENEMY_STATUS,LOGIC_AND};
    battle->boosted_move_indices[0][0] = 0;
    key(game,KEY_ENTER); /* A entrada que encerrou a ação não confirma o resultado. */
    assert(game->screen == GAME_DEFEAT && game->encounter == encounter);
}
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN); SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Resultados da campanha"); SetExitKey(KEY_NULL); SetTargetFPS(120);
    assert(IsWindowReady());
    assert(DirectoryExists("build/result_preview") || MakeDirectory("build/result_preview") == 0);
    Game game = {0};
    game.audio.current_music = game.audio.target_music = MUSIC_NONE;
    ui_theme_load(&game.ui);
    selection_ui_assets_load(&game.trainer_select_assets.ui);
    trainer_background_load(&game.trainer_select_assets.background);
    intro_assets_load(&game.intro_assets);
    strcpy(game.campaign.player_name,"WWWWWWWWWWWW");
    game.campaign.has_team = true;
    Team_SelectSpecies(&game.campaign.team,0,SPECIES_CLOVER);
    Team_SelectSpecies(&game.campaign.team,1,SPECIES_CARNIVOROUS_PLANT);
    Team_SelectSpecies(&game.campaign.team,2,SPECIES_ORCHID);
    world_init(&game.world); game.world.maintenance_key = true;
    game.world.light = true; game.world.relay_step = 1; game.world.logic_move_mask = 1;
    lost_battle(&game,WORLD_ENCOUNTER_ALOE);
    capture(&game,1280,720,"defeat_long_name");
    strcpy(game.campaign.player_name,"Marcos");
    capture(&game,1280,720,"defeat");
    capture(&game,1920,1080,"defeat_1080p");
    key(&game,KEY_ENTER);
    assert(game.screen == GAME_BATTLE && game.encounter == WORLD_ENCOUNTER_ALOE);
    assert(game.battle_screen.battle.state == BATTLE_STATE_ONGOING);
    assert(game.battle_screen.battle.player_team.count == 3);
    assert(game.battle_screen.battle.enemy_team.plants[0].id == SPECIES_ALOE);
    assert(game.battle_screen.battle.boosted_move_indices[0][0] == 0);
    for (int i = 0; i < 3; i++) {
        Plant *plant = &game.battle_screen.battle.player_team.plants[i];
        assert(plant->current_hp == plant->max_hp);
    }
    assert(game.world.maintenance_key && game.world.light && game.world.relay_step == 1);
    lost_battle(&game,WORLD_ENCOUNTER_SUNFLOWER);
    key(&game,KEY_ENTER);
    assert(game.screen == GAME_BATTLE && game.battle_screen.battle.enemy_team.plants[0].id == SPECIES_SUNFLOWER);
    lost_battle(&game,WORLD_ENCOUNTER_SUNFLOWER);
    key(&game,KEY_RIGHT); assert(game.result.cursor == 1);
    key(&game,KEY_ENTER);
    assert(game.screen == GAME_EXPLORATION && game.world.location == WORLD_EXTERIOR);
    assert(game.encounter == WORLD_ENCOUNTER_NONE && !game.world.guardian_defeated);
    assert(game.world.maintenance_key && game.world.light && game.world.logic_move_mask == 1);
    assert(game.campaign.team.plants[0].current_hp == game.campaign.team.plants[0].max_hp);
    game.world.completed = true; game.world.dialogue_title = "Íris";
    tick(&game); assert(game.screen == GAME_EXPLORATION);
    game.world.dialogue_title = NULL; tick(&game);
    assert(game.screen == GAME_ENDING && game.input_blocked);
    tick(&game);
    capture(&game,1280,720,"ending"); capture(&game,1920,1080,"ending_1080p");
    capture(&game,1024,768,"ending_letterbox");
    SelectionUiAssets saved_fonts = game.trainer_select_assets.ui;
    IntroAssets saved_plants = game.intro_assets;
    game.trainer_select_assets.ui = (SelectionUiAssets){0}; game.intro_assets = (IntroAssets){0};
    capture(&game,1280,720,"missing_assets");
    selection_ui_assets_unload(&game.trainer_select_assets.ui);
    game.trainer_select_assets.ui = saved_fonts; game.intro_assets = saved_plants;
    key(&game,KEY_ESCAPE); assert(game.screen == GAME_MENU);
    game.screen = GAME_ENDING; game.input_blocked = false; result_screen_begin(&game.result);
    Rectangle button = result_screen_button_bounds(0);
    event(7,(int)(button.x+button.width/2),(int)(button.y+button.height/2)); event(6,0,0); tick(&game);
    event(5,0,0); tick(&game); tick(&game);
    assert(game.screen == GAME_LEADERBOARD);
    key(&game,KEY_ENTER);
    assert(game.screen == GAME_TRAINER_SELECT);
    /* ESC só abandona a sessão depois de confirmação explícita. */
    game.screen = GAME_EXPLORATION; game.input_blocked = false;
    game.world.completed = false;
    World preserved = game.world;
    key(&game,KEY_ESCAPE);
    assert(game.exit_confirm_open && game.exit_confirm_cursor == 0);
    assert(game.screen == GAME_EXPLORATION);
    assert(memcmp(&preserved,&game.world,sizeof(preserved)) == 0);
    key(&game,KEY_ENTER); /* A opção segura mantém a partida. */
    assert(!game.exit_confirm_open && game.screen == GAME_EXPLORATION);
    assert(memcmp(&preserved,&game.world,sizeof(preserved)) == 0);
    key(&game,KEY_ESCAPE); key(&game,KEY_ESCAPE);
    assert(!game.exit_confirm_open && game.screen == GAME_EXPLORATION);
    /* Os painéis internos continuam fechando com ESC, sem pedir saída. */
    game.world.relay_panel_open = true;
    key(&game,KEY_ESCAPE);
    assert(!game.world.relay_panel_open && !game.exit_confirm_open);
    game.screen = GAME_BATTLE; game.input_blocked = false;
    game.battle_screen.menu = BATTLE_MENU_ROOT;
    game.battle_screen.battle.state = BATTLE_STATE_ONGOING;
    game.battle_screen.pending_action = BATTLE_PENDING_MOVE;
    game.battle_screen.action_timer = 10;
    key(&game,KEY_ESCAPE);
    assert(game.exit_confirm_open && game.screen == GAME_BATTLE);
    BattleScreen paused = game.battle_screen;
    for (int i = 0; i < 4; i++) tick(&game);
    assert(memcmp(&paused,&game.battle_screen,sizeof(paused)) == 0);
    capture(&game,1280,720,"exit_confirmation");
    capture(&game,1920,1080,"exit_confirmation_1080p");
    key(&game,KEY_ESCAPE);
    assert(!game.exit_confirm_open && game.screen == GAME_BATTLE);
    assert(game.battle_screen.action_timer <= paused.action_timer);
    game.battle_screen.pending_action = BATTLE_PENDING_NONE;
    game.battle_screen.menu = BATTLE_MENU_MOVES;
    key(&game,KEY_ESCAPE);
    assert(!game.exit_confirm_open && game.battle_screen.menu == BATTLE_MENU_ROOT);
    key(&game,KEY_ESCAPE);
    key(&game,KEY_RIGHT);
    assert(game.exit_confirm_open && game.exit_confirm_cursor == 1);
    /* Confirmação por clique. */
    event(7,820,440); event(6,0,0); tick(&game);
    event(5,0,0); tick(&game); tick(&game);
    assert(!game.exit_confirm_open && game.screen == GAME_MENU);
    /* Vitória da Babosa entrega a chave e abre o aviso no estilo das placas. */
    game.screen = GAME_BATTLE; game.input_blocked = false;
    game.encounter = WORLD_ENCOUNTER_ALOE;
    game.battle_screen.setup_side = -1;
    game.battle_screen.battle.state = BATTLE_STATE_PLAYER_WIN;
    game.battle_screen.fx.active = false;
    world_init(&game.world); game.world.location = WORLD_DEPOT;
    key(&game,KEY_ENTER);
    assert(game.screen == GAME_EXPLORATION && game.world.maintenance_key);
    assert(game.world.dialogue_title && strcmp(game.world.dialogue_title,"Chave do alçapão") == 0);
    assert(strstr(game.world.dialogue_pages[0],"derrotou a Babosa"));
    assert(strstr(game.world.dialogue_pages[0],"Chave da Manutenção"));
    assert(strstr(game.world.dialogue_pages[0],"casa de manutenção"));
    assert(!game.world.dialogue_pages[1]);
    key(&game,KEY_ESCAPE);
    assert(!game.world.dialogue_title && !game.exit_confirm_open && game.world.maintenance_key);
    world_finish_encounter(&game.world,WORLD_ENCOUNTER_ALOE);
    assert(!game.world.dialogue_title); /* Recompensa não se repete. */
    intro_assets_unload(&game.intro_assets);
    trainer_background_unload(&game.trainer_select_assets.background);
    selection_ui_assets_unload(&game.trainer_select_assets.ui);
    ui_theme_unload(&game.ui); CloseWindow();
    puts("PASS: derrota, revanche nos dois bosses, equipe recuperada, fórmulas/progresso preservados, diálogo final, menu/reinício, mouse/teclado, resoluções, reserva, confirmação de saída, pausa/cancelamento e desenho imutável.");
    return 0;
}
