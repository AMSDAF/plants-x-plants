/* Exercita todos os golpes, resultados reais, áudio agendado e desenho imutável. */
#include "game.h"
#include "game_audio.h"
#include "game_canvas.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void prepare(Game *game, int species, int side)
{
    Team a = {0}, b = {0};
    Team_SelectSpecies(&a,0,side == 0 ? species : SPECIES_ALOE);
    Team_SelectSpecies(&b,0,side == 1 ? species : SPECIES_ALOE);
    assert(Team_SelectSpecies(&a,1,a.plants[0].id == SPECIES_CACTUS ? SPECIES_ORCHID : SPECIES_CACTUS));
    assert(Team_SelectSpecies(&b,1,b.plants[0].id == SPECIES_CACTUS ? SPECIES_ORCHID : SPECIES_CACTUS));
    game->screen = GAME_BATTLE;
    game->battle_screen = (BattleScreen){.setup_side=-1,.animation_side=-1};
    Battle_Init(&game->battle_screen.battle,a,b);
    BattleState *battle = &game->battle_screen.battle;
    battle->current_turn = side;
    battle->player_team.plants[0].max_hp = battle->player_team.plants[0].current_hp = 500;
    battle->enemy_team.plants[0].max_hp = battle->enemy_team.plants[0].current_hp = 500;
    battle->player_team.plants[0].fruit_count = 3;
    battle->enemy_team.plants[0].fruit_count = 3;
    game->audio = (GameAudio){.ready=true,.current_music=MUSIC_NONE,.target_music=MUSIC_NONE};
}
static void execute(Game *game, int index, bool miss)
{
    BattleState *battle = &game->battle_screen.battle;
    Team *team = battle->current_turn == 0 ? &battle->player_team : &battle->enemy_team;
    team->plants[0].moves[index].accuracy = miss ? 0 : 100;
    team->plants[0].moves[index].critical_chance = 100;
    BattleState before = *battle;
    GameAudioSnapshot shot = game_audio_capture(game);
    assert(Battle_ExecuteMove(battle,index));
    battle_fx_begin(&game->battle_screen.fx,&before,battle);
    game_audio_react(game,&shot);
    assert(battle->feedback.sequence == before.feedback.sequence+1);
    assert(game->battle_screen.fx.active);
    assert(battle->feedback.hit == !miss);
}
static bool scheduled(const GameAudio *audio, SoundId id)
{
    for (int i = 0; i < AUDIO_DELAYED_CAPACITY; i++)
        if (audio->delayed[i].active && audio->delayed[i].id == id) return true;
    return false;
}
static void capture(Game *game, const char *name, float time, int width, int height)
{
    game->battle_screen.fx.elapsed = time;
    GameCanvas canvas = {0}; assert(game_canvas_resize(&canvas,width,height,width,height));
    selection_ui_update_fonts(&game->trainer_select_assets.ui,canvas.target.texture.width/1280.0f);
    Game before = *game;
    srand(2468); int expected = rand(); srand(2468);
    BeginDrawing(); game_canvas_begin(&canvas); game_draw(game); game_canvas_present(&canvas); EndDrawing();
    assert(rand() == expected); /* Efeitos não alteram a sequência aleatória do combate. */
    assert(memcmp(&before,game,sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas.target.texture); ImageFlipVertical(&image);
    assert(ExportImage(image,TextFormat("build/battle_fx_preview/%s.png",name)));
    UnloadImage(image); game_canvas_unload(&canvas);
}
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN); SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Efeitos do combate"); SetExitKey(KEY_NULL); SetTargetFPS(120);
    assert(IsWindowReady());
    assert(DirectoryExists("build/battle_fx_preview") || MakeDirectory("build/battle_fx_preview") == 0);
    Game game = {0}; ui_theme_load(&game.ui);
    selection_ui_assets_load(&game.trainer_select_assets.ui);
    intro_assets_load(&game.intro_assets); battle_screen_assets_load(&game.battle_assets);
    for (int side = 0; side < 2; side++) for (int species = 0; species < SPECIES_COUNT; species++) {
        for (int move = 0; move < MAX_MOVES; move++) {
            prepare(&game,species,side); execute(&game,move,false);
            BattleState saved = game.battle_screen.battle;
            BattleFx *fx = &game.battle_screen.fx;
            if (fx->move.damage > 0) {
                assert(fx->move.critical);
                assert(scheduled(&game.audio,SFX_CRITICAL));
                int target = fx->move.target_side;
                assert(battle_fx_hp(fx,target,0,fx->hp_after[target][0]) == fx->hp_before[target][0]);
                fx->elapsed = 0.60f;
                float shown = battle_fx_hp(fx,target,0,fx->hp_after[target][0]);
                assert(shown >= fx->hp_after[target][0] && shown <= fx->hp_before[target][0]);
            }
            char name[64]; snprintf(name,sizeof(name),"side%d_species%d_move%d",side,species,move);
            if (side == 0) capture(&game,name,fx->kind == BATTLE_FX_BUFF || fx->kind == BATTLE_FX_DEBUFF ? .55f : .26f,1280,720);
            for (int frame = 0; frame < 15; frame++) battle_fx_update(fx,.1f);
            assert(!fx->active && memcmp(&saved,&game.battle_screen.battle,sizeof(saved)) == 0);
        }
    }
    prepare(&game,SPECIES_PEPPER,0); execute(&game,2,true);
    assert(!game.battle_screen.fx.move.critical && !game.battle_screen.fx.move.damage);
    assert(scheduled(&game.audio,SFX_MISS) && !scheduled(&game.audio,SFX_CRITICAL));
    capture(&game,"miss",.55f,1280,720);
    prepare(&game,SPECIES_CLOVER,0);
    game.battle_screen.battle.boosted_move_indices[0][0] = 0;
    game.battle_screen.battle.formulas[0][0] = (LogicFormula){LOGIC_OWN_RESERVE,LOGIC_ENEMY_LOW_HP,LOGIC_OR};
    execute(&game,0,false); assert(game.battle_screen.fx.move.boosted);
    capture(&game,"boosted",.18f,1280,720);
    capture(&game,"critical",.47f,1920,1080);
    BattleState saved = game.battle_screen.battle;
    assert(!Battle_ExecuteMove(&game.battle_screen.battle,MAX_MOVES));
    assert(memcmp(&saved,&game.battle_screen.battle,sizeof(saved)) == 0);
    /* Último impacto termina antes da tela de derrota. */
    game.battle_screen.battle.state = BATTLE_STATE_ENEMY_WIN;
    game.battle_screen.fx.elapsed = 0;
    game.input_blocked = false; game.audio.ready = false;
    game_update(&game); assert(game.screen == GAME_BATTLE);
    for (int i = 0; i < 12; i++) battle_screen_update(&game.battle_screen,.1f);
    game_update(&game); assert(game.screen == GAME_DEFEAT);
    /* Sons gerados são carregados/liberados pelo mesmo ciclo dos WAV existentes. */
    InitAudioDevice(); audio_init(&game.audio);
    if (game.audio.ready) {
        for (int i = SFX_ATTACK_LEAF; i < SFX_COUNT; i++) assert(game.audio.sound_loaded[i]);
        srand(321); int expected = rand(); srand(321);
        audio_play(&game.audio,SFX_ATTACK_FIRE); audio_play(&game.audio,SFX_CRITICAL);
        assert(rand() == expected);
        assert(game.audio.impact_duck > 0);
        audio_shutdown(&game.audio);
        assert(!game.audio.ready);
        puts("PASS: recursos de áudio sintetizados, variação independente do RNG e redução da música.");
    } else puts("SKIP: dispositivo de áudio indisponível.");
    CloseAudioDevice();
    battle_screen_assets_unload(&game.battle_assets); intro_assets_unload(&game.intro_assets);
    selection_ui_assets_unload(&game.trainer_select_assets.ui); ui_theme_unload(&game.ui);
    CloseWindow();
    puts("PASS: 24 golpes dos dois lados, erro/crítico/fortalecido, HP visual, eventos inválidos, último impacto e desenho imutável.");
    return 0;
}
