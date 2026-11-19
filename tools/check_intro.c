/* Verifica a abertura com entradas reais, seleção limitada e integração com o combate. */
#include "game.h"
#include "plant_data.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void tick(Game *game)
{
    game_update(game);
    BeginDrawing();
    game_draw(game);
    EndDrawing();
}

static void event(unsigned int type, int value, int second)
{
    PlayAutomationEvent((AutomationEvent){.type = type, .params = {value, second, 0, 0}});
}

static void key(Game *game, int code)
{
    event(2, code, 0); tick(game);
    event(1, code, 0); tick(game); tick(game);
}

static void click(Game *game, int x, int y)
{
    event(7, x, y); event(6, 0, 0); tick(game);
    event(5, 0, 0); tick(game); tick(game);
}

static void capture(Game *game, const char *path)
{
    Game before = *game;
    RenderTexture2D target = LoadRenderTexture(1280, 720);
    BeginTextureMode(target); game_draw(game); EndTextureMode();
    assert(memcmp(&before, game, sizeof(before)) == 0);
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    assert(ExportImage(image, path));
    UnloadImage(image); UnloadRenderTexture(target);
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "Verificação da introdução");
    assert(IsWindowReady());
    SetExitKey(KEY_NULL);
    SetTargetFPS(120);
    MakeDirectory("build/intro_preview");
    Game game; game_init(&game);
    game.screen = GAME_NAME_ENTRY;
    game.name_entry.progress = 1;
    const char *name = "Teste";
    while (*name) assert(name_entry_append(&game.name_entry, *name++));
    key(&game, KEY_ENTER);
    assert(game.screen == GAME_INTRO && game.intro.stage == INTRO_LETTER);
    assert(!game.campaign.has_team && game.campaign.team.count == 0);
    /* ENTER mantido na transição não pode aceitar a carta. */
    event(2, KEY_ENTER, 0); tick(&game);
    assert(game.intro.stage == INTRO_LETTER);
    event(1, KEY_ENTER, 0); tick(&game); tick(&game);
    game.intro.reveal = 10000;
    capture(&game, "build/intro_preview/01_letter.png");
    key(&game, KEY_ENTER);
    assert(game.intro.stage == INTRO_ACCEPT);
    key(&game, KEY_ENTER); /* Completa o texto sem avançar. */
    assert(game.intro.stage == INTRO_ACCEPT);
    capture(&game, "build/intro_preview/02_accept.png");
    key(&game, KEY_ENTER);
    assert(game.intro.stage == INTRO_STARTERS);
    key(&game, KEY_ENTER);
    assert(game.intro.stage == INTRO_STARTERS && game.intro.count == 0);
    /* Todas as seis candidatas podem ser selecionadas e removidas. */
    for (int i = 0; i < INTRO_STARTER_COUNT; i++) {
        game.intro.cursor = i;
        key(&game, KEY_SPACE); assert(game.intro.count == 1);
        key(&game, KEY_SPACE); assert(game.intro.count == 0);
    }
    click(&game, 150, 210);
    click(&game, 500, 210);
    key(&game, KEY_ENTER); assert(game.intro.stage == INTRO_STARTERS);
    click(&game, 850, 210);
    assert(game.intro.count == INTRO_TEAM_SIZE);
    click(&game, 150, 410); /* Uma quarta planta não pode entrar. */
    assert(game.intro.count == INTRO_TEAM_SIZE);
    Team team = {0};
    assert(intro_screen_team(&game.intro, &team) && team.count == INTRO_TEAM_SIZE);
    assert(team.plants[0].id == SPECIES_CACTUS && team.plants[2].id == SPECIES_ALOE);
    IntroScreen invalid = game.intro;
    invalid.selected[1] = invalid.selected[0];
    assert(!intro_screen_team(&invalid, &team));
    capture(&game, "build/intro_preview/03_starters.png");
    key(&game, KEY_ENTER); assert(game.intro.stage == INTRO_TRAVEL);
    key(&game, KEY_ESCAPE);
    assert(game.intro.stage == INTRO_STARTERS && game.intro.count == INTRO_TEAM_SIZE);
    key(&game, KEY_ENTER);
    key(&game, KEY_ENTER);
    capture(&game, "build/intro_preview/04_travel.png");
    key(&game, KEY_ENTER);
    assert(game.screen == GAME_EXPLORATION && game.campaign.has_team);
    assert(game.campaign.team.count == INTRO_TEAM_SIZE && !game.campaign.has_formula);
    capture(&game, "build/intro_preview/05_arrival.png");
    /* Abre o guardião do depósito pelo mesmo caminho usado pela exploração. */
    game.world.location = WORLD_DEPOT;
    game.player.feet = (Vector2){640, 420};
    key(&game, KEY_E);
    assert(game.screen == GAME_BATTLE && !game.battle_screen.choosing_team);
    assert(game.battle_screen.battle.player_team.count == INTRO_TEAM_SIZE);
    assert(game.battle_screen.battle.enemy_team.count == INTRO_TEAM_SIZE);
    assert(game.battle_screen.setup_side == 0);
    key(&game, KEY_ENTER);
    assert(game.battle_screen.setup_side == -1);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_EXPLORATION && game.campaign.has_formula);
    assert(game.campaign.team.count == INTRO_TEAM_SIZE);
    key(&game, KEY_ESCAPE);
    key(&game, KEY_ENTER); key(&game, KEY_ENTER);
    for (int i = 0; i < 90; i++) tick(&game);
    key(&game, KEY_ENTER);
    assert(game.screen == GAME_INTRO && game.intro.count == 0 && !game.campaign.has_team);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_NAME_ENTRY && strcmp(game.name_entry.draft, "Teste") == 0);
    game_shutdown(&game); CloseWindow();
    puts("Introdução: fluxo, escolhas, retorno, sessão e primeiro combate verificados.");
    return 0;
}
