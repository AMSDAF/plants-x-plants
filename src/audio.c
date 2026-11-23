/* Carrega os arquivos de som, toca efeitos e faz a transição suave entre músicas. */
#include "audio.h"

#include <stddef.h>

/* Os níveis relativos já vêm equilibrados nos arquivos de efeito. */
static const float sfx_volume = 0.9f;
/* A música fica abaixo dos efeitos para não encobrir golpes e diálogos. */
static const float music_volume = 0.45f;
static const float music_fade_seconds = 0.8f;

static const char *const sound_paths[SFX_COUNT] = {
    [SFX_UI_MOVE] = "assets/sounds/sfx/ui_move.wav",
    [SFX_UI_CONFIRM] = "assets/sounds/sfx/ui_confirm.wav",
    [SFX_UI_BACK] = "assets/sounds/sfx/ui_back.wav",
    [SFX_UI_INVALID] = "assets/sounds/sfx/ui_invalid.wav",
    [SFX_UI_TYPE] = "assets/sounds/sfx/ui_type.wav",
    [SFX_STEP_GRASS] = "assets/sounds/sfx/step_grass.wav",
    [SFX_DOOR_OPEN] = "assets/sounds/sfx/door_open.wav",
    [SFX_HATCH] = "assets/sounds/sfx/hatch.wav",
    [SFX_DIALOGUE_BLIP] = "assets/sounds/sfx/dialogue_blip.wav",
    [SFX_RELAY_ON] = "assets/sounds/sfx/relay_on.wav",
    [SFX_BATTLE_START] = "assets/sounds/sfx/battle_start.wav",
    [SFX_HIT_THORN] = "assets/sounds/sfx/hit_thorn.wav",
    [SFX_HIT_LIGHT] = "assets/sounds/sfx/hit_light.wav",
    [SFX_HIT_IMPACT] = "assets/sounds/sfx/hit_impact.wav",
    [SFX_BOOSTED] = "assets/sounds/sfx/boosted.wav",
    [SFX_SWITCH_OUT] = "assets/sounds/sfx/switch_out.wav",
    [SFX_SWITCH_IN] = "assets/sounds/sfx/switch_in.wav",
    [SFX_FAINT] = "assets/sounds/sfx/faint.wav",
    [SFX_PARALYSIS] = "assets/sounds/sfx/paralysis.wav",
    [SFX_LOW_HP] = "assets/sounds/sfx/low_hp.wav",
    [SFX_JINGLE_VICTORY] = "assets/sounds/sfx/jingle_victory.wav",
    [SFX_JINGLE_DEFEAT] = "assets/sounds/sfx/jingle_defeat.wav",
};

static const char *const music_paths[MUSIC_COUNT] = {
    [MUSIC_MENU] = "assets/sounds/music/menu.ogg",
    [MUSIC_EXPLORATION] = "assets/sounds/music/exploration.ogg",
    [MUSIC_BASEMENT] = "assets/sounds/music/basement.ogg",
    [MUSIC_BATTLE] = "assets/sounds/music/battle.ogg",
    [MUSIC_ENDING] = "assets/sounds/music/ending.ogg",
};

void audio_init(GameAudio *audio)
{
    *audio = (GameAudio){.current_music = MUSIC_NONE, .target_music = MUSIC_NONE};
    /* Alvos de verificação sem dispositivo de áudio seguem sem som. */
    if (!IsAudioDeviceReady()) return;
    audio->ready = true;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (!FileExists(sound_paths[i])) {
            TraceLog(LOG_WARNING, "Som ausente: %s", sound_paths[i]);
            continue;
        }
        audio->sounds[i] = LoadSound(sound_paths[i]);
        audio->sound_loaded[i] = IsSoundValid(audio->sounds[i]);
        if (audio->sound_loaded[i]) SetSoundVolume(audio->sounds[i], sfx_volume);
    }
    for (int i = 0; i < MUSIC_COUNT; i++) {
        if (!FileExists(music_paths[i])) {
            TraceLog(LOG_WARNING, "Musica ausente: %s", music_paths[i]);
            continue;
        }
        audio->music[i] = LoadMusicStream(music_paths[i]);
        audio->music_loaded[i] = IsMusicValid(audio->music[i]);
        if (audio->music_loaded[i]) audio->music[i].looping = true;
    }
}

void audio_shutdown(GameAudio *audio)
{
    if (!audio->ready) return;
    for (int i = 0; i < SFX_COUNT; i++)
        if (audio->sound_loaded[i]) UnloadSound(audio->sounds[i]);
    for (int i = 0; i < MUSIC_COUNT; i++) {
        if (!audio->music_loaded[i]) continue;
        StopMusicStream(audio->music[i]);
        UnloadMusicStream(audio->music[i]);
    }
    *audio = (GameAudio){.current_music = MUSIC_NONE, .target_music = MUSIC_NONE};
}

static bool music_available(const GameAudio *audio, MusicId id)
{
    return id > MUSIC_NONE && id < MUSIC_COUNT && audio->music_loaded[id];
}

static void apply_music_volume(GameAudio *audio)
{
    if (!music_available(audio, audio->current_music)) return;
    float volume = audio->muted ? 0.0f : music_volume * audio->music_fade;
    SetMusicVolume(audio->music[audio->current_music], volume);
}

/* Diminui a faixa atual até o silêncio e então inicia a faixa pedida. */
static void update_music(GameAudio *audio, float dt)
{
    float step = dt / music_fade_seconds;
    if (audio->current_music != audio->target_music) {
        audio->music_fade -= step;
        if (audio->music_fade > 0 && music_available(audio, audio->current_music)) {
            apply_music_volume(audio);
        } else {
            if (music_available(audio, audio->current_music))
                StopMusicStream(audio->music[audio->current_music]);
            audio->current_music = audio->target_music;
            audio->music_fade = 0;
            if (music_available(audio, audio->current_music))
                PlayMusicStream(audio->music[audio->current_music]);
        }
    } else if (audio->music_fade < 1) {
        audio->music_fade += step;
        if (audio->music_fade > 1) audio->music_fade = 1;
    }
    apply_music_volume(audio);
    if (music_available(audio, audio->current_music))
        UpdateMusicStream(audio->music[audio->current_music]);
}

void audio_update(GameAudio *audio, float dt)
{
    if (!audio->ready) return;
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    for (int i = 0; i < AUDIO_DELAYED_CAPACITY; i++) {
        DelayedSound *pending = &audio->delayed[i];
        if (!pending->active) continue;
        pending->remaining -= dt;
        if (pending->remaining <= 0) {
            pending->active = false;
            audio_play(audio, pending->id);
        }
    }
    update_music(audio, dt);
}

void audio_play(GameAudio *audio, SoundId id)
{
    if (!audio->ready || audio->muted || id < 0 || id >= SFX_COUNT || !audio->sound_loaded[id]) return;
    PlaySound(audio->sounds[id]);
}

void audio_play_delayed(GameAudio *audio, SoundId id, float delay)
{
    if (!audio->ready) return;
    for (int i = 0; i < AUDIO_DELAYED_CAPACITY; i++) {
        if (audio->delayed[i].active) continue;
        audio->delayed[i] = (DelayedSound){id, delay, true};
        return;
    }
    /* Fila cheia: toca agora para não perder o retorno sonoro. */
    audio_play(audio, id);
}

void audio_set_music(GameAudio *audio, MusicId id)
{
    audio->target_music = id;
}

void audio_toggle_mute(GameAudio *audio)
{
    audio->muted = !audio->muted;
    if (audio->muted)
        for (int i = 0; i < SFX_COUNT; i++)
            if (audio->sound_loaded[i]) StopSound(audio->sounds[i]);
    apply_music_volume(audio);
}
