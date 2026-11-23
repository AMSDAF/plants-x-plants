/* Verifica a abertura com entradas reais, seleção limitada e integração com o combate. */
#include "game.h"
#include "plant_data.h"
#include "game_canvas.h"
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

/* Confere o indicador visível nas seis telas, inclusive nas capturas redimensionadas. */
static void check_progress_image(const Game *game, Image image)
{
    int filled = game->screen == GAME_TRAINER_SELECT ? 1 : game->screen == GAME_NAME_ENTRY ? 2
                   : game->screen == GAME_INTRO ? 3 + (int)game->intro.stage : 0;
    if (!filled) return;
    float scale = image.width / 1280.0f;
    for (int i = 0; i < 6; i++) {
        Color expected = i < filled ? (Color){244,133,25,255} : (Color){253,245,216,255};
        Color actual = GetImageColor(image, (int)((520 + i*48)*scale), (int)(106*scale));
        assert(actual.r == expected.r && actual.g == expected.g && actual.b == expected.b);
    }
}

static void capture(Game *game, const char *path)
{
    Game before = *game;
    RenderTexture2D target = LoadRenderTexture(1280, 720);
    BeginTextureMode(target); game_draw(game); EndTextureMode();
    assert(memcmp(&before, game, sizeof(before)) == 0);
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    check_progress_image(game, image);
    assert(ExportImage(image, path));
    UnloadImage(image); UnloadRenderTexture(target);
}

static void capture_scaled(Game *game, int width, int height, const char *path)
{
    GameCanvas canvas = {0};
    assert(game_canvas_resize(&canvas, width, height, width, height));
    float previous_scale = game->intro_assets.font_scale;
    float setup_scale = game->trainer_select_assets.ui.font_scale;
    intro_assets_update_fonts(&game->intro_assets, canvas.target.texture.width / 1280.0f);
    selection_ui_update_fonts(&game->trainer_select_assets.ui, canvas.target.texture.width / 1280.0f);
    Game before = *game;
    BeginDrawing();
    game_canvas_begin(&canvas);
    game_draw(game);
    game_canvas_present(&canvas);
    EndDrawing();
    assert(memcmp(&before, game, sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas.target.texture);
    ImageFlipVertical(&image);
    check_progress_image(game, image);
    assert(ExportImage(image, path));
    UnloadImage(image);
    game_canvas_unload(&canvas);
    intro_assets_update_fonts(&game->intro_assets, previous_scale);
    selection_ui_update_fonts(&game->trainer_select_assets.ui, setup_scale);
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
    for (int i = 0; i < INTRO_UI_COUNT; i++) assert(game.intro_assets.ui[i].id);
    for (int i = 0; i < INTRO_FONT_COUNT; i++) assert(game.intro_assets.fonts[i].texture.id);
    capture(&game, "build/intro_preview/home_720p.png");
    capture_scaled(&game, 1920, 1080, "build/intro_preview/home_1080p.png");
    game.screen = GAME_TRAINER_SELECT;
    for (int i = 0; i < SELECTION_SKIN_COUNT; i++) assert(game.trainer_select_assets.ui.skins[i].id);
    for (int i = 0; i < SELECTION_FONT_COUNT; i++) assert(game.trainer_select_assets.ui.fonts[i].texture.id);
    capture_scaled(&game, 1920, 1080, "build/intro_preview/trainer_select_1080p.png");
    capture(&game, "build/intro_preview/trainer_select_720p.png");
    game.trainer_select.selected = TRAINER_FEMALE;
    capture_scaled(&game, 1920, 1080, "build/intro_preview/trainer_female_1080p.png");
    game.trainer_select.selected = TRAINER_MALE;
    SelectionUiAssets saved_setup = game.trainer_select_assets.ui;
    memset(game.trainer_select_assets.ui.skins, 0, sizeof(game.trainer_select_assets.ui.skins));
    memset(game.trainer_select_assets.ui.fonts, 0, sizeof(game.trainer_select_assets.ui.fonts));
    capture(&game, "build/intro_preview/trainer_setup_fallback.png");
    game.trainer_select_assets.ui = saved_setup;
    game.screen = GAME_NAME_ENTRY;
    game.name_entry.progress = 1;
    const char *name = "Teste";
    while (*name) assert(name_entry_append(&game.name_entry, *name++));
    capture_scaled(&game, 1920, 1080, "build/intro_preview/name_1080p.png");
    capture(&game, "build/intro_preview/name_720p.png");
    saved_setup = game.trainer_select_assets.ui;
    memset(game.trainer_select_assets.ui.skins, 0, sizeof(game.trainer_select_assets.ui.skins));
    capture(&game, "build/intro_preview/name_setup_fallback.png");
    game.trainer_select_assets.ui = saved_setup;
    key(&game, KEY_ENTER);
    assert(game.screen == GAME_INTRO && game.intro.stage == INTRO_LETTER);
    assert(!game.campaign.has_team && game.campaign.team.count == 0);
    /* ENTER mantido na transição não pode aceitar a carta. */
    event(2, KEY_ENTER, 0); tick(&game);
    assert(game.intro.stage == INTRO_LETTER);
    event(1, KEY_ENTER, 0); tick(&game); tick(&game);
    game.intro.reveal = 10000;
    capture(&game, "build/intro_preview/01_letter.png");
    capture_scaled(&game, 1920, 1080, "build/intro_preview/letter_1080p.png");
    click(&game, 1020, 660);
    assert(game.intro.stage == INTRO_ACCEPT);
    key(&game, KEY_ENTER); /* Completa o texto sem avançar. */
    assert(game.intro.stage == INTRO_ACCEPT);
    capture(&game, "build/intro_preview/02_accept.png");
    capture_scaled(&game, 1920, 1080, "build/intro_preview/accept_1080p.png");
    click(&game, 1020, 660);
    assert(game.intro.stage == INTRO_STARTERS);
    key(&game, KEY_ENTER);
    assert(game.intro.stage == INTRO_STARTERS && game.intro.count == 0);
    capture(&game, "build/intro_preview/selection_0.png");
    click(&game, 1020, 660); /* Confirmar desabilitado não avança por mouse. */
    assert(game.intro.stage == INTRO_STARTERS && game.intro.count == 0);
    /* Todas as seis candidatas podem ser selecionadas e removidas. */
    for (int i = 0; i < INTRO_STARTER_COUNT; i++) {
        game.intro.cursor = i;
        key(&game, KEY_SPACE); assert(game.intro.count == 1);
        IntroScreen draft = game.intro;
        draft.count = INTRO_TEAM_SIZE;
        for (int slot = 0; slot < INTRO_TEAM_SIZE; slot++)
            draft.selected[slot] = (i + slot) % INTRO_STARTER_COUNT;
        Team selected = {0};
        assert(intro_screen_team(&draft, &selected));
        assert(selected.plants[0].id == (int)PlantData_GetStarter(i)->id);
        assert(selected.plants[0].id != SPECIES_ALOE && selected.plants[0].id != SPECIES_SUNFLOWER);
        key(&game, KEY_SPACE); assert(game.intro.count == 0);
    }
    click(&game, 150, 210);
    capture(&game, "build/intro_preview/selection_1.png");
    click(&game, 500, 210);
    capture(&game, "build/intro_preview/selection_2.png");
    key(&game, KEY_ENTER); assert(game.intro.stage == INTRO_STARTERS);
    click(&game, 850, 210);
    assert(game.intro.count == INTRO_TEAM_SIZE);
    click(&game, 150, 410); /* Uma quarta planta não pode entrar. */
    assert(game.intro.count == INTRO_TEAM_SIZE);
    Team team = {0};
    assert(intro_screen_team(&game.intro, &team) && team.count == INTRO_TEAM_SIZE);
    assert(team.plants[0].id == SPECIES_CLOVER && team.plants[1].id == SPECIES_POISON_IVY &&
           team.plants[2].id == SPECIES_PEPPER);
    IntroScreen invalid = game.intro;
    invalid.selected[1] = invalid.selected[0];
    assert(!intro_screen_team(&invalid, &team));
    invalid = game.intro;
    invalid.selected[0] = -1;
    assert(!intro_screen_team(&invalid, &team));
    invalid.selected[0] = INTRO_STARTER_COUNT;
    assert(!intro_screen_team(&invalid, &team));
    capture(&game, "build/intro_preview/03_starters.png");
    /* Foco e seleção são independentes; mouse apenas passando não seleciona. */
    event(7, 900, 450); tick(&game);
    assert(game.intro.cursor == 5 && game.intro.count == 3);
    key(&game, KEY_RIGHT); assert(game.intro.cursor == 0);
    capture(&game, "build/intro_preview/selection_focused_selected.png");
    click(&game, 150, 210);
    assert(game.intro.count == 2 && game.intro.selected[0] == 1 && game.intro.selected[1] == 2);
    click(&game, 150, 210);
    assert(game.intro.count == 3 && game.intro.selected[2] == 0);
    /* Recupera a ordem inicial, preservada até os dois combates. */
    click(&game, 150, 210); click(&game, 500, 210); click(&game, 850, 210);
    click(&game, 150, 210); click(&game, 500, 210); click(&game, 850, 210);
    capture_scaled(&game, 1920, 1080, "build/intro_preview/selection_1080p.png");
    capture_scaled(&game, 1024, 768, "build/intro_preview/selection_4x3.png");
    IntroAssets saved_assets = game.intro_assets;
    for (int i = 0; i < INTRO_UI_COUNT; i++) game.intro_assets.ui[i] = (Texture2D){0};
    capture(&game, "build/intro_preview/selection_missing_ui.png");
    game.intro_assets = saved_assets;
    Font saved_font = game.ui.dialogue_font;
    Font saved_fonts[INTRO_FONT_COUNT];
    memcpy(saved_fonts, game.intro_assets.fonts, sizeof(saved_fonts));
    memset(game.intro_assets.fonts, 0, sizeof(game.intro_assets.fonts));
    game.ui.dialogue_font = (Font){0};
    capture(&game, "build/intro_preview/selection_missing_font.png");
    game.ui.dialogue_font = saved_font;
    memcpy(game.intro_assets.fonts, saved_fonts, sizeof(saved_fonts));
    click(&game, 180, 660);
    assert(game.intro.stage == INTRO_ACCEPT && game.intro.count == 3);
    game.intro.reveal = 10000;
    key(&game, KEY_ENTER);
    assert(game.intro.stage == INTRO_STARTERS && game.intro.count == 3);
    click(&game, 1020, 660); assert(game.intro.stage == INTRO_TRAVEL);
    key(&game, KEY_ESCAPE);
    assert(game.intro.stage == INTRO_STARTERS && game.intro.count == INTRO_TEAM_SIZE);
    key(&game, KEY_ENTER);
    key(&game, KEY_ENTER);
    capture(&game, "build/intro_preview/04_travel.png");
    capture_scaled(&game, 1920, 1080, "build/intro_preview/travel_1080p.png");
    click(&game, 1020, 660);
    assert(game.screen == GAME_EXPLORATION && game.campaign.has_team);
    assert(game.campaign.team.count == INTRO_TEAM_SIZE && !game.campaign.has_formula);
    capture(&game, "build/intro_preview/05_arrival.png");
    /* Abre o guardião do depósito pelo mesmo caminho usado pela exploração. */
    game.world.location = WORLD_DEPOT;
    game.player.feet = (Vector2){640, 420};
    key(&game, KEY_E);
    assert(game.screen == GAME_BATTLE && !game.battle_screen.choosing_team);
    assert(game.battle_screen.battle.player_team.count == INTRO_TEAM_SIZE);
    assert(game.battle_screen.battle.enemy_team.count == 1);
    assert(game.battle_screen.battle.enemy_team.plants[0].id == SPECIES_ALOE);
    assert(game.battle_screen.battle.enemy_team.plants[0].max_hp == 235);
    assert(game.battle_screen.scene == BATTLE_SCENE_DEPOT);
    assert(game.battle_screen.setup_side == -1);
    for (int i = 0; i < INTRO_TEAM_SIZE; i++) {
        assert(game.battle_screen.battle.boosted_move_indices[0][i] == -1);
        assert(Battle_CanUseMove(&game.battle_screen.battle,0,0,0));
    }
    capture(&game, "build/intro_preview/06_starter_battle.png");
    key(&game,KEY_ENTER);
    capture(&game,"build/intro_preview/normal_moves.png");
    key(&game,KEY_ESCAPE);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_EXPLORATION && !game.campaign.has_formula);
    assert(game.campaign.team.count == INTRO_TEAM_SIZE);
    /* Abas livres, perguntas individuais, rascunhos e confirmação explícita. */
    game.world.location = WORLD_EXTERIOR;
    game.player.feet = (Vector2){505,465};
    key(&game,KEY_E);
    assert(game.world.logic_quiz_open && !game.world.logic_move_mask);
    capture(&game,"build/intro_preview/logic_quiz.png");
    Vector2 quiz_feet = game.player.feet;
    key(&game,KEY_D);
    assert(game.player.feet.x == quiz_feet.x && game.player.feet.y == quiz_feet.y);
    click(&game,985,175); /* Pode começar pela terceira planta. */
    assert(game.logic_setup.selected == 2);
    key(&game,KEY_TWO);
    assert(game.logic_setup.rejected_mask == 4u && !game.world.logic_move_mask);
    click(&game,300,175);
    key(&game,KEY_ONE);
    assert(game.world.logic_move_mask == 1u && !game.campaign.has_formula);
    assert(game.world.logic_quiz_open && !game.world.dialogue_title);
    capture(&game,"build/intro_preview/logic_configuration.png");
    click(&game,700,298); key(&game,KEY_DOWN); key(&game,KEY_ENTER);
    assert(game.logic_setup.moves[0] == 1 && game.campaign.boosted_move_indices[0] == -1);
    click(&game,560,370); key(&game,KEY_UP); key(&game,KEY_ENTER);
    assert(game.logic_setup.formulas[0].left == LOGIC_OWN_RESERVE);
    click(&game,920,370); key(&game,KEY_UP); key(&game,KEY_ENTER);
    assert(game.logic_setup.formulas[0].right == LOGIC_OWN_LOW_HP);
    click(&game,1040,460);
    assert(game.logic_setup.formulas[0].operator == LOGIC_XOR);
    click(&game,560,370);
    click(&game,560,428); /* A condição Q repetida está indisponível. */
    assert(game.logic_setup.dropdown == 1 && game.logic_setup.formulas[0].left == LOGIC_OWN_RESERVE);
    key(&game,KEY_ESCAPE);
    assert(game.logic_setup.dropdown == -1 && game.world.logic_quiz_open);
    click(&game,985,175); key(&game,KEY_ONE);
    assert(game.world.logic_move_mask == 5u && !game.campaign.has_formula);
    click(&game,700,298); key(&game,KEY_DOWN); key(&game,KEY_ENTER);
    assert(game.logic_setup.dropdown == 0 && game.logic_setup.moves[2] == 0); /* Crescer é atributo. */
    capture(&game,"build/intro_preview/logic_attack_dropdown.png");
    key(&game,KEY_DOWN); key(&game,KEY_ENTER);
    assert(game.logic_setup.moves[2] == 2);
    click(&game,300,175);
    assert(game.logic_setup.moves[0] == 1 && game.logic_setup.formulas[0].operator == LOGIC_XOR);
    click(&game,560,370); key(&game,KEY_UP);
    capture(&game,"build/intro_preview/logic_dropdown.png");
    capture_scaled(&game,1920,1080,"build/intro_preview/logic_dropdown_1080p.png");
    key(&game,KEY_ESCAPE);
    click(&game,1000,595);
    assert(game.campaign.has_formula && game.campaign.boosted_move_indices[0] == 1);
    assert(game.campaign.formulas[0].operator == LOGIC_XOR);
    key(&game,KEY_ESCAPE);
    assert(!game.world.logic_quiz_open && game.screen == GAME_EXPLORATION);
    game.world.location = WORLD_DEPOT;
    game.player.feet = (Vector2){640,420};
    key(&game,KEY_E);
    assert(game.screen == GAME_BATTLE && game.battle_screen.setup_side == -1);
    assert(game.battle_screen.battle.boosted_move_indices[0][0] == 1);
    for (int i = 1; i < INTRO_TEAM_SIZE; i++) assert(game.battle_screen.battle.boosted_move_indices[0][i] == -1);
    key(&game,KEY_ENTER);
    capture(&game,"build/intro_preview/logical_move_partial.png");
    key(&game,KEY_ESCAPE); key(&game,KEY_ESCAPE);
    game.world.location = WORLD_EXTERIOR;
    game.player.feet = (Vector2){505,465};
    key(&game,KEY_E);
    assert(game.world.logic_quiz_open && game.logic_setup.selected == 0);
    click(&game,640,175); key(&game,KEY_TWO);
    assert(game.world.logic_move_mask == 7u && game.campaign.boosted_move_indices[1] == -1);
    key(&game,KEY_TAB); key(&game,KEY_TAB); key(&game,KEY_TAB); key(&game,KEY_TAB); key(&game,KEY_ENTER);
    assert(game.campaign.boosted_move_indices[1] == 0);
    click(&game,985,175);
    assert(game.world.logic_quiz_open); /* A terceira pergunta já foi resolvida. */
    click(&game,1000,595);
    assert(game.campaign.boosted_move_indices[2] == 2);
    click(&game,300,175); click(&game,520,460); /* Editar sem confirmar não altera o salvo. */
    assert(game.logic_setup.formulas[0].operator == LOGIC_AND);
    assert(game.campaign.formulas[0].operator == LOGIC_XOR);
    key(&game,KEY_ESCAPE);
    game.world.location = WORLD_NURSERY;
    game.player.feet = (Vector2){640, 420};
    key(&game, KEY_E);
    assert(game.screen == GAME_BATTLE && game.battle_screen.setup_side == -1);
    assert(game.battle_screen.battle.enemy_team.count == 1);
    assert(game.battle_screen.battle.enemy_team.plants[0].id == SPECIES_SUNFLOWER);
    assert(game.battle_screen.battle.enemy_team.plants[0].max_hp == 240);
    assert(game.battle_screen.scene == BATTLE_SCENE_NURSERY);
    for (int i = 0; i < INTRO_TEAM_SIZE; i++) assert(game.battle_screen.battle.boosted_move_indices[0][i] == (i == 0 ? 1 : i == 1 ? 0 : 2));
    assert(game.battle_screen.battle.formulas[0][0].operator == LOGIC_XOR);
    for (int i = 0; i < INTRO_TEAM_SIZE; i++)
        assert(game.battle_screen.battle.player_team.plants[i].id == team.plants[i].id);
    key(&game, KEY_ESCAPE);
    key(&game, KEY_ESCAPE);
    key(&game, KEY_ENTER); key(&game, KEY_ENTER);
    for (int i = 0; i < 90; i++) tick(&game);
    key(&game, KEY_ENTER);
    assert(game.screen == GAME_INTRO && game.intro.count == 0 && !game.campaign.has_team && !game.world.logic_move_mask);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_NAME_ENTRY && strcmp(game.name_entry.draft, "Teste") == 0);
    game_shutdown(&game);
    for (int i = 0; i < INTRO_UI_COUNT; i++) assert(!game.intro_assets.ui[i].id);
    for (int i = 0; i < INTRO_FONT_COUNT; i++) assert(!game.intro_assets.fonts[i].texture.id);
    for (int i = 0; i < SELECTION_SKIN_COUNT; i++) assert(!game.trainer_select_assets.ui.skins[i].id);
    for (int i = 0; i < SELECTION_FONT_COUNT; i++) assert(!game.trainer_select_assets.ui.fonts[i].texture.id);
    CloseWindow();
    puts("Introdução: escolhas, foco/mouse/teclado, confirmação, recursos/fallback, resoluções, sessão e dois bosses verificados.");
    return 0;
}
