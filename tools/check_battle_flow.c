/* Confirma avanço híbrido, proteção de entrada e apresentação da ordem real. */
#include "game.h"
#include "game_canvas.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void event(unsigned int type, int key)
{ PlayAutomationEvent((AutomationEvent){.type=type,.params={key,0,0,0}}); }
static void tick(Game *game)
{
    battle_screen_update(&game->battle_screen,.1f);
    BeginDrawing(); ClearBackground(BLACK); EndDrawing();
}
static void key(Game *game, int code)
{
    event(2,code); tick(game); event(1,code); tick(game);
}
static void prepare(Game *game, int player_move, int enemy_move, bool enemy_first)
{
    Team a = {0}, b = {0};
    Team_SelectSpecies(&a,0,SPECIES_CLOVER); Team_SelectSpecies(&a,1,SPECIES_CACTUS);
    Team_SelectSpecies(&b,0,SPECIES_ALOE); Team_SelectSpecies(&b,1,SPECIES_SUNFLOWER);
    a.plants[0].max_hp = a.plants[0].current_hp = 500;
    b.plants[0].max_hp = b.plants[0].current_hp = 500;
    a.plants[0].speed = enemy_first ? 10 : 100;
    b.plants[0].speed = enemy_first ? 100 : 10;
    game->screen = GAME_BATTLE; game->input_blocked = false;
    game->battle_screen = (BattleScreen){.setup_side=-1,.menu=BATTLE_MENU_MOVES};
    BattleScreen *screen = &game->battle_screen;
    Battle_Init(&screen->battle,a,b);
    /* A inicialização restaura atributos; a ordem de teste é explicitamente controlada. */
    screen->battle.player_team.plants[0].speed = enemy_first ? 10 : 100;
    screen->battle.enemy_team.plants[0].speed = enemy_first ? 100 : 10;
    screen->battle.player_team.plants[0].moves[player_move].accuracy = 100;
    screen->battle.enemy_team.plants[0].moves[enemy_move].accuracy = 100;
    screen->battle.player_team.plants[0].moves[player_move].critical_chance = 0;
    screen->battle.enemy_team.plants[0].moves[enemy_move].critical_chance = 0;
    assert(Battle_PrepareRound(&screen->battle,(BattleAction){BATTLE_ACTION_MOVE,player_move},
        (BattleAction){BATTLE_ACTION_MOVE,enemy_move},&screen->round));
    screen->round_active = true;
    screen->pending_action = BATTLE_PENDING_ROUND; screen->action_timer = .15f;
}
static void first_result(Game *game)
{
    for (int i = 0; i < 20 && game->battle_screen.pending_action != BATTLE_PENDING_ROUND_RESULT; i++) tick(game);
    assert(game->battle_screen.pending_action == BATTLE_PENDING_ROUND_RESULT);
}
static void finish_round(Game *game)
{
    for (int i = 0; i < 300 && game->battle_screen.pending_action != BATTLE_PENDING_NONE; i++) {
        tick(game);
        if (game->battle_screen.pending_action == BATTLE_PENDING_ROUND_RESULT &&
            !game->battle_screen.fx.active && game->battle_screen.message_armed) key(game,KEY_ENTER);
    }
    assert(game->battle_screen.pending_action == BATTLE_PENDING_NONE);
}
static void capture(Game *game, const char *name)
{
    GameCanvas canvas = {0}; assert(game_canvas_resize(&canvas,1280,720,1280,720));
    Game before = *game;
    BeginDrawing(); game_canvas_begin(&canvas); game_draw(game); game_canvas_present(&canvas); EndDrawing();
    assert(memcmp(&before,game,sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas.target.texture); ImageFlipVertical(&image);
    assert(ExportImage(image,TextFormat("build/battle_flow_preview/%s.png",name)));
    UnloadImage(image); game_canvas_unload(&canvas);
}
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN); SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Fluxo manual da rodada"); SetExitKey(KEY_NULL); SetTargetFPS(120);
    assert(DirectoryExists("build/battle_flow_preview") || MakeDirectory("build/battle_flow_preview") == 0);
    Game game = {0}; ui_theme_load(&game.ui); selection_ui_assets_load(&game.trainer_select_assets.ui);
    intro_assets_load(&game.intro_assets); battle_screen_assets_load(&game.battle_assets);
    prepare(&game,0,0,false);
    event(2,KEY_ENTER); first_result(&game);
    for (int i = 0; i < 12; i++) tick(&game);
    assert(!game.battle_screen.fx.active && !game.battle_screen.message_armed);
    assert(game.battle_screen.round_message_index == 0 && game.battle_screen.round.step == 1);
    event(1,KEY_ENTER); tick(&game);
    BattleState saved = game.battle_screen.battle;
    for (int i = 0; i < 5; i++) battle_screen_update(&game.battle_screen,.1f);
    assert(memcmp(&saved,&game.battle_screen.battle,sizeof(saved)) == 0);
    assert(game.battle_screen.round_message_index == 0);
    assert(game.battle_screen.round.sides[0] == 0);
    capture(&game,"first_action_waiting");
    key(&game,KEY_SPACE);
    assert(game.battle_screen.round_message_index == 1);
    assert(strstr(game.battle_screen.round_result_message+game.battle_screen.round_message_offsets[1],"recebeu"));
    capture(&game,"damage_waiting");
    int page = game.battle_screen.round_message_index;
    float elapsed = game.battle_screen.message_elapsed;
    game.battle_screen.show_statuses = true;
    for (int i = 0; i < 30; i++) tick(&game);
    assert(game.battle_screen.round_message_index == page && game.battle_screen.message_elapsed == elapsed);
    game.battle_screen.show_statuses = false;
    for (int i = 0; i < 16; i++) tick(&game);
    assert(game.battle_screen.round_message_index != page || game.battle_screen.pending_action != BATTLE_PENDING_ROUND_RESULT);
    event(6,0); tick(&game); event(5,0); tick(&game); /* Clique continua disponível. */
    finish_round(&game);
    assert(!game.battle_screen.round_active);
    prepare(&game,2,2,true); first_result(&game);
    assert(game.battle_screen.round.sides[0] == 1);
    for (int i = 0; i < 12; i++) tick(&game);
    key(&game,KEY_ENTER);
    assert(strstr(game.battle_screen.round_result_message+game.battle_screen.round_message_offsets[1],"Defesa"));
    capture(&game,"enemy_buff"); finish_round(&game);
    prepare(&game,0,0,false);
    game.battle_screen.battle.player_team.plants[0].moves[0].accuracy = 0;
    first_result(&game); for (int i = 0; i < 12; i++) tick(&game);
    key(&game,KEY_ENTER);
    assert(strstr(game.battle_screen.round_result_message+game.battle_screen.round_message_offsets[1],"não acertou"));
    finish_round(&game);
    /* A rodada também termina sem entrada, com 1,5 s para cada página. */
    prepare(&game,0,0,false);
    for (int i = 0; i < 200 && game.battle_screen.pending_action != BATTLE_PENDING_NONE; i++) tick(&game);
    assert(game.battle_screen.pending_action == BATTLE_PENDING_NONE && !game.battle_screen.round_active);
    /* O último golpe conserva suas páginas durante o intervalo de leitura. */
    prepare(&game,0,0,true);
    game.battle_screen.battle.player_team.count = 1;
    game.battle_screen.battle.player_team.plants[0].current_hp = 1;
    game.battle_screen.battle.enemy_team.plants[0].moves[0].power = 999;
    first_result(&game);
    assert(game.battle_screen.battle.state == BATTLE_STATE_ENEMY_WIN);
    for (int i = 0; i < 20; i++) tick(&game);
    game_update(&game); assert(game.screen == GAME_BATTLE);
    capture(&game,"final_hit_waiting"); finish_round(&game);
    game_update(&game); assert(game.screen == GAME_DEFEAT);
    battle_screen_assets_unload(&game.battle_assets); intro_assets_unload(&game.intro_assets);
    selection_ui_assets_unload(&game.trainer_select_assets.ui); ui_theme_unload(&game.ui); CloseWindow();
    puts("PASS: avanço automático em 1,5 s após animação, consulta pausa leitura, ENTER/Espaço/clique, entrada mantida, ordem real, dano/buff/erro, fim da rodada e derrota após confirmação.");
    return 0;
}
