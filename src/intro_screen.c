/* Conduz a abertura narrativa e a escolha de três companheiras, sem regras de combate. */
#include "intro_screen.h"
#include "plant_data.h"
#include "selection_ui.h"
#include <math.h>

static const Color ink = {35, 64, 46, 255};
static const Color cream = {253, 245, 216, 255};
static const Rectangle next_button = {830, 608, 330, 56};
static const Rectangle back_button = {120, 608, 180, 56};
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
    return (Rectangle){120 + (index % 3) * 352, 172 + (index / 3) * 190, 336, 174};
}

static int selected_slot(const IntroScreen *screen, int species)
{
    for (int i = 0; i < screen->count; i++)
        if (screen->selected[i] == species) return i;
    return -1;
}

static bool activate(Rectangle bounds)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
           CheckCollisionPointRec(GetMousePosition(), bounds);
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
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    screen->elapsed += dt;
    screen->reveal += dt * 48;
    if (screen->input_blocked) {
        if (!selection_ui_input_down()) screen->input_blocked = false;
        return INTRO_NONE;
    }
    if (!accept_input) return INTRO_NONE;
    if (IsKeyPressed(KEY_ESCAPE) || activate(back_button)) {
        if (screen->stage == INTRO_LETTER) return INTRO_BACK;
        set_stage(screen, (IntroStage)(screen->stage - 1));
        return INTRO_NONE;
    }
    if (screen->stage == INTRO_STARTERS) {
        int cursor = screen->cursor;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) cursor--;
        else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) cursor++;
        else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) cursor -= 3;
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) cursor += 3;
        screen->cursor = (cursor + INTRO_STARTER_COUNT) % INTRO_STARTER_COUNT;
        int chosen = IsKeyPressed(KEY_SPACE) ? screen->cursor : -1;
        for (int i = 0; i < INTRO_STARTER_COUNT; i++)
            if (activate(starter_card(i))) { chosen = i; screen->cursor = i; break; }
        if (chosen >= 0) {
            int slot = selected_slot(screen, chosen);
            if (slot >= 0) {
                for (int i = slot; i + 1 < screen->count; i++)
                    screen->selected[i] = screen->selected[i + 1];
                screen->count--;
            } else if (screen->count < INTRO_TEAM_SIZE) screen->selected[screen->count++] = chosen;
        }
    }
    if (IsKeyPressed(KEY_ENTER) || activate(next_button)) {
        if (screen->stage == INTRO_STARTERS) {
            if (screen->count == INTRO_TEAM_SIZE) set_stage(screen, INTRO_TRAVEL);
        } else if (screen->reveal < GetCodepointCount(passages[screen->stage])) {
            screen->reveal = (float)GetCodepointCount(passages[screen->stage]);
        } else if (screen->stage == INTRO_TRAVEL) return INTRO_FINISH;
        else set_stage(screen, (IntroStage)(screen->stage + 1));
    }
    return INTRO_NONE;
}

static void button(const UiTheme *ui, Rectangle bounds, const char *text, bool enabled)
{
    bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), bounds);
    selection_ui_panel(bounds, 6, enabled ? (hover ? SELECTION_ORANGE : cream) : (Color){185, 186, 160, 255});
    selection_ui_text(ui, text, (int)(bounds.x + bounds.width / 2), (int)bounds.y + 16, 23, ink);
}

/* Pequenas silhuetas provisórias desenhadas em código; nenhum recurso novo é exigido. */
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
    } else if (species == SPECIES_VENUS_FLYTRAP) {
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

void intro_screen_draw(const UiTheme *ui, const IntroScreen *screen,
                       const TrainerSelectAssets *assets, const char *player_name)
{
    static const char *const titles[] = {"Uma carta para você", "Um novo começo", "Quem vai com você?", "A caminho da vila"};
    static const char *const labels[] = {"Ler a resposta · ENTER", "Aceitar e preparar equipe", "Confirmar equipe · ENTER", "Chegar à vila · ENTER"};
    trainer_background_draw(&assets->background, screen->elapsed);
    DrawRectangle(0, 0, 1280, 720, Fade((Color){20, 42, 33, 255}, 0.55f));
    selection_ui_text(ui, "PLANTS x PLANTS  /  PRÓLOGO", 640, 28, 20, cream);
    selection_ui_text(ui, titles[screen->stage], 640, 64, 38, cream);
    for (int i = 0; i < 4; i++)
        DrawRectangle(562 + i * 42, 124, 30, 5, i <= (int)screen->stage ? SELECTION_ORANGE : cream);
    if (screen->stage == INTRO_STARTERS) {
        selection_ui_text(ui, TextFormat("Escolha 3 de 6 · %d/3 selecionadas", screen->count), 640, 141, 21, cream);
        for (int i = 0; i < INTRO_STARTER_COUNT; i++) {
            Rectangle card = starter_card(i);
            const SpeciesData *plant = PlantData_GetStarter(i);
            int slot = selected_slot(screen, i);
            selection_ui_panel(card, 8, slot >= 0 ? (Color){223, 235, 183, 255} : cream);
            if (screen->cursor == i) selection_ui_accents(card, 1);
            draw_plant(plant->id, (int)card.x + 58, (int)card.y + 108, screen->elapsed);
            int name_size = 26;
            while (name_size > 18 && ui_measure_bold(ui, plant->name_pt, name_size) > 216) name_size--;
            ui_text_bold(ui, plant->name_pt, (int)card.x + 108, (int)card.y + 22, name_size, ink);
            ui_text(ui, TextFormat("Vida %d · ATQ %d", plant->base_hp, plant->base_attack), (int)card.x + 108, (int)card.y + 62, 19, ink);
            ui_text(ui, TextFormat("Defesa %d", plant->base_defense), (int)card.x + 108, (int)card.y + 89, 19, ink);
            ui_text(ui, slot >= 0 ? TextFormat("%d · Na equipe", slot + 1) : "Selecionar", (int)card.x + 108, (int)card.y + 128, 21, ink);
        }
        selection_ui_text(ui, "Setas / WASD: navegar · ESPAÇO ou clique: selecionar / remover", 640, 568, 21, cream);
    } else {
        Rectangle panel = {120, 172, 1040, 402};
        ui_botanical_panel(ui, panel);
        ui_text_bold(ui, screen->stage == INTRO_LETTER ? TextFormat("Para %s", player_name) : player_name, 164, 201, 28, ink);
        ui_text_wrap_reveal(ui_pixel_font(ui), passages[screen->stage], (Rectangle){164, 251, 948, 290}, 27, ink, (int)screen->reveal);
    }
    button(ui, back_button, "Voltar · ESC", true);
    bool revealing = screen->stage != INTRO_STARTERS &&
                     screen->reveal < GetCodepointCount(passages[screen->stage]);
    button(ui, next_button, revealing ? "Completar texto · ENTER" : labels[screen->stage],
           screen->stage != INTRO_STARTERS || screen->count == INTRO_TEAM_SIZE);
}
