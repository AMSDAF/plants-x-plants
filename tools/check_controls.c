/* Reproduz regressões de foco, teclado e cliques com os módulos reais, sem assets. */
#include "game.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void event(int type, int a, int b)
{
    PlayAutomationEvent((AutomationEvent){.type=(unsigned int)type,.params={a,b,0,0}});
}
static void frame(void) { BeginDrawing(); ClearBackground(BLACK); EndDrawing(); }
static void battle_key(BattleScreen *screen, int key)
{
    event(2,key,0); battle_screen_update(screen,.01f); frame();
    event(1,key,0); battle_screen_update(screen,.01f); frame();
}
static ResultAction result_key(ResultScreen *screen, int key)
{
    event(2,key,0); ResultAction action = result_screen_update(screen,true,.01f); frame();
    event(1,key,0); result_screen_update(screen,true,.01f); frame();
    return action;
}
static BattleScreen fresh_battle(void)
{
    BattleScreen screen = {.setup_side=-1};
    Team own = {0}, enemy = {0};
    Team_SelectSpecies(&own,0,SPECIES_CACTUS);
    Team_SelectSpecies(&own,1,SPECIES_CLOVER);
    Team_SelectSpecies(&enemy,0,SPECIES_ALOE);
    Battle_Init(&screen.battle,own,enemy);
    return screen;
}

static void check_battle_input(void)
{
    BattleScreen screen = fresh_battle();
    battle_key(&screen,KEY_TAB);
    assert(screen.show_statuses);
    BattleState before = screen.battle;
    battle_key(&screen,KEY_ONE); battle_key(&screen,KEY_W); battle_key(&screen,KEY_ENTER);
    assert(screen.pending_action == BATTLE_PENDING_NONE);
    assert(memcmp(&before,&screen.battle,sizeof(before)) == 0);
    /* Um clique iniciado no painel não pode vazar quando ele fecha. */
    event(7,650,545); event(6,0,0); battle_screen_update(&screen,.01f); frame();
    battle_key(&screen,KEY_TAB);
    event(5,0,0); battle_screen_update(&screen,.01f); frame();
    assert(!screen.show_statuses && screen.menu == BATTLE_MENU_ROOT);
    assert(screen.pending_action == BATTLE_PENDING_NONE);
    screen.pending_action = BATTLE_PENDING_MOVE; screen.action_timer = 1;
    battle_key(&screen,KEY_TAB);
    float timer = screen.action_timer;
    for (int i = 0; i < 10; i++) battle_screen_update(&screen,.1f);
    assert(screen.action_timer == timer && memcmp(&before,&screen.battle,sizeof(before)) == 0);

    screen = fresh_battle(); battle_key(&screen,KEY_W);
    assert(screen.menu_cursor == 2 && screen.pending_action == BATTLE_PENDING_NONE);
    screen.menu = BATTLE_MENU_TEAM; screen.menu_cursor = 0;
    battle_key(&screen,KEY_KP_2);
    assert(screen.pending_action == BATTLE_PENDING_ROUND);
    assert(screen.pending_player_action.type == BATTLE_ACTION_SWITCH && screen.pending_player_action.index == 1);
    screen = fresh_battle(); battle_key(&screen,KEY_KP_1);
    assert(screen.pending_player_action.type == BATTLE_ACTION_MOVE && screen.pending_player_action.index == 0);
    screen = fresh_battle(); screen.battle.player_team.plants[0].current_hp = 0;
    screen.battle.current_turn = 0;
    screen.battle.state = BATTLE_STATE_REPLACEMENT;
    battle_key(&screen,KEY_TWO);
    assert(screen.pending_action == BATTLE_PENDING_SWITCH && screen.pending_plant_index == 1);
}

static void check_results(void)
{
    ResultScreen screen; result_screen_begin(&screen); screen.defeat = true;
    assert(result_key(&screen,KEY_ESCAPE) == RESULT_NONE && screen.abandon_confirm);
    assert(screen.cursor == 0);
    assert(result_key(&screen,KEY_ENTER) == RESULT_NONE && !screen.abandon_confirm);
    result_key(&screen,KEY_ESCAPE);
    assert(result_key(&screen,KEY_ESCAPE) == RESULT_NONE && !screen.abandon_confirm);
    result_key(&screen,KEY_ESCAPE); result_key(&screen,KEY_D);
    assert(result_key(&screen,KEY_SPACE) == RESULT_SECONDARY);

    result_screen_begin(&screen); screen.quiz_open = true; screen.question_index = 0;
    assert(result_key(&screen,KEY_ESCAPE) == RESULT_NONE && screen.quiz_open);
    result_key(&screen,KEY_DOWN); assert(screen.cursor == 1);
    result_key(&screen,KEY_W); assert(screen.cursor == 0);
    assert(result_key(&screen,KEY_KP_1) == RESULT_CORRECT);
    screen.question_index = 1;
    assert(result_key(&screen,KEY_KP_2) == RESULT_CORRECT);

    result_screen_begin(&screen);
    Rectangle box = result_screen_button_bounds(0);
    event(7,(int)box.x+20,(int)box.y+20); event(6,0,0);
    assert(result_screen_update(&screen,true,.01f) == RESULT_NONE); frame();
    event(7,0,0); event(5,0,0);
    assert(result_screen_update(&screen,true,.01f) == RESULT_NONE); frame();
    event(7,(int)box.x+20,(int)box.y+20); event(6,0,0);
    assert(result_screen_update(&screen,true,.01f) == RESULT_NONE); frame();
    event(5,0,0);
    assert(result_screen_update(&screen,true,.01f) == RESULT_PRIMARY); frame();
}

static void check_logic_and_relays(void)
{
    Team team = {0}; Team_SelectSpecies(&team,0,SPECIES_CLOVER);
    LogicFormula formulas[MAX_TEAM_SIZE] = {0};
    int moves[MAX_TEAM_SIZE] = {-1,-1,-1,-1};
    unsigned int solved = 0;
    LogicSetupScreen screen = {0};
    logic_setup_begin(&screen,&team,formulas,moves); screen.question_indices[0] = 0;
    event(2,KEY_KP_1,0); logic_setup_update(&screen,&team,&solved,formulas,moves); frame();
    event(1,KEY_KP_1,0); logic_setup_update(&screen,&team,&solved,formulas,moves); frame();
    assert(solved == 1);
    event(2,KEY_S,0); logic_setup_update(&screen,&team,&solved,formulas,moves); frame();
    event(1,KEY_S,0); frame(); assert(screen.focus == 2);
    World world; world_init(&world); world.relay_panel_open = true;
    event(2,KEY_D,0); world_update_relay_panel(&world,"Teste"); frame();
    event(1,KEY_D,0); frame(); assert(world.relay_cursor == 1);
    event(2,KEY_SPACE,0); world_update_relay_panel(&world,"Teste"); frame();
    event(1,KEY_SPACE,0); frame(); assert(world.relay_mask == 2);
    event(2,KEY_KP_3,0); world_update_relay_panel(&world,"Teste"); frame();
    event(1,KEY_KP_3,0); frame(); assert(world.relay_mask == 6);
}

static void check_intro_click(void)
{
    IntroScreen screen = {.stage=INTRO_STARTERS};
    event(7,150,220); event(6,0,0);
    intro_screen_update(&screen,true,.01f); frame(); assert(screen.count == 0);
    event(7,0,0); event(5,0,0);
    intro_screen_update(&screen,true,.01f); frame(); assert(screen.count == 0);
    event(7,150,220); event(6,0,0);
    intro_screen_update(&screen,true,.01f); frame(); assert(screen.count == 0);
    event(5,0,0); intro_screen_update(&screen,true,.01f); frame();
    assert(screen.count == 1 && screen.selected[0] == 0);

    TrainerSelectScreen trainer; trainer_select_screen_init(&trainer);
    Rectangle card = trainer_select_card_bounds(TRAINER_FEMALE);
    event(7,(int)(card.x+card.width/2),(int)(card.y+card.height/2)); event(6,0,0);
    trainer_select_screen_update(&trainer,true,.01f); frame();
    assert(trainer.selected == TRAINER_MALE);
    event(7,0,0); event(5,0,0);
    trainer_select_screen_update(&trainer,true,.01f); frame();
    assert(trainer.selected == TRAINER_MALE);
    event(7,(int)(card.x+card.width/2),(int)(card.y+card.height/2)); event(6,0,0);
    trainer_select_screen_update(&trainer,true,.01f); frame();
    event(5,0,0); trainer_select_screen_update(&trainer,true,.01f); frame();
    assert(trainer.selected == TRAINER_FEMALE);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN); SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Regressões de controles"); SetExitKey(KEY_NULL);
    assert(IsWindowReady());
    check_battle_input(); check_results(); check_logic_and_relays(); check_intro_click();
    CloseWindow();
    puts("PASS: painel modal, WASD sem troca acidental, reservas numericas, numpad, confirmacao de desistencia e clique cancelavel.");
    return 0;
}
