/* Carrega os arquivos de som, toca efeitos e faz a transição suave entre músicas. */
#include "audio.h"

#include <stddef.h>
#include <math.h>
#include <stdint.h>

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

/* V0 sintetizada uma vez no carregamento; ruído local não altera rand() do combate. */
static Sound synth_effect(SoundId id)
{
    const int rate = 22050;
    const float duration = id == SFX_ATTACK_SOLAR ? 0.38f : id == SFX_ATTACK_FIRE ? 0.30f : 0.20f;
    int frames = (int)(duration*rate);
    short *samples = MemAlloc((unsigned int)frames*sizeof(short));
    if (!samples) return (Sound){0};
    uint32_t noise = 2654435761u+(uint32_t)id;
    float phase = 0;
    for (int i = 0; i < frames; i++) {
        float t = (float)i/frames, time = (float)i/rate;
        noise = noise*1664525u+1013904223u;
        float n = ((noise >> 8)&65535)/32767.5f-1;
        float frequency = 420, mix = 0.25f;
        switch (id) {
            case SFX_ATTACK_LEAF: frequency = 600-320*t; mix = 0.70f; break;
            case SFX_ATTACK_WHIP: frequency = 180+1500*t*t; mix = 0.65f; break;
            case SFX_ATTACK_FIRE: frequency = 100+70*sinf(time*70); mix = 0.85f; break;
            case SFX_ATTACK_SOLAR: frequency = 260+800*t; mix = 0.10f; break;
            case SFX_ATTACK_BODY: frequency = 230-160*t; mix = 0.55f; break;
            case SFX_ATTACK_BITE: frequency = 140-80*t; mix = 0.60f; break;
            case SFX_ATTACK_SPARK: frequency = 700+350*sinf(time*130); mix = 0.45f; break;
            case SFX_BUFF: case SFX_HEAL: frequency = 350+650*t; mix = 0.03f; break;
            case SFX_DEBUFF: frequency = 750-550*t; mix = 0.15f; break;
            case SFX_MISS: frequency = 450-300*t; mix = 0.85f; break;
            case SFX_CRITICAL: frequency = 180-120*t; mix = 0.75f; break;
            case SFX_STATUS_TICK: frequency = 150+60*sinf(time*50); mix = 0.40f; break;
            default: break;
        }
        phase += 2*PI*frequency/rate;
        float envelope = fminf(1,time/0.008f)*powf(1-t,2);
        float tone = sinf(phase)+0.25f*sinf(phase*2);
        samples[i] = (short)(12000*envelope*((1-mix)*tone+mix*n));
    }
    Wave wave = {.frameCount=(unsigned int)frames,.sampleRate=rate,.sampleSize=16,.channels=1,.data=samples};
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

void audio_init(GameAudio *audio)
{
    *audio = (GameAudio){.current_music = MUSIC_NONE, .target_music = MUSIC_NONE};
    /* Alvos de verificação sem dispositivo de áudio seguem sem som. */
    if (!IsAudioDeviceReady()) return;
    audio->ready = true;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (i >= SFX_ATTACK_LEAF) {
            audio->sounds[i] = synth_effect((SoundId)i);
            audio->sound_loaded[i] = IsSoundValid(audio->sounds[i]);
            if (audio->sound_loaded[i]) SetSoundVolume(audio->sounds[i],sfx_volume*0.75f);
            continue;
        }
        if (!sound_paths[i] || !FileExists(sound_paths[i])) {
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
    float volume = audio->muted ? 0.0f : music_volume * audio->music_fade * (audio->impact_duck > 0 ? 0.65f : 1);
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
    if (!audio->effects_paused) audio->impact_duck = fmaxf(0,audio->impact_duck-dt);
    for (int i = 0; i < AUDIO_DELAYED_CAPACITY; i++) {
        if (audio->effects_paused) break;
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
    if (id >= SFX_ATTACK_LEAF || id == SFX_HIT_IMPACT || id == SFX_HIT_THORN) {
        /* Variação determinística pequena, independente da sorte dos ataques. */
        float pitch = 0.97f+0.015f*(audio->variation_counter++%5);
        SetSoundPitch(audio->sounds[id],pitch);
        SetSoundVolume(audio->sounds[id],sfx_volume*(id >= SFX_ATTACK_LEAF ? 0.75f : 1));
    }
    if (id == SFX_HIT_IMPACT || id == SFX_CRITICAL) audio->impact_duck = 0.28f;
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
