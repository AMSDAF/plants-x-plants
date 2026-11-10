/* Verifica nomes, foco, transições e sessão com os módulos reais e OpenGL. */
#include "game.h"
#include "selection_ui.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void check_queue_codepoint(void *window, unsigned int codepoint);

static void input(unsigned int type,int a,int b)
{
    PlayAutomationEvent((AutomationEvent){.type=type,.params={a,b,0,0}});
}

static void tick(Game *game)
{
    game_update(game);
    BeginDrawing(); game_draw(game); EndDrawing();
}

static void key(Game *game,int code)
{
    input(2,code,0); tick(game);
    input(1,code,0); tick(game); tick(game);
}

static void click(Game *game,int x,int y)
{
    input(7,x,y); input(6,0,0); tick(game);
    input(5,0,0); tick(game); tick(game);
}

static void settle(Game *game)
{
    for (int i=0;i<45;i++) tick(game);
}

static void text(Game *game,const char *value)
{
    while (*value) {
        int bytes=0;
        int codepoint=GetCodepointNext(value,&bytes);
        check_queue_codepoint(GetWindowHandle(),(unsigned int)codepoint);
        BeginDrawing(); EndDrawing();
        tick(game);
        value+=bytes;
    }
}

static void append(NameEntryScreen *screen,const char *value)
{
    while (*value) {
        int bytes=0,codepoint=GetCodepointNext(value,&bytes);
        assert(name_entry_append(screen,codepoint));
        value+=bytes;
    }
}

static void capture(Game *game,RenderTexture2D target,const char *path)
{
    Game before=*game;
    BeginTextureMode(target); game_draw(game); EndTextureMode();
    assert(memcmp(&before,game,sizeof(before))==0);
    Image image=LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    if (strstr(path,".ppm")) {
        FILE *file=fopen(path,"wb"); assert(file);
        fprintf(file,"P6\n%d %d\n255\n",image.width,image.height);
        ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        size_t size=(size_t)image.width*image.height*3;
        assert(fwrite(image.data,1,size,file)==size);
        assert(fclose(file)==0);
    } else assert(ExportImage(image,path));
    UnloadImage(image);
}

static void check_names(void)
{
    NameEntryScreen screen;
    char result[PLAYER_NAME_CAPACITY];
    name_entry_init(&screen);
    assert(screen.count==0 && screen.draft[0]==0);
    assert(!name_entry_confirm(&screen,result) && screen.error);
    append(&screen,"   ");
    assert(!name_entry_confirm(&screen,result));
    name_entry_init(&screen);
    append(&screen,"  Antônio  ");
    assert(screen.count==11 && name_entry_confirm(&screen,result));
    assert(strcmp(result,"Antônio")==0);
    name_entry_init(&screen); append(&screen,"João");
    assert(screen.count==4 && strlen(screen.draft)==5);
    name_entry_backspace(&screen); name_entry_backspace(&screen);
    assert(screen.count==2 && strcmp(screen.draft,"Jo")==0);
    name_entry_init(&screen);
    for (int i=0;i<12;i++) assert(name_entry_append(&screen,0xe7));
    assert(screen.count==12 && strlen(screen.draft)==24 && !name_entry_append(&screen,'a'));
    name_entry_backspace(&screen);
    assert(screen.count==11 && strlen(screen.draft)==22);
    name_entry_init(&screen);
    assert(name_entry_append(&screen,0x10400));
    assert(strlen(screen.draft)==4 && screen.count==1);
    name_entry_backspace(&screen); assert(screen.draft[0]==0 && screen.count==0);
    for (int i=0;i<12;i++) assert(name_entry_append(&screen,0x10400));
    assert(strlen(screen.draft)==48 && screen.count==12 && !name_entry_append(&screen,0x10400));
    for (int i=0;i<12;i++) name_entry_backspace(&screen);
    assert(!name_entry_append(&screen,0x1f600));
    assert(!name_entry_append(&screen,'\n') && !name_entry_append(&screen,0x7f));
    append(&screen,"D'Ávila-2");
    assert(name_entry_confirm(&screen,result));
    Font font=GetFontDefault();
    int accents[]={0xe7,0xe3,0xf4,0xc1};
    for (unsigned int i=0;i<sizeof(accents)/sizeof(accents[0]);i++)
        assert(font.glyphs[GetGlyphIndex(font,accents[i])].value==accents[i]);
}

static void walk_axis(Game *game, bool horizontal, float target)
{
    float *position = horizontal ? &game->player.feet.x : &game->player.feet.y;
    bool forward = target > *position;
    int code = horizontal ? (forward ? KEY_D : KEY_A) : (forward ? KEY_S : KEY_W);
    input(2, code, 0);
    for (int i = 0; i < 400 && (forward ? *position < target : *position > target); i++) {
        float before = *position;
        player_update(&game->player, &game->world, 1.0f / 60.0f);
        if (*position == before) break;
    }
    input(1, code, 0);
    tick(game);
}

static void win_guardian(Game *game)
{
    BattleState *battle = &game->battle_screen.battle;
    for (int i = 0; i < battle->enemy_team.count; i++) {
        battle->enemy_team.plants[i].current_hp = 1;
        key(game, KEY_ONE);
        if (battle->state == BATTLE_STATE_PLAYER_WIN) break;
        assert(battle->state == BATTLE_STATE_REPLACEMENT && battle->current_turn == 1);
        assert(Battle_SwitchPlant(battle, i + 1));
        battle->enemy_team.plants[i + 1].moves[0].accuracy = 0;
        assert(Battle_ExecuteMove(battle, 0));
    }
    assert(battle->state == BATTLE_STATE_PLAYER_WIN);
    key(game, KEY_ENTER);
    assert(game->screen == GAME_EXPLORATION);
}

static void lose_guardian(Game *game)
{
    BattleState *battle = &game->battle_screen.battle;
    for (int i = 0; i < battle->player_team.count; i++) {
        Plant *plant = &battle->player_team.plants[i];
        plant->current_hp = 1;
        int accuracy = plant->moves[0].accuracy;
        plant->moves[0].accuracy = 0;
        assert(Battle_ExecuteMove(battle, 0));
        plant->moves[0].accuracy = accuracy;
        assert(Battle_ExecuteMove(battle, 0));
        if (battle->state == BATTLE_STATE_ENEMY_WIN) break;
        assert(Battle_SwitchPlant(battle, i + 1));
    }
    assert(battle->state == BATTLE_STATE_ENEMY_WIN);
    key(game, KEY_ENTER);
    assert(game->screen == GAME_EXPLORATION);
}

static void check_greenhouse(Game *game)
{
    World *world = &game->world;
    for (int row = 0; row < 16; row++) {
        World sample = {.alarm = (row & 8) != 0, .water = (row & 4) != 0,
                        .light = (row & 2) != 0, .key_collected = (row & 1) != 0};
        assert(world_gate_rule(&sample) == (row >= 5 && row <= 7));
    }
    for (int row = 0; row < 4; row++) {
        World sample = {.north_channel = (row & 2) != 0, .south_channel = (row & 1) != 0};
        assert(world_bridge_open(&sample) == (row == 1 || row == 2));
    }
    const unsigned int solution = (1u << 1) | (1u << 4) | (1u << 6) | (1u << 7);
    int solutions = 0;
    for (unsigned int mask = 0; mask < (1u << WORLD_RELAY_COUNT); mask++) {
        if (world_relays_match(mask)) {
            assert(mask == solution);
            solutions++;
        }
    }
    assert(solutions == 1 && !world_relays_match(solution | (1u << WORLD_RELAY_COUNT)));

    World power_route;
    world_init(&power_route);
    Vector2 alarm = {230, 580}, power = {250, 890}, valve = {1070, 305};
    world_interact(&power_route, &alarm);
    world_interact(&power_route, &power);
    world_interact(&power_route, &valve);
    assert(world_gate_open(&power_route) && !power_route.key_collected);
    world_update_position(&power_route, (Vector2){1620, 420});
    world_interact(&power_route, &power);
    assert(!world_gate_rule(&power_route) && world_gate_open(&power_route));

    RenderTexture2D target = LoadRenderTexture(1280, 720);
    capture(game, target, "build/name_entry_preview/greenhouse_start.png");
    walk_axis(game, true, 960);
    assert(game->player.feet.x < 785);
    walk_axis(game, true, 230);
    walk_axis(game, false, 580);
    key(game, KEY_E);
    walk_axis(game, true, 420);
    key(game, KEY_E);
    assert(!world->alarm && world->alarm_note);
    walk_axis(game, true, 535);
    walk_axis(game, false, 300);
    key(game, KEY_E);
    assert(game->player.feet.y > 700);
    walk_axis(game, true, 625);
    walk_axis(game, false, 888);
    key(game, KEY_E);
    assert(game->screen == GAME_BATTLE && game->encounter == WORLD_ENCOUNTER_ALOE);
    key(game, KEY_ENTER);
    key(game, KEY_ENTER);
    assert(game->battle_screen.setup_side < 0);
    lose_guardian(game);
    assert(game->player.feet.x == world_spawn().x);
    assert(game->campaign.has_team && !world->key_collected && world->alarm_note && !world->alarm);
    walk_axis(game, true, 535);
    walk_axis(game, false, 300);
    key(game, KEY_E);
    walk_axis(game, true, 625);
    walk_axis(game, false, 888);
    key(game, KEY_E);
    assert(game->screen == GAME_BATTLE && game->battle_screen.setup_side < 0);
    capture(game, target, "build/name_entry_preview/greenhouse_guard.png");
    win_guardian(game);
    assert(world->key_collected);
    walk_axis(game, true, 250);
    key(game, KEY_E);
    assert(world->light);
    walk_axis(game, true, 535);
    walk_axis(game, false, 743);
    key(game, KEY_E);
    assert(game->player.feet.y > 700);
    walk_axis(game, false, 888);
    walk_axis(game, true, 250);
    key(game, KEY_E);
    assert(!world->light);
    walk_axis(game, true, 535);
    walk_axis(game, false, 743);
    key(game, KEY_E);
    assert(game->player.feet.y == 300);
    walk_axis(game, true, 675);
    key(game, KEY_E);
    assert(world_bridge_open(world));
    walk_axis(game, false, 536);
    key(game, KEY_E);
    assert(world->north_channel && world->south_channel && !world_bridge_open(world));
    walk_axis(game, false, 420);
    walk_axis(game, true, 960);
    assert(game->player.feet.x < 785);
    walk_axis(game, true, 675);
    walk_axis(game, false, 305);
    key(game, KEY_E);
    assert(!world->north_channel && world->south_channel && world_bridge_open(world));
    walk_axis(game, false, 420);
    walk_axis(game, true, 1070);
    walk_axis(game, false, 305);
    key(game, KEY_E);
    assert(world->water && world_gate_rule(world));
    walk_axis(game, true, 1630);
    assert(game->player.feet.x < 1515); /* A parede continua sólida fora do vão. */
    walk_axis(game, false, 420);
    walk_axis(game, true, 1620);
    assert(world->gate_unlocked && game->player.feet.x > 1547);
    capture(game, target, "build/name_entry_preview/greenhouse_open.png");
    walk_axis(game, true, 2150);
    key(game, KEY_E);
    assert(!world->relay_panel_open);
    walk_axis(game, true, 1760);
    key(game, KEY_E);
    assert(game->screen == GAME_BATTLE && game->encounter == WORLD_ENCOUNTER_SUNFLOWER);
    win_guardian(game);
    assert(world->guardian_defeated);
    walk_axis(game, true, 2150);
    key(game, KEY_E);
    key(game, KEY_ENTER);
    assert(world->relay_panel_open && world->relay_rejected && world->relay_step == 0);
    click(game, 289, 237); /* B satisfaz o primeiro XOR. */
    key(game, KEY_ENTER);
    assert(world->relay_step == 1 && !world->relay_rejected);
    key(game, KEY_ENTER);
    assert(world->relay_step == 2 && world->relay_attempts == 3);
    key(game, KEY_ENTER);
    assert(world->relay_step == 2 && world->relay_attempts == 3); /* Ainda falta a pista. */
    key(game, KEY_ESCAPE);
    walk_axis(game, true, 1070);
    walk_axis(game, false, 605);
    key(game, KEY_E);
    assert(world->canal_note);
    walk_axis(game, false, 420);
    walk_axis(game, true, 2150);
    key(game, KEY_E);
    assert(world->relay_panel_open && !world->completed && world->relay_step == 2);
    Vector2 before = game->player.feet;
    key(game, KEY_D);
    assert(game->player.feet.x == before.x && game->player.feet.y == before.y);
    key(game, KEY_TWO); /* Quebra a primeira regra enquanto a terceira está visível. */
    key(game, KEY_ENTER);
    assert(world->relay_attempts == 4 && world->relay_rejected && world->relay_step == 0);
    key(game, KEY_TWO);
    key(game, KEY_SIX); /* B e F satisfazem as oito primeiras regras. */
    for (int i = 0; i < WORLD_RELAY_RULE_COUNT - 1; i++) {
        assert(world->relay_step == i && !world->completed);
        key(game, KEY_ENTER);
    }
    key(game, KEY_ENTER);
    assert(world->relay_step == WORLD_RELAY_RULE_COUNT - 1 && world->relay_rejected && !world->completed);
    capture(game, target, "build/name_entry_preview/greenhouse_relays.png");
    key(game, KEY_SIX);
    key(game, KEY_FIVE);
    key(game, KEY_SEVEN);
    key(game, KEY_EIGHT);
    key(game, KEY_LEFT);
    assert(world->relay_step == WORLD_RELAY_RULE_COUNT - 2);
    unsigned int attempts = world->relay_attempts;
    key(game, KEY_ESCAPE);
    assert(game->screen == GAME_EXPLORATION && !world->relay_panel_open);
    key(game, KEY_E);
    assert(world->relay_panel_open && world->relay_mask == solution && world->relay_attempts == attempts);
    assert(world->relay_step == WORLD_RELAY_RULE_COUNT - 2);
    key(game, KEY_ENTER);
    assert(world->relay_step == WORLD_RELAY_RULE_COUNT - 1 && !world->completed);
    click(game, 232, 615);
    assert(world->completed);
    assert(!world->relay_panel_open && world->relay_attempts == attempts + 2);
    capture(game, target, "build/name_entry_preview/greenhouse_complete.png");
    key(game, KEY_ENTER);
    assert(game->screen == GAME_MENU);
    UnloadRenderTexture(target);
    puts("PASS: mapa, duas rotas, derrota e vitórias, pistas, 256 combinações da central e saída.");
}

int main(void)
{
    assert(DirectoryExists("build/name_entry_preview") || MakeDirectory("build/name_entry_preview")==0);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Verificacao de nome de campanha");
    assert(IsWindowReady()); SetExitKey(KEY_NULL); SetTargetFPS(60);
    check_names();
    Game game; game_init(&game);
    game.screen=GAME_TRAINER_SELECT;
    for (int gender=0;gender<2;gender++) {
        trainer_select_screen_init(&game.trainer_select);
        game.trainer_select.selected=(Trainer)gender;
        name_entry_init(&game.name_entry);
        game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
        float male=game.trainer_select.male_idle_time, female=game.trainer_select.female_idle_time;
        input(2,KEY_ENTER,0); tick(&game);
        assert(game.screen==GAME_NAME_ENTRY && game.name_entry.direction==1);
        text(&game,"AD"); /* Texto durante a transição deve ser descartado. */
        settle(&game);
        assert(game.name_entry.progress==1 && game.name_entry.input_blocked);
        assert(game.name_entry.draft[0]==0 && game.name_entry.focus==0);
        assert(game.trainer_select.male_idle_time>male && game.trainer_select.female_idle_time>female);
        assert(game.trainer_select.selected==(Trainer)gender);
        input(1,KEY_ENTER,0); tick(&game); tick(&game);
        text(&game,gender ? "  João  " : "Antônio");
        assert(strcmp(game.name_entry.draft,gender ? "  João  " : "Antônio")==0);
        key(&game,KEY_A); key(&game,KEY_D); key(&game,KEY_LEFT); key(&game,KEY_RIGHT);
        assert(game.trainer_select.selected==(Trainer)gender);
        key(&game,KEY_TAB); assert(game.name_entry.focus==1);
        input(2,KEY_LEFT_SHIFT,0); key(&game,KEY_TAB); input(1,KEY_LEFT_SHIFT,0); tick(&game);
        assert(game.name_entry.focus==0);
        click(&game,950,570); /* Confirmar por mouse transfere nome e personagem. */
        assert(game.screen==GAME_EXPLORATION && game.campaign.ready);
        assert(game.campaign.trainer==(Trainer)gender && game.player.trainer==(Trainer)gender);
        assert(strcmp(game.campaign.player_name,gender ? "João" : "Antônio")==0);
        key(&game,KEY_ESCAPE);
        game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
        game.name_entry.progress=0;
        key(&game,KEY_ENTER); settle(&game);
        assert(game.screen==GAME_NAME_ENTRY);
        char draft[PLAYER_NAME_CAPACITY]; strcpy(draft,game.name_entry.draft);
        key(&game,KEY_ESCAPE); settle(&game);
        assert(game.screen==GAME_TRAINER_SELECT && game.trainer_select.selected==(Trainer)gender);
        assert(strcmp(draft,game.name_entry.draft)==0);
    }
    /* Foco, validação e repetição de Backspace pelo módulo real. */
    game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
    name_entry_init(&game.name_entry);
    key(&game,KEY_ENTER); settle(&game);
    key(&game,KEY_ENTER); assert(game.screen==GAME_NAME_ENTRY && game.name_entry.error);
    text(&game,"çãôéABCDEFGHI");
    assert(game.name_entry.count==12);
    input(2,KEY_BACKSPACE,0); tick(&game);
    for (int i=0;i<12;i++) name_entry_update(&game.name_entry,true,.1f);
    input(1,KEY_BACKSPACE,0); tick(&game);
    assert(game.name_entry.count==0 && game.name_entry.draft[0]==0);
    text(&game,"João");
    click(&game,680,570); settle(&game);
    assert(game.screen==GAME_TRAINER_SELECT && strcmp(game.name_entry.draft,"João")==0);
    key(&game,KEY_ENTER); settle(&game);
    assert(strcmp(game.name_entry.draft,"João")==0 && game.name_entry.focus==0);
    key(&game,KEY_TAB); key(&game,KEY_TAB); assert(game.name_entry.focus==2);
    key(&game,KEY_ENTER); assert(game.screen==GAME_EXPLORATION);
    check_greenhouse(&game);
    /* Botão Voltar focado por teclado e soltura de mouse mantido na entrada. */
    game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
    name_entry_init(&game.name_entry);
    input(7,820,360); input(6,0,0); tick(&game);
    key(&game,KEY_ENTER); settle(&game);
    assert(game.screen==GAME_NAME_ENTRY && game.name_entry.input_blocked);
    text(&game,"Não"); assert(game.name_entry.count==0);
    input(5,0,0); tick(&game); tick(&game);
    text(&game,"João");
    key(&game,KEY_TAB); assert(game.name_entry.focus==1);
    input(2,KEY_ENTER,0); tick(&game); settle(&game);
    assert(game.screen==GAME_TRAINER_SELECT && game.input_blocked);
    input(1,KEY_ENTER,0); tick(&game); tick(&game);
    assert(!game.input_blocked);
    /* Capturas temporais controladas: relógios avançam com o mesmo dt da transição. */
    RenderTexture2D target=LoadRenderTexture(1280,720);
    for (int gender=0;gender<2;gender++) {
        trainer_select_screen_init(&game.trainer_select);
        game.trainer_select.selected=(Trainer)gender;
        name_entry_init(&game.name_entry);
        game.screen=GAME_TRAINER_SELECT;
        capture(&game,target,TextFormat("build/name_entry_preview/selection_%d.ppm",gender));
        game.screen=GAME_NAME_ENTRY;
        name_entry_begin(&game.name_entry,true);
        for (int i=0;i<=12;i++) {
            Rectangle card=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
            assert(card.x>=172 && card.width>=308 && card.width<=336);
            capture(&game,target,TextFormat("build/name_entry_preview/forward_%d_%02d.ppm",gender,i));
            if (i<12) {
                trainer_select_screen_update(&game.trainer_select,false,.05f);
                name_entry_update(&game.name_entry,false,.05f);
            }
        }
        Rectangle last=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
        Rectangle desired=name_entry_portrait_bounds();
        assert(memcmp(&last,&desired,sizeof(last))==0 && game.name_entry.progress==1);
        capture(&game,target,TextFormat("build/name_entry_preview/empty_%d.png",gender));
        append(&game.name_entry,gender ? "João" : "Antônio");
        capture(&game,target,TextFormat("build/name_entry_preview/final_%d.png",gender));
        name_entry_begin(&game.name_entry,false);
        for (int i=0;i<=12;i++) {
            capture(&game,target,TextFormat("build/name_entry_preview/reverse_%d_%02d.ppm",gender,i));
            if (i<12) {
                trainer_select_screen_update(&game.trainer_select,false,.05f);
                name_entry_update(&game.name_entry,false,.05f);
            }
        }
        last=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
        desired=trainer_select_card_bounds((Trainer)gender);
        assert(memcmp(&last,&desired,sizeof(last))==0 && game.name_entry.progress==0);
        game.screen=GAME_TRAINER_SELECT;
        capture(&game,target,TextFormat("build/name_entry_preview/selection_end_%d.ppm",gender));
    }
    UnloadRenderTexture(target); game_shutdown(&game); CloseWindow();
    puts("PASS: UTF-8/acentos, 12 codepoints, Backspace multibyte/repetido, nomes vazios, foco, fila nativa, descarte, entrada mantida, sessão, rascunho, dois sentidos/personagens e desenho imutável.");
    return 0;
}
