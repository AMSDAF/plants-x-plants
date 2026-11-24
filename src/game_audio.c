/* Traduz mudanças de tela, exploração e combate em efeitos sonoros e trilhas. */
#include "game_audio.h"
#include "plant_data.h"

#include <math.h>
#include <stddef.h>

/* Distância, em pixels do mundo, entre dois passos audíveis. */
static const float step_length = 50.0f;
/* Um bipe a cada tantas letras reveladas no diálogo. */
static const int dialogue_blip_interval = 3;

static unsigned int world_controls(const World *world)
{
    return (unsigned int)world->water | (unsigned int)world->light << 1 |
           (unsigned int)world->gate_unlocked << 2 | (unsigned int)world->drain_open << 3 |
           (unsigned int)world->north_channel << 4 | (unsigned int)world->south_channel << 5 |
           (unsigned int)world->maintenance_key << 6;
}

static const Team *side_team(const BattleState *battle, int side)
{
    return side == 0 ? &battle->player_team : &battle->enemy_team;
}

GameAudioSnapshot game_audio_capture(const Game *game)
{
    const World *world = &game->world;
    const LogicSetupScreen *logic = &game->logic_setup;
    const BattleScreen *screen = &game->battle_screen;
    GameAudioSnapshot shot = {
        .screen = game->screen,
        .result_cursor = game->result.cursor,
        .feedback_sequence = screen->battle.feedback.sequence,
        .trainer_selected = game->trainer_select.selected,
        .name_count = game->name_entry.count,
        .name_focus = game->name_entry.focus,
        .name_error = game->name_entry.error,
        .intro_stage = game->intro.stage,
        .intro_cursor = game->intro.cursor,
        .intro_count = game->intro.count,
        .location = world->location,
        .feet = game->player.feet,
        .dialogue_open = world->dialogue_title != NULL,
        .dialogue_page = world->dialogue_page,
        .dialogue_characters = world->dialogue_visible_characters,
        .relay_mask = world->relay_mask,
        .relay_step = world->relay_step,
        .relay_rejected = world->relay_rejected,
        .relay_panel_open = world->relay_panel_open,
        .logic_quiz_open = world->logic_quiz_open,
        .world_controls = world_controls(world),
        .completed = world->completed,
        .logic_selected = logic->selected,
        .logic_focus = logic->focus,
        .logic_dropdown = logic->dropdown,
        .logic_option = logic->option,
        .logic_confirmed = logic->confirmed_mask,
        .logic_wrong_answers = logic->wrong_answer_count,
        .pending_action = screen->pending_action,
        .battle_menu = screen->menu,
        .battle_cursor = screen->menu_cursor,
        .setup_side = screen->setup_side,
        .setup_cursor = screen->roster_cursor,
        .setup_slot = screen->team_slot,
        .setup_plant = screen->draft_plant_index,
        .setup_move = screen->draft_move_index,
        .setup_draft = screen->draft,
        .battle_state = screen->battle.state,
        .round_step = screen->round.step,
        .round_message_index = screen->round_message_index,
    };
    for (int side = 0; side < 2; side++) {
        const Team *team = side_team(&screen->battle, side);
        shot.team_count[side] = team->count;
        shot.active_index[side] = team->active_index;
        for (int i = 0; i < MAX_TEAM_SIZE; i++)
            shot.hp[side][i] = i < team->count ? team->plants[i].current_hp : 0;
    }
    return shot;
}

static void react_screen_change(GameAudio *audio, GameScreen from, GameScreen to)
{
    bool forward = (from == GAME_MENU && to == GAME_LEADERBOARD) ||
                   (from == GAME_LEADERBOARD && to == GAME_TRAINER_SELECT) ||
                   (from == GAME_TRAINER_SELECT && to == GAME_NAME_ENTRY) ||
                   (from == GAME_NAME_ENTRY && to == GAME_INTRO) ||
                   (from == GAME_INTRO && to == GAME_EXPLORATION);
    bool backward = (from == GAME_LEADERBOARD && to == GAME_MENU) ||
                    (from == GAME_TRAINER_SELECT && to == GAME_MENU) ||
                    (from == GAME_NAME_ENTRY && to == GAME_TRAINER_SELECT) ||
                    (from == GAME_INTRO && to == GAME_NAME_ENTRY) ||
                    (from == GAME_EXPLORATION && to == GAME_MENU);
    if (to == GAME_BATTLE) audio_play(audio, SFX_BATTLE_START);
    else if (from == GAME_ENDING || from == GAME_DEFEAT) audio_play(audio, SFX_UI_CONFIRM);
    else if (forward) audio_play(audio, SFX_UI_CONFIRM);
    else if (backward) audio_play(audio, SFX_UI_BACK);
    audio->step_distance = step_length;
}

static void react_menus(GameAudio *audio, const GameAudioSnapshot *before, const Game *game)
{
    if (game->screen == GAME_TRAINER_SELECT && before->trainer_selected != game->trainer_select.selected)
        audio_play(audio, SFX_UI_MOVE);
    if (game->screen == GAME_NAME_ENTRY) {
        const NameEntryScreen *name = &game->name_entry;
        if (name->error && !before->name_error) audio_play(audio, SFX_UI_INVALID);
        else if (name->count != before->name_count) audio_play(audio, SFX_UI_TYPE);
        else if (name->focus != before->name_focus) audio_play(audio, SFX_UI_MOVE);
    }
    if (game->screen == GAME_INTRO) {
        const IntroScreen *intro = &game->intro;
        if (intro->stage > before->intro_stage || intro->count > before->intro_count)
            audio_play(audio, SFX_UI_CONFIRM);
        else if (intro->stage < before->intro_stage || intro->count < before->intro_count)
            audio_play(audio, SFX_UI_BACK);
        else if (intro->cursor != before->intro_cursor)
            audio_play(audio, SFX_UI_MOVE);
    }
}

static void react_steps(GameAudio *audio, const GameAudioSnapshot *before, const Game *game)
{
    if (!game->player.moving) {
        /* O primeiro passo soa logo que o jogador volta a andar. */
        audio->step_distance = step_length;
        return;
    }
    float dx = game->player.feet.x - before->feet.x;
    float dy = game->player.feet.y - before->feet.y;
    audio->step_distance += sqrtf(dx * dx + dy * dy);
    if (audio->step_distance >= step_length) {
        audio->step_distance = fmodf(audio->step_distance, step_length);
        audio_play(audio, SFX_STEP_GRASS);
    }
}

static void react_logic_setup(GameAudio *audio, const GameAudioSnapshot *before, const Game *game)
{
    const LogicSetupScreen *logic = &game->logic_setup;
    if (logic->wrong_answer_count != before->logic_wrong_answers)
        audio_play(audio, SFX_UI_INVALID);
    else if (logic->confirmed_mask & ~before->logic_confirmed) audio_play(audio, SFX_UI_CONFIRM);
    else if (logic->selected != before->logic_selected || logic->focus != before->logic_focus ||
             logic->dropdown != before->logic_dropdown || logic->option != before->logic_option)
        audio_play(audio, SFX_UI_MOVE);
}

static void react_exploration(GameAudio *audio, const GameAudioSnapshot *before, const Game *game)
{
    const World *world = &game->world;
    if (world->location != before->location) {
        bool hatch = world->location == WORLD_BASEMENT || before->location == WORLD_BASEMENT;
        audio_play(audio, hatch ? SFX_HATCH : SFX_DOOR_OPEN);
        audio->step_distance = step_length;
    } else {
        react_steps(audio, before, game);
    }

    if (world->dialogue_title) {
        int shown = world->dialogue_visible_characters;
        bool new_page = !before->dialogue_open || world->dialogue_page != before->dialogue_page;
        if (new_page ? shown > 0
                     : shown / dialogue_blip_interval > before->dialogue_characters / dialogue_blip_interval)
            audio_play(audio, SFX_DIALOGUE_BLIP);
    }

    bool opened = (world->relay_panel_open && !before->relay_panel_open) ||
                  (world->logic_quiz_open && !before->logic_quiz_open);
    bool closed = (!world->relay_panel_open && before->relay_panel_open) ||
                  (!world->logic_quiz_open && before->logic_quiz_open);
    if (opened) audio_play(audio, SFX_UI_CONFIRM);
    else if (closed && !world->completed) audio_play(audio, SFX_UI_BACK);

    if (world->relay_rejected && !before->relay_rejected) audio_play(audio, SFX_UI_INVALID);
    else if (world->relay_mask != before->relay_mask || world_controls(world) != before->world_controls)
        audio_play(audio, SFX_RELAY_ON);
    else if (world->relay_step != before->relay_step)
        audio_play(audio, world->relay_step > before->relay_step ? SFX_UI_CONFIRM : SFX_UI_BACK);

    if (world->logic_quiz_open && before->logic_quiz_open) react_logic_setup(audio, before, game);
    if (world->completed && !before->completed) audio_play_delayed(audio, SFX_JINGLE_VICTORY, 0.3f);
}

static SoundId effect_sound(BattleFxKind kind)
{
    switch (kind) {
        case BATTLE_FX_LEAF: case BATTLE_FX_PETAL: return SFX_ATTACK_LEAF;
        case BATTLE_FX_THORN: return SFX_HIT_THORN;
        case BATTLE_FX_BITE: return SFX_ATTACK_BITE;
        case BATTLE_FX_WHIP: return SFX_ATTACK_WHIP;
        case BATTLE_FX_FIRE: return SFX_ATTACK_FIRE;
        case BATTLE_FX_SOLAR: return SFX_ATTACK_SOLAR;
        case BATTLE_FX_SPARK: return SFX_ATTACK_SPARK;
        case BATTLE_FX_BUFF: return SFX_BUFF;
        case BATTLE_FX_DEBUFF: return SFX_DEBUFF;
        default: return SFX_ATTACK_BODY;
    }
}

/* Visual e som usam o mesmo resultado válido e o mesmo instante de impacto. */
static bool react_move_effect(GameAudio *audio, const GameAudioSnapshot *before, const BattleScreen *screen)
{
    const BattleMoveFeedback *event = &screen->battle.feedback;
    if (before->feedback_sequence == event->sequence) return false;
    const Team *team = side_team(&screen->battle,event->side);
    if (event->plant_index < 0 || event->plant_index >= team->count) return false;
    const Plant *plant = &team->plants[event->plant_index];
    const Move *move = &plant->moves[event->move_index];
    BattleFxKind kind = battle_fx_kind(plant->id,event->move_index,move);
    audio_play(audio,effect_sound(kind));
    if (event->boosted) audio_play(audio,SFX_BOOSTED);
    if (!event->hit) audio_play_delayed(audio,SFX_MISS,BATTLE_FX_IMPACT_TIME);
    else if (event->damage > 0)
        audio_play_delayed(audio,event->critical ? SFX_CRITICAL : SFX_HIT_IMPACT,BATTLE_FX_IMPACT_TIME);
    return true;
}

/* Com o golpe tocando no mesmo quadro do dano, o impacto espera o avanço da planta. */
static void react_battle_result(GameAudio *audio, const GameAudioSnapshot *before,
                                const BattleState *battle, bool attack_now)
{
    float impact_delay = attack_now ? BATTLE_FX_IMPACT_TIME : 0.0f;
    bool damaged = false;
    bool fainted = false;
    bool healed = false;
    for (int side = 0; side < 2; side++) {
        const Team *team = side_team(battle, side);
        for (int i = 0; i < team->count && i < MAX_TEAM_SIZE; i++) {
            int hp = team->plants[i].current_hp;
            if (hp > before->hp[side][i]) healed = true;
            if (hp >= before->hp[side][i]) continue;
            damaged = true;
            if (hp <= 0) fainted = true;
        }
        int old = before->active_index[side];
        if (team->active_index != old && team->count == before->team_count[side]) {
            /* Troca voluntária: a planta sai antes da reserva entrar; após nocaute, só entra. */
            bool old_alive = old >= 0 && old < team->count && team->plants[old].current_hp > 0;
            if (old_alive) audio_play(audio, SFX_SWITCH_OUT);
            audio_play_delayed(audio, SFX_SWITCH_IN, old_alive ? 0.4f : 0.1f);
        }
    }
    if (damaged && (!attack_now || !battle->feedback.hit || battle->feedback.damage == 0))
        audio_play_delayed(audio,SFX_STATUS_TICK,impact_delay);
    if (healed) audio_play_delayed(audio,SFX_HEAL,impact_delay);
    if (fainted) audio_play_delayed(audio, SFX_FAINT, impact_delay + 0.35f);

    const Team *player = &battle->player_team;
    int active = player->active_index;
    if (active >= 0 && active < player->count && active == before->active_index[0]) {
        const Plant *plant = &player->plants[active];
        int limit = plant->max_hp / 4;
        if (plant->current_hp > 0 && plant->current_hp <= limit && before->hp[0][active] > limit)
            audio_play_delayed(audio, SFX_LOW_HP, impact_delay + 0.6f);
    }

    if (battle->state != before->battle_state) {
        if (battle->state == BATTLE_STATE_PLAYER_WIN) audio_play_delayed(audio, SFX_JINGLE_VICTORY, 0.9f);
        if (battle->state == BATTLE_STATE_ENEMY_WIN) audio_play_delayed(audio, SFX_JINGLE_DEFEAT, 0.9f);
    }
}

static void react_battle(GameAudio *audio, const GameAudioSnapshot *before, const BattleScreen *screen)
{
    if (before->setup_side >= 0) {
        if (screen->setup_side != before->setup_side ||
            screen->battle.player_team.count > before->team_count[0])
            audio_play(audio, SFX_UI_CONFIRM);
        else if (screen->roster_cursor != before->setup_cursor || screen->team_slot != before->setup_slot ||
                 screen->draft_plant_index != before->setup_plant ||
                 screen->draft_move_index != before->setup_move ||
                 screen->draft.left != before->setup_draft.left ||
                 screen->draft.right != before->setup_draft.right ||
                 screen->draft.operator != before->setup_draft.operator)
            audio_play(audio, SFX_UI_MOVE);
        return;
    }

    if (before->pending_action == BATTLE_PENDING_NONE && screen->pending_action != BATTLE_PENDING_NONE)
        if (screen->pending_action == BATTLE_PENDING_PARALYSIS) audio_play(audio,SFX_PARALYSIS);
    if (before->pending_action == BATTLE_PENDING_ROUND_RESULT &&
        (screen->pending_action != BATTLE_PENDING_ROUND_RESULT ||
         screen->round_message_index != before->round_message_index)) audio_play(audio,SFX_UI_CONFIRM);
    bool attack_now = react_move_effect(audio,before,screen);
    react_battle_result(audio, before, &screen->battle, attack_now);

    /* A resolução da ação também volta o menu à raiz; isso não é navegação. */
    if (before->pending_action != BATTLE_PENDING_NONE || screen->pending_action != BATTLE_PENDING_NONE) return;
    if (screen->menu != before->battle_menu)
        audio_play(audio, screen->menu == BATTLE_MENU_ROOT ? SFX_UI_BACK : SFX_UI_CONFIRM);
    else if (screen->menu_cursor != before->battle_cursor)
        audio_play(audio, SFX_UI_MOVE);
}

static MusicId screen_music(const Game *game)
{
    switch (game->screen) {
        case GAME_MENU:
        case GAME_LEADERBOARD:
        case GAME_TRAINER_SELECT:
        case GAME_NAME_ENTRY:
        case GAME_INTRO:
            return MUSIC_MENU;
        case GAME_ENDING: return MUSIC_ENDING;
        case GAME_DEFEAT: return MUSIC_NONE;
        case GAME_EXPLORATION:
            if (game->world.completed) return MUSIC_ENDING;
            return game->world.location == WORLD_BASEMENT ? MUSIC_BASEMENT : MUSIC_EXPLORATION;
        case GAME_BATTLE: {
            BattleStateEnum state = game->battle_screen.battle.state;
            /* A vinheta de resultado toca sozinha. */
            bool over = state == BATTLE_STATE_PLAYER_WIN || state == BATTLE_STATE_ENEMY_WIN;
            return over ? MUSIC_NONE : MUSIC_BATTLE;
        }
    }
    return MUSIC_NONE;
}

void game_audio_react(Game *game, const GameAudioSnapshot *before)
{
    GameAudio *audio = &game->audio;
    if (IsKeyPressed(KEY_F9)) audio_toggle_mute(audio);

    if (game->screen != before->screen) {
        react_screen_change(audio, before->screen, game->screen);
    } else {
        react_menus(audio, before, game);
        if ((game->screen == GAME_ENDING || game->screen == GAME_DEFEAT) &&
            game->result.cursor != before->result_cursor) audio_play(audio,SFX_UI_MOVE);
        if (game->screen == GAME_EXPLORATION) react_exploration(audio, before, game);
        if (game->screen == GAME_BATTLE) react_battle(audio, before, &game->battle_screen);
    }

    audio->effects_paused = game->exit_confirm_open;
    audio_set_music(audio, screen_music(game));
    audio_update(audio, GetFrameTime());
}
