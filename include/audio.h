/*
 * audio.h
 * Carrega, toca e encerra efeitos sonoros e músicas do jogo.
 * Não conhece regras de combate nem telas; recebe apenas pedidos de reprodução.
 */
#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include "raylib.h"

typedef enum {
    SFX_UI_MOVE,
    SFX_UI_CONFIRM,
    SFX_UI_BACK,
    SFX_UI_INVALID,
    SFX_UI_TYPE,
    SFX_STEP_GRASS,
    SFX_DOOR_OPEN,
    SFX_HATCH,
    SFX_DIALOGUE_BLIP,
    SFX_RELAY_ON,
    SFX_BATTLE_START,
    SFX_HIT_THORN,
    SFX_HIT_LIGHT,
    SFX_HIT_IMPACT,
    SFX_BOOSTED,
    SFX_SWITCH_OUT,
    SFX_SWITCH_IN,
    SFX_FAINT,
    SFX_PARALYSIS,
    SFX_LOW_HP,
    SFX_JINGLE_VICTORY,
    SFX_JINGLE_DEFEAT,
    SFX_COUNT
} SoundId;

typedef enum {
    MUSIC_NONE = -1,
    MUSIC_MENU,
    MUSIC_EXPLORATION,
    MUSIC_BASEMENT,
    MUSIC_BATTLE,
    MUSIC_ENDING,
    MUSIC_COUNT
} MusicId;

#define AUDIO_DELAYED_CAPACITY 8

typedef struct {
    SoundId id;
    float remaining;
    bool active;
} DelayedSound;

/* Recursos ausentes ficam desativados; o jogo continua em silêncio. */
typedef struct {
    bool ready;
    bool muted;
    Sound sounds[SFX_COUNT];
    bool sound_loaded[SFX_COUNT];
    Music music[MUSIC_COUNT];
    bool music_loaded[MUSIC_COUNT];
    MusicId current_music;
    MusicId target_music;
    float music_fade; /* 0 silêncio, 1 volume pleno da faixa atual. */
    DelayedSound delayed[AUDIO_DELAYED_CAPACITY];
    float step_distance; /* Distância percorrida desde o último passo audível. */
} GameAudio;

void audio_init(GameAudio *audio);
void audio_shutdown(GameAudio *audio);
void audio_update(GameAudio *audio, float dt);
void audio_play(GameAudio *audio, SoundId id);
void audio_play_delayed(GameAudio *audio, SoundId id, float delay);
void audio_set_music(GameAudio *audio, MusicId id);
void audio_toggle_mute(GameAudio *audio);

#endif /* AUDIO_H */
