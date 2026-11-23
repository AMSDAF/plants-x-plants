/* Compõe a central industrial com recursos emprestados e desenho somente de leitura. */
#include "relay_ui.h"
#include <math.h>

static const Color ink = {24,52,30,255};
static const Color ivory = {247,239,227,255};
static const Color sage = {214,229,170,255};
static const Color amber = {250,204,89,255};
static const Rectangle sources[RELAY_ART_COUNT] = {
    {80,97,1376,828}, {186,210,1800,303}, {542,303,429,409}, {542,303,429,409}
};

void relay_ui_load(RelayUiAssets *assets)
{
    static const char *const paths[RELAY_ART_COUNT] = {
        "assets/ui/relay_panel/panel_frame_v1.png", "assets/ui/relay_panel/button_frame_v1.png",
        "assets/ui/relay_panel/switch_on_v1.png", "assets/ui/relay_panel/switch_off_v1.png"
    };
    *assets = (RelayUiAssets){0};
    for (int i = 0; i < RELAY_ART_COUNT; i++) {
        if (!FileExists(paths[i])) continue;
        Texture2D texture = LoadTexture(paths[i]);
        int width = i == RELAY_BUTTON_SKIN ? 2172 : 1536;
        int height = i == RELAY_BUTTON_SKIN ? 724 : 1024;
        if (texture.id && (texture.width != width || texture.height != height)) {
            UnloadTexture(texture);
            continue;
        }
        if (texture.id) SetTextureFilter(texture,TEXTURE_FILTER_POINT);
        assets->textures[i] = texture;
    }
}

void relay_ui_unload(RelayUiAssets *assets)
{
    for (int i = 0; i < RELAY_ART_COUNT; i++)
        if (assets->textures[i].id) UnloadTexture(assets->textures[i]);
    *assets = (RelayUiAssets){0};
}

/* A cor de estado afeta apenas o centro; cantos e parafusos mantêm o aço. */
static void skin(const RelayUiAssets *assets, RelayArt art, Rectangle box, Color tint, float corner)
{
    Texture2D texture = assets->textures[art];
    if (!texture.id) {
        selection_ui_panel(box,5,(Color){55,79,90,255});
        DrawRectangleLinesEx(box,2,(Color){170,194,204,255});
        selection_ui_panel((Rectangle){box.x+5,box.y+5,box.width-10,box.height-10},3,
            tint.r == 255 && tint.g == 255 ? ivory : tint);
        return;
    }
    Rectangle source = sources[art];
    float horizontal = art == RELAY_BUTTON_SKIN ? 120 : 72;
    float vertical = art == RELAY_BUTTON_SKIN ? 70 : 72;
    float scale = corner/vertical;
    float dx = fminf(horizontal*scale,box.width/2), dy = fminf(vertical*scale,box.height/2);
    float sx[] = {source.x,source.x+horizontal,source.x+source.width-horizontal,source.x+source.width};
    float sy[] = {source.y,source.y+vertical,source.y+source.height-vertical,source.y+source.height};
    float tx[] = {box.x,box.x+dx,box.x+box.width-dx,box.x+box.width};
    float ty[] = {box.y,box.y+dy,box.y+box.height-dy,box.y+box.height};
    for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++)
        DrawTexturePro(texture,(Rectangle){sx[col],sy[row],sx[col+1]-sx[col],sy[row+1]-sy[row]},
            (Rectangle){tx[col],ty[row],tx[col+1]-tx[col],ty[row+1]-ty[row]},
            (Vector2){0},0,row == 1 && col == 1 ? tint : WHITE);
}

static void label(const UiTheme *ui, const SelectionUiAssets *fonts, const char *value,
                    Rectangle box, int size, Color color)
{
    selection_ui_pixel_box(ui,fonts,value,box,size,color);
}

static void pointer(const SelectionUiAssets *fonts, Rectangle box)
{
    selection_ui_skin(fonts,SELECTION_POINTER,(Rectangle){box.x-24,box.y+box.height/2-13,20,26},1);
}

static void action(const UiTheme *ui, const SelectionUiAssets *fonts, const RelayUiAssets *assets,
                    Rectangle box, const char *value, bool primary)
{
    bool hover = CheckCollisionPointRec(GetMousePosition(),box);
    skin(assets,RELAY_BUTTON_SKIN,box,hover ? sage : WHITE,10);
    if (primary || hover) DrawRectangleLinesEx((Rectangle){box.x+4,box.y+4,box.width-8,box.height-8},2,
        primary ? (Color){217,154,33,255} : (Color){104,135,75,255});
    label(ui,fonts,value,(Rectangle){box.x+14,box.y+6,box.width-28,box.height-12},28,ink);
}

static void control(const RelayUiAssets *assets, Rectangle box, bool on)
{
    RelayArt art = on ? RELAY_SWITCH_ON : RELAY_SWITCH_OFF;
    Rectangle source = sources[art];
    Texture2D texture = assets->textures[art];
    if (texture.id) {
        float scale = fminf(box.width/source.width,box.height/source.height);
        Rectangle dest = {roundf(box.x+(box.width-source.width*scale)/2),
            roundf(box.y+(box.height-source.height*scale)/2),source.width*scale,source.height*scale};
        DrawTexturePro(texture,source,dest,(Vector2){0},0,WHITE);
    } else {
        Rectangle rocker = {box.x+24,box.y+4,32,box.height-8};
        DrawRectangleRec(rocker,ink);
        DrawRectangle((int)rocker.x+4,(int)rocker.y+4,24,30,on ? LIME : GRAY);
        DrawRectangle((int)rocker.x+4,(int)rocker.y+36,24,30,on ? GRAY : MAROON);
        DrawCircle((int)(box.x+88),(int)(box.y+box.height/2),8,on ? LIME : MAROON);
    }
}

void relay_ui_draw(const UiTheme *ui, const SelectionUiAssets *fonts,
                   const RelayUiAssets *assets, const World *world)
{
    if (!world->relay_panel_open || world->relay_step < 0 || world->relay_step >= WORLD_RELAY_RULE_COUNT) return;
    static const char *const formulas[] = {"A","A E B","(A OU B) E NÃO C"};
    static const char *const guides[] = {
        "Ligue o relé A. Ligado significa verdadeiro.",
        "Ligue A e B. As duas partes precisam ser verdadeiras.",
        "Ligue A ou B e mantenha C desligado."
    };
    static const char *const hints[] = {
        "Cada relé pode estar ligado ou desligado.",
        "E: os dois relés precisam estar ligados.",
        "OU: basta um ligado. NÃO C: C desligado."
    };
    int step = world->relay_step;
    DrawRectangle(0,0,1280,720,Fade(ink,.80f));
    skin(assets,RELAY_PANEL_SKIN,(Rectangle){56,38,1168,654},WHITE,24);
    label(ui,fonts,"Central de relés",(Rectangle){120,59,1040,55},72,ink);
    label(ui,fonts,"Restabeleça a energia da estufa",(Rectangle){120,114,1040,28},28,ink);
    for (int i = 0; i < WORLD_RELAY_RULE_COUNT; i++) {
        Rectangle box = {205+i*295,145,280,44};
        skin(assets,RELAY_BUTTON_SKIN,box,i < step ? sage : i == step ? amber : WHITE,10);
        label(ui,fonts,TextFormat("Etapa %d",i+1),(Rectangle){box.x+12,box.y+6,box.width-24,32},28,ink);
    }
    label(ui,fonts,TextFormat("Etapa %d de %d",step+1,WORLD_RELAY_RULE_COUNT),(Rectangle){120,192,1040,24},24,ink);
    DrawLine(90,219,1190,219,(Color){91,114,85,255});
    skin(assets,RELAY_PANEL_SKIN,(Rectangle){150,228,980,124},WHITE,18);
    label(ui,fonts,"Objetivo",(Rectangle){180,237,920,25},28,ink);
    label(ui,fonts,formulas[step],(Rectangle){180,264,920,46},36,ink);
    label(ui,fonts,guides[step],(Rectangle){180,313,920,27},24,ink);
    int focus = world->relay_cursor;
    for (int i = 0; i < WORLD_RELAY_COUNT; i++) {
        Rectangle box = world_relay_bounds(i);
        bool on = (world->relay_mask & (1u << i)) != 0;
        skin(assets,RELAY_PANEL_SKIN,box,on ? sage : WHITE,16);
        Rectangle name = {box.x+40,box.y+12,box.width-80,31};
        skin(assets,RELAY_BUTTON_SKIN,name,WHITE,7);
        label(ui,fonts,TextFormat("Relé %c",'A'+i),name,28,ink);
        control(assets,(Rectangle){box.x+72,box.y+47,136,82},on);
        label(ui,fonts,on ? "Ligado" : "Desligado",(Rectangle){box.x+24,box.y+131,box.width-48,26},24,ink);
        Rectangle key = {box.x+58,box.y+159,box.width-116,26};
        skin(assets,RELAY_BUTTON_SKIN,key,WHITE,6);
        label(ui,fonts,TextFormat("Tecla %d",i+1),key,20,ink);
        if (i == focus) pointer(fonts,box);
    }
    label(ui,fonts,world->relay_rejected ? "Ainda não! Siga a dica e tente de novo." : hints[step],
        (Rectangle){100,580,820,30},22,world->relay_rejected ? (Color){153,52,39,255} : ink);
    label(ui,fonts,TextFormat("Tentativas: %u",world->relay_attempts),(Rectangle){960,580,220,30},22,ink);
    DrawLine(90,619,1190,619,(Color){91,114,85,255});
    action(ui,fonts,assets,world_relay_close_bounds(),"Voltar · ESC",false);
    label(ui,fonts,"1/2/3 ou clique: alternar relés",(Rectangle){350,641,550,25},22,ink);
    action(ui,fonts,assets,world_relay_validate_bounds(),"Validar · ENTER",true);
}
