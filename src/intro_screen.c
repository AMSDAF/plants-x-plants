/* Conduz a abertura narrativa e a escolha de três companheiras, sem regras de combate. */
#include "intro_screen.h"
#include "plant_data.h"
#include "selection_ui.h"
#include <math.h>

static const Color ink = {35, 64, 46, 255};
static const Color cream = {253, 245, 216, 255};
static const Rectangle prologue_next = {832, 628, 350, 64};
static const Rectangle prologue_back = {98, 628, 240, 64};
static const Rectangle text_panel = {120, 172, 1040, 402};
static const int font_sizes[INTRO_FONT_COUNT] = {20, 22, 26, 28, 30, 72};
/* Recortes registrados no manifesto; os cartões compartilham o mesmo registro. */
static const Rectangle ui_sources[INTRO_UI_COUNT] = {
    [INTRO_CARD_NORMAL] = {118, 81, 1378, 814},
    [INTRO_CARD_SELECTED] = {118, 81, 1378, 814},
    [INTRO_FOCUS_OUTLINE] = {115, 78, 1382, 816},
    [INTRO_FOOTER_BUTTON] = {152, 158, 1868, 387},
    [INTRO_LEAF_BADGE] = {376, 293, 696, 491},
    [INTRO_CURSOR] = {410, 552, 293, 401}
};
static const char *const passages[] = {
    "As plantas da nossa vila estão perdendo a força. A água parou de chegar ao viveiro, "
    "o refletor se apagou e ninguém consegue fazer a central funcionar.\n\n"
    "Bento, Íris e eu estamos tentando cuidar de tudo, mas precisamos de ajuda. "
    "Traga suas plantas e venha nos encontrar na estufa.\n\nCom esperança,\nLia",
    "Uma vila inteira depende daquele jardim. Você dobra a carta com cuidado e decide partir.\n\n"
    "Mas uma jornada assim pede companhia. Antes de sair, escolha três plantas para formar sua equipe. "
    "Elas vão lutar ao seu lado nos desafios que encontrar pelo caminho.",
    "",
    "Com a carta na mochila e três novas companheiras, você segue pela estrada até a vila.\n\n"
    "Seu primeiro destino é a estufa: encontre Lia no pátio, ajude os moradores a recuperar "
    "a água e a luz e descubra como reativar a central."
};

static Rectangle starter_card(int index)
{
    return (Rectangle){98 + (index % 3) * 367, 162 + (index / 3) * 226, 350, 208};
}

/* Entrada e desenho consultam os mesmos limites das bordas externas. */
static Rectangle prologue_button_bounds(IntroStage stage, bool forward)
{
    Rectangle bounds = forward ? prologue_next : prologue_back;
    if (stage != INTRO_STARTERS)
        bounds.x = forward ? text_panel.x + text_panel.width - bounds.width : text_panel.x;
    return bounds;
}

static int selected_slot(const IntroScreen *screen, int species)
{
    for (int i = 0; i < screen->count; i++)
        if (screen->selected[i] == species) return i;
    return -1;
}

static bool activate(const UiPointer *pointer, Rectangle bounds)
{
    return ui_pointer_clicked(pointer,bounds);
}

static void set_stage(IntroScreen *screen, IntroStage stage)
{
    screen->stage = stage;
    screen->reveal = 0;
    screen->input_blocked = true;
}

void intro_screen_init(IntroScreen *screen)
{
    *screen = (IntroScreen){0};
}

/* Rasteriza no tamanho físico de exibição, sem ampliar o atlas dos diálogos. */
void intro_assets_update_fonts(IntroAssets *assets, float scale)
{
    if (scale <= 0) return;
    if (assets->font_scale > 0) {
        bool unchanged = true;
        for (int i = 0; i < INTRO_FONT_COUNT; i++)
            if (roundf(font_sizes[i] * scale) != roundf(font_sizes[i] * assets->font_scale)) unchanged = false;
        if (unchanged) {
            assets->font_scale = scale;
            return;
        }
    }
    int codepoints[352];
    int count = 0;
    for (int code = 32; code <= 383; code++) codepoints[count++] = code;
    for (int i = 0; i < INTRO_FONT_COUNT; i++) {
        if (assets->fonts[i].texture.id) UnloadFont(assets->fonts[i]);
        assets->fonts[i] = (Font){0};
        if (!FileExists("assets/fonts/pixelify_sans_regular.ttf")) continue;
        int pixels = (int)fmaxf(1, roundf(font_sizes[i] * scale));
        Font font = LoadFontEx("assets/fonts/pixelify_sans_regular.ttf", pixels, codepoints, count);
        if (font.texture.id && font.texture.id != GetFontDefault().texture.id) {
            SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
            assets->fonts[i] = font;
        }
    }
    assets->font_scale = scale;
}

void intro_assets_load(IntroAssets *assets)
{
    static const char *const paths[SPECIES_COUNT] = {
        [SPECIES_CLOVER] = "assets/sprites/plants/clover_v2.png",
        [SPECIES_POISON_IVY] = "assets/sprites/plants/poison_ivy_v2.png",
        [SPECIES_PEPPER] = "assets/sprites/plants/pepper_v2.png",
        [SPECIES_CACTUS] = "assets/sprites/plants/cactus_v2.png",
        [SPECIES_ORCHID] = "assets/sprites/plants/orchid_v2.png",
        [SPECIES_CARNIVOROUS_PLANT] = "assets/sprites/plants/venus_flytrap_v2.png"
    };
    *assets = (IntroAssets){0};
    for (int i = 0; i < INTRO_STARTER_COUNT; i++) {
        SpeciesId id = PlantData_GetStarter(i)->id;
        Image image = LoadImage(paths[id]);
        if (!image.data) continue;
        Rectangle source = GetImageAlphaBorder(image, 0.1f);
        if (source.width > 0 && source.height > 0) {
            assets->plants[id] = LoadTextureFromImage(image);
            assets->sources[id] = source;
            if (assets->plants[id].id) SetTextureFilter(assets->plants[id], TEXTURE_FILTER_POINT);
        }
        UnloadImage(image);
    }
    static const char *const ui_paths[INTRO_UI_COUNT] = {
        "assets/ui/plant_selection/card_normal_v1.png",
        "assets/ui/plant_selection/card_selected_v1.png",
        "assets/ui/plant_selection/focus_outline_v1.png",
        "assets/ui/plant_selection/footer_button_v1.png",
        "assets/ui/plant_selection/leaf_badge_v1.png",
        "assets/ui/plant_selection/cursor_v1.png"
    };
    for (int i = 0; i < INTRO_UI_COUNT; i++) {
        if (!FileExists(ui_paths[i])) continue;
        Image image = LoadImage(ui_paths[i]);
        if (!image.data) continue;
        Rectangle source = ui_sources[i];
        if (source.x + source.width <= image.width && source.y + source.height <= image.height) {
            assets->ui[i] = LoadTextureFromImage(image);
            if (assets->ui[i].id) SetTextureFilter(assets->ui[i], TEXTURE_FILTER_POINT);
        } else {
            TraceLog(LOG_WARNING, "Moldura da selecao invalida: %s", ui_paths[i]);
        }
        UnloadImage(image);
    }
    float scale = fminf(GetRenderWidth() / 1280.0f, GetRenderHeight() / 720.0f);
    intro_assets_update_fonts(assets, scale);
}

void intro_assets_unload(IntroAssets *assets)
{
    for (int i = 0; i < SPECIES_COUNT; i++)
        if (assets->plants[i].id) UnloadTexture(assets->plants[i]);
    for (int i = 0; i < INTRO_UI_COUNT; i++)
        if (assets->ui[i].id) UnloadTexture(assets->ui[i]);
    for (int i = 0; i < INTRO_FONT_COUNT; i++)
        if (assets->fonts[i].texture.id) UnloadFont(assets->fonts[i]);
    *assets = (IntroAssets){0};
}

bool intro_screen_team(const IntroScreen *screen, Team *team)
{
    if (screen->count != INTRO_TEAM_SIZE) return false;
    Team result = {0};
    for (int i = 0; i < screen->count; i++) {
        const SpeciesData *species = PlantData_GetStarter(screen->selected[i]);
        if (!species || !Team_SelectSpecies(&result, result.count, species->id)) return false;
    }
    *team = result;
    return true;
}

IntroAction intro_screen_update(IntroScreen *screen, bool accept_input, float dt)
{
    ui_pointer_update(&screen->pointer,accept_input && !screen->input_blocked);
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    screen->elapsed += dt;
    screen->reveal += dt * 48;
    if (screen->input_blocked) {
        if (!selection_ui_input_down()) screen->input_blocked = false;
        return INTRO_NONE;
    }
    if (!accept_input) return INTRO_NONE;
    if (IsKeyPressed(KEY_ESCAPE) || activate(&screen->pointer, prologue_button_bounds(screen->stage, false))) {
        if (screen->stage == INTRO_LETTER) return INTRO_BACK;
        set_stage(screen, (IntroStage)(screen->stage - 1));
        return INTRO_NONE;
    }
    if (screen->stage == INTRO_STARTERS) {
        Vector2 mouse_delta = GetMouseDelta();
        if (mouse_delta.x != 0 || mouse_delta.y != 0) {
            for (int i = 0; i < INTRO_STARTER_COUNT; i++)
                if (CheckCollisionPointRec(GetMousePosition(), starter_card(i))) screen->cursor = i;
        }
        int cursor = screen->cursor;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) cursor--;
        else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) cursor++;
        else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) cursor -= 3;
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) cursor += 3;
        screen->cursor = (cursor + INTRO_STARTER_COUNT) % INTRO_STARTER_COUNT;
        int chosen = IsKeyPressed(KEY_SPACE) ? screen->cursor : -1;
        for (int i = 0; i < INTRO_STARTER_COUNT; i++)
            if (activate(&screen->pointer, starter_card(i))) { chosen = i; screen->cursor = i; break; }
        if (chosen >= 0) {
            int slot = selected_slot(screen, chosen);
            if (slot >= 0) {
                for (int i = slot; i + 1 < screen->count; i++)
                    screen->selected[i] = screen->selected[i + 1];
                screen->count--;
            } else if (screen->count < INTRO_TEAM_SIZE) screen->selected[screen->count++] = chosen;
        }
    }
    if (IsKeyPressed(KEY_ENTER) || activate(&screen->pointer, prologue_button_bounds(screen->stage, true))) {
        if (screen->stage == INTRO_STARTERS) {
            if (screen->count == INTRO_TEAM_SIZE) set_stage(screen, INTRO_TRAVEL);
        } else if (screen->reveal < GetCodepointCount(passages[screen->stage])) {
            screen->reveal = (float)GetCodepointCount(passages[screen->stage]);
        } else if (screen->stage == INTRO_TRAVEL) return INTRO_FINISH;
        else set_stage(screen, (IntroStage)(screen->stage + 1));
    }
    return INTRO_NONE;
}

/* Reserva geométrica quando a imagem da inicial não está disponível. */
static void draw_plant(int species, int x, int y, float elapsed)
{
    int sway = (int)(sinf(elapsed * 1.8f + species) * 2);
    DrawRectangle(x - 4 + sway, y - 47, 8, 48, (Color){71, 117, 57, 255});
    DrawRectangle(x - 28 + sway, y - 35, 25, 12, (Color){91, 146, 67, 255});
    DrawRectangle(x + sway, y - 49, 26, 13, (Color){91, 146, 67, 255});
    if (species == SPECIES_ORCHID) {
        Color petals = {194, 80, 167, 255};
        DrawRectangle(x - 15 + sway, y - 70, 30, 28, petals);
        DrawRectangle(x - 6 + sway, y - 62, 12, 12, (Color){111, 77, 49, 255});
    }
    if (species == SPECIES_CACTUS)
        DrawRectangle(x - 11 + sway, y - 67, 22, 49, (Color){79, 133, 78, 255});
    if (species == SPECIES_CLOVER) {
        for (int i = 0; i < 4; i++)
            DrawRectangle(x - 18 + (i % 2) * 20 + sway, y - 73 + (i / 2) * 20,
                          17, 17, (Color){70, 150, 61, 255});
    } else if (species == SPECIES_PEPPER) {
        DrawRectangle(x - 10 + sway, y - 64, 21, 27, (Color){205, 65, 45, 255});
        DrawRectangle(x - 5 + sway, y - 37, 11, 10, (Color){205, 65, 45, 255});
    } else if (species == SPECIES_CARNIVOROUS_PLANT) {
        DrawRectangle(x - 21 + sway, y - 70, 42, 35, (Color){87, 143, 62, 255});
        DrawRectangle(x - 15 + sway, y - 58, 30, 11, (Color){167, 62, 70, 255});
        for (int i = 0; i < 4; i++)
            DrawRectangle(x - 14 + i * 8 + sway, y - 60, 4, 6, cream);
    } else if (species == SPECIES_POISON_IVY) {
        DrawRectangle(x - 10 + sway, y - 76, 20, 25, (Color){89, 125, 51, 255});
        DrawRectangle(x - 19 + sway, y - 69, 38, 11, (Color){89, 125, 51, 255});
    }
    DrawRectangle(x - 23, y, 46, 12, (Color){174, 98, 58, 255});
    DrawRectangle(x - 18, y + 12, 36, 20, (Color){144, 78, 47, 255});
}

/* Atlases próprios evitam escalar letras de 28 px para todos os tamanhos. */
static Font selection_font(const UiTheme *ui, const IntroAssets *assets, int size, float *draw_size)
{
    for (int i = 0; i < INTRO_FONT_COUNT; i++) {
        if (font_sizes[i] == size && assets->fonts[i].texture.id) {
            *draw_size = assets->fonts[i].baseSize / assets->font_scale;
            return assets->fonts[i];
        }
    }
    *draw_size = (float)size;
    return ui_pixel_font(ui);
}

static void pixel_text(const UiTheme *ui, const IntroAssets *assets, const char *text,
                       float x, float y, int size, Color color)
{
    float draw_size = 0;
    Font font = selection_font(ui, assets, size, &draw_size);
    float scale = assets->font_scale > 0 ? assets->font_scale : 1;
    Vector2 position = {roundf(x * scale) / scale, roundf(y * scale) / scale};
    DrawTextEx(font, text, position, draw_size, 0, color);
}

static void pixel_center(const UiTheme *ui, const IntroAssets *assets, const char *text,
                         float center, float y, int size, float max_width, Color color)
{
    float draw_size = 0;
    Font font = selection_font(ui, assets, size, &draw_size);
    while (size > 12 && MeasureTextEx(font, text, draw_size, 0).x > max_width) {
        size--;
        font = selection_font(ui, assets, size, &draw_size);
    }
    float width = MeasureTextEx(font, text, draw_size, 0).x;
    pixel_text(ui, assets, text, center - width / 2, y, size, color);
}

/* Centraliza a tinta dos glifos, sem contar a margem de ascensão da fonte. */
static void pixel_box(const UiTheme *ui, const IntroAssets *assets, const char *text,
                      Rectangle box, int size, Color color)
{
    float draw_size = 0;
    Font font = selection_font(ui, assets, size, &draw_size);
    float top = (float)font.baseSize, bottom = 0;
    for (const char *cursor = text; *cursor;) {
        int bytes = 0;
        int code = GetCodepointNext(cursor, &bytes);
        int index = GetGlyphIndex(font, code);
        if (code != ' ') {
            top = fminf(top, (float)font.glyphs[index].offsetY);
            bottom = fmaxf(bottom, font.glyphs[index].offsetY + font.recs[index].height);
        }
        cursor += bytes;
    }
    float y = box.y + box.height / 2 - (top + bottom) * draw_size / font.baseSize / 2;
    pixel_center(ui, assets, text, box.x + box.width / 2, y, size, box.width, color);
}

static void draw_ui_asset(const IntroAssets *assets, IntroUiAsset id, Rectangle bounds, Color tint)
{
    DrawTexturePro(assets->ui[id], ui_sources[id], bounds, (Vector2){0}, 0, tint);
}

/* Somente o centro se alonga: cantos, folhas e espessura mantêm a escala da altura. */
static void selection_button(const UiTheme *ui, const IntroAssets *assets,
                             Rectangle bounds, const char *label, bool enabled)
{
    bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), bounds);
    Color tint = enabled ? (hover ? (Color){255, 244, 210, 255} : WHITE)
                         : (Color){170, 175, 158, 255};
    if (assets->ui[INTRO_FOOTER_BUTTON].id) {
        Rectangle source = ui_sources[INTRO_FOOTER_BUTTON];
        float scale = bounds.height / source.height;
        float sx[4] = {source.x, source.x + 260, source.x + source.width - 260,
                       source.x + source.width};
        float sy[4] = {source.y, source.y + 88, source.y + source.height - 88,
                       source.y + source.height};
        float dx[4] = {bounds.x, bounds.x + 260 * scale,
                       bounds.x + bounds.width - 260 * scale, bounds.x + bounds.width};
        float dy[4] = {bounds.y, bounds.y + 88 * scale,
                       bounds.y + bounds.height - 88 * scale, bounds.y + bounds.height};
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                Rectangle part = {sx[col], sy[row], sx[col+1]-sx[col], sy[row+1]-sy[row]};
                Rectangle target = {dx[col], dy[row], dx[col+1]-dx[col], dy[row+1]-dy[row]};
                DrawTexturePro(assets->ui[INTRO_FOOTER_BUTTON], part, target, (Vector2){0}, 0, tint);
            }
        }
    } else {
        ui_botanical_panel(ui, bounds);
        if (!enabled) DrawRectangleRec(bounds, Fade(DARKGRAY, 0.25f));
    }
    pixel_box(ui, assets, label, (Rectangle){bounds.x + 32, bounds.y + 12, bounds.width - 64, bounds.height - 24},
              28, enabled ? ink : (Color){65, 76, 58, 255});
}

static void selection_card(const UiTheme *ui, const IntroScreen *screen,
                           const IntroAssets *assets, int index)
{
    Rectangle bounds = starter_card(index);
    const SpeciesData *plant = PlantData_GetStarter(index);
    int slot = selected_slot(screen, index);
    IntroUiAsset skin = slot >= 0 ? INTRO_CARD_SELECTED : INTRO_CARD_NORMAL;
    Rectangle source = ui_sources[skin];
    float scale = fminf(bounds.width / source.width, bounds.height / source.height);
    Rectangle target = {bounds.x, bounds.y + (bounds.height - source.height * scale) / 2,
                        source.width * scale, source.height * scale};
    if (assets->ui[skin].id) {
        draw_ui_asset(assets, skin, target, WHITE);
    } else {
        ui_botanical_panel(ui, bounds);
        if (slot >= 0) DrawRectangleRec((Rectangle){bounds.x+20, bounds.y+55, 310, 135},
                                      Fade((Color){122, 153, 65, 255}, 0.18f));
    }

    Rectangle plant_source = assets->sources[plant->id];
    if (assets->plants[plant->id].id && plant_source.width > 0 && plant_source.height > 0) {
        float plant_scale = fminf(107.0f / plant_source.width, 113.0f / plant_source.height);
        Rectangle plant_target = {bounds.x + 88.5f - plant_source.width * plant_scale / 2,
                                  bounds.y + 179 - plant_source.height * plant_scale,
                                  plant_source.width * plant_scale, plant_source.height * plant_scale};
        DrawTexturePro(assets->plants[plant->id], plant_source, plant_target, (Vector2){0}, 0, WHITE);
    } else {
        draw_plant(plant->id, (int)bounds.x + 88, (int)bounds.y + 142, screen->elapsed);
    }

    if (slot >= 0) {
        if (assets->ui[INTRO_LEAF_BADGE].id)
            draw_ui_asset(assets, INTRO_LEAF_BADGE,
                          (Rectangle){bounds.x + 59, bounds.y + 27, 34, 25}, WHITE);
        else
            DrawEllipse((int)bounds.x + 76, (int)bounds.y + 39, 17, 12, DARKGREEN);
        pixel_box(ui, assets, TextFormat("%d", slot + 1),
                  (Rectangle){bounds.x + 59, bounds.y + 27, 34, 25}, 26, cream);
    }
    pixel_center(ui, assets, plant->name_pt, bounds.x + (slot >= 0 ? 202 : 176), bounds.y + 23,
                 30, slot >= 0 ? 211 : 274, ink);
    pixel_text(ui, assets, TextFormat("Vida  %d", plant->base_hp), bounds.x + 187, bounds.y + 61, 28, ink);
    pixel_text(ui, assets, TextFormat("ATQ  %d", plant->base_attack), bounds.x + 187, bounds.y + 89, 28, ink);
    pixel_text(ui, assets, TextFormat("DEF  %d", plant->base_defense), bounds.x + 187, bounds.y + 117, 28, ink);
    pixel_center(ui, assets, slot >= 0 ? TextFormat("%d · Na equipe", slot + 1) : "Selecionar",
                 bounds.x + 234, bounds.y + 154, 26, 137, ink);

    if (screen->cursor == index) {
        if (assets->ui[INTRO_FOCUS_OUTLINE].id) {
            Rectangle focus_source = ui_sources[INTRO_FOCUS_OUTLINE];
            /* Usa o registro comum, sem deslocar o foco ao mudar o cartão. */
            Rectangle focus_target = {target.x + (focus_source.x - source.x) * scale,
                                      target.y + (focus_source.y - source.y) * scale,
                                      focus_source.width * scale, focus_source.height * scale};
            draw_ui_asset(assets, INTRO_FOCUS_OUTLINE, focus_target, WHITE);
        } else {
            DrawRectangleLinesEx(bounds, 2, GOLD);
        }
        if (assets->ui[INTRO_CURSOR].id) {
            draw_ui_asset(assets, INTRO_CURSOR,
                          (Rectangle){bounds.x - 25, bounds.y + bounds.height / 2 - 13, 19, 26}, WHITE);
        } else {
            selection_ui_cursor((int)bounds.x - 6, (int)(bounds.y + bounds.height / 2),
                                screen->elapsed, 1);
        }
    }
}

/* Cabeçalho e rodapé mantêm as mesmas medidas nas quatro etapas. */
static void prologue_header(const UiTheme *ui, const IntroAssets *assets, const IntroScreen *screen)
{
    static const char *const titles[] = {
        "Uma carta para você", "Um novo começo", "Quem vai com você?", "A caminho da vila"
    };
    static const char *const subtitles[] = {
        "Um pedido de ajuda chegou até você.", "Sua jornada começa com uma decisão.",
        "", "Próximo destino: a estufa da vila."
    };
    pixel_center(ui, assets, titles[screen->stage], 640, 29, 72, 1100, cream);
    selection_ui_progress((OpeningStep)(OPENING_LETTER + (int)screen->stage), 1);
    const char *subtitle = screen->stage == INTRO_STARTERS
        ? TextFormat("Escolha 3 de 6 plantas   |   %d/3 selecionadas", screen->count)
        : subtitles[screen->stage];
    pixel_center(ui, assets, subtitle, 640, 116, 28, 1100, cream);
}

static void prologue_footer(const UiTheme *ui, const IntroAssets *assets, const IntroScreen *screen)
{
    static const char *const labels[] = {
        "Continuar · ENTER", "Preparar equipe · ENTER", "Confirmar equipe · ENTER", "Chegar à vila · ENTER"
    };
    bool selecting = screen->stage == INTRO_STARTERS;
    bool revealing = !selecting && screen->reveal < GetCodepointCount(passages[screen->stage]);
    Rectangle back = prologue_button_bounds(screen->stage, false);
    Rectangle next = prologue_button_bounds(screen->stage, true);
    selection_button(ui, assets, back, "Voltar · ESC", true);
    selection_button(ui, assets, next, revealing ? "Completar texto · ENTER" : labels[screen->stage],
                     !selecting || screen->count == INTRO_TEAM_SIZE);
    pixel_center(ui, assets, selecting ? "Setas / WASD: navegar · Espaço ou clique: selecionar"
                                      : "ENTER ou clique: completar o texto / avançar",
                 (back.x + back.width + next.x) / 2, 643, 20,
                 next.x - back.x - back.width - 24, cream);
}

void intro_screen_draw(const UiTheme *ui, const IntroScreen *screen,
                       const TrainerSelectAssets *assets, const IntroAssets *plant_assets,
                       const char *player_name)
{
    trainer_background_draw_dimmed(&assets->background, screen->elapsed);
    bool selecting = screen->stage == INTRO_STARTERS;
    prologue_header(ui, plant_assets, screen);
    if (selecting) {
        for (int i = 0; i < INTRO_STARTER_COUNT; i++) selection_card(ui, screen, plant_assets, i);
    } else {
        ui_botanical_panel(ui, text_panel);
        pixel_text(ui, plant_assets, screen->stage == INTRO_LETTER ? TextFormat("Para %s", player_name) : player_name,
                    164, 201, 28, ink);
        float draw_size;
        Font font = selection_font(ui, plant_assets, 28, &draw_size);
        ui_text_wrap_reveal_pixels(font, passages[screen->stage],
                                   (Rectangle){164,251,948,290}, draw_size, ink,
                                   (int)screen->reveal, plant_assets->font_scale);
    }
    prologue_footer(ui, plant_assets, screen);
}
