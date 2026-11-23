/* Efeitos em pixels com trajetórias determinísticas: não usam o RNG do combate. */
#include "battle_fx.h"
#include "plant_data.h"
#include <math.h>

static const Vector2 centers[2] = {{454,320},{888,195}};
static float clamp01(float x) { return fminf(1,fmaxf(0,x)); }
static float grid(float x) { return roundf(x/3)*3; }
static Vector2 lerp(Vector2 a, Vector2 b, float t)
{ return (Vector2){a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t}; }

BattleFxKind battle_fx_kind(int species, int move_index, const Move *move)
{
    if (move->effect == MOVE_EFFECT_ATTRIBUTE)
        return move->target == MOVE_TARGET_SELF ? BATTLE_FX_BUFF : BATTLE_FX_DEBUFF;
    if (species == SPECIES_CARNIVOROUS_PLANT) return BATTLE_FX_BITE;
    if (species == SPECIES_POISON_IVY) return BATTLE_FX_WHIP;
    if (species == SPECIES_PEPPER && move_index == 2) return BATTLE_FX_FIRE;
    if (species == SPECIES_SUNFLOWER && move_index == 0) return BATTLE_FX_SOLAR;
    if (species == SPECIES_ORCHID) return move_index == 0 ? BATTLE_FX_PETAL : BATTLE_FX_WHIP;
    if (species == SPECIES_CLOVER) return move_index == 0 ? BATTLE_FX_LEAF : BATTLE_FX_SPARK;
    if (move->category == MOVE_CATEGORY_PHYSICAL) return BATTLE_FX_BODY;
    return BATTLE_FX_THORN;
}

void battle_fx_begin(BattleFx *fx, const BattleState *before, const BattleState *after)
{
    if (after->feedback.sequence == before->feedback.sequence) return;
    BattleMoveFeedback event = after->feedback;
    const Team *acting = event.side == 0 ? &before->player_team : &before->enemy_team;
    if (event.side < 0 || event.side > 1 || event.plant_index < 0 || event.plant_index >= acting->count ||
        event.move_index < 0 || event.move_index >= MAX_MOVES) return;
    const Plant *plant = &acting->plants[event.plant_index];
    const Move *move = &plant->moves[event.move_index];
    *fx = (BattleFx){.active=true,.move=event,.category=move->category,
                   .kind=battle_fx_kind(plant->id,event.move_index,move)};
    for (int side = 0; side < 2; side++) {
        const Team *old = side == 0 ? &before->player_team : &before->enemy_team;
        const Team *now = side == 0 ? &after->player_team : &after->enemy_team;
        for (int i = 0; i < old->count && i < MAX_TEAM_SIZE; i++) {
            fx->hp_before[side][i] = old->plants[i].current_hp;
            fx->hp_after[side][i] = i < now->count ? now->plants[i].current_hp : 0;
        }
    }
}

void battle_fx_update(BattleFx *fx, float dt)
{
    if (!fx->active) return;
    fx->elapsed += fminf(0.1f,fmaxf(0,dt));
    if (fx->elapsed >= BATTLE_FX_DURATION) fx->active = false;
}

float battle_fx_hp(const BattleFx *fx, int side, int index, int actual)
{
    if (!fx->active || side < 0 || side > 1 || index < 0 || index >= MAX_TEAM_SIZE) return (float)actual;
    float t = clamp01((fx->elapsed-BATTLE_FX_IMPACT_TIME)/0.45f);
    t = t*t*(3-2*t);
    return fx->hp_before[side][index]+(fx->hp_after[side][index]-fx->hp_before[side][index])*t;
}

Rectangle battle_fx_plant_bounds(const BattleFx *fx, int side, Rectangle bounds)
{
    if (!fx->active) return bounds;
    float time = fx->elapsed;
    if (fx->move.side == side && time < 0.62f) {
        float wave = sinf(clamp01(time/0.62f)*PI);
        if (fx->category == MOVE_CATEGORY_PHYSICAL) bounds.x += grid((side == 0 ? 54 : -54)*wave);
        else if (fx->category == MOVE_CATEGORY_STATUS) bounds.y -= grid(18*wave);
    }
    if (fx->move.target_side == side && fx->move.hit && fx->move.damage > 0 && time >= BATTLE_FX_IMPACT_TIME) {
        float t = clamp01((time-BATTLE_FX_IMPACT_TIME)/0.28f);
        bounds.x += grid((side == 0 ? -1 : 1)*sinf(t*PI)*(fx->move.critical ? 20 : 12));
        if (fx->move.critical && t < 1) bounds.y += grid(sinf(t*PI*6)*(1-t)*5);
    }
    return bounds;
}

Color battle_fx_tint(const BattleFx *fx, int side, Color base)
{
    if (fx->active && fx->move.target_side == side && fx->move.hit && fx->move.damage > 0 &&
        fx->elapsed >= BATTLE_FX_IMPACT_TIME && fx->elapsed < BATTLE_FX_IMPACT_TIME+0.22f &&
        ((int)((fx->elapsed-BATTLE_FX_IMPACT_TIME)*28)%2) == 0)
        return (Color){255,155,145,255};
    return base;
}

static void pixel(Vector2 p, int size, Color color)
{ DrawRectangle((int)grid(p.x),(int)grid(p.y),size,size,color); }
static void burst(Vector2 center, float progress, Color color, int count)
{
    for (int i = 0; i < count; i++) {
        float a = i*2*PI/count;
        Vector2 p = {center.x+cosf(a)*(12+progress*70),center.y+sinf(a)*(12+progress*50)};
        pixel(p,progress < 0.5f ? 9 : 6,Fade(color,1-progress));
    }
}

void battle_fx_draw(const BattleFx *fx, const BattleState *battle)
{
    if (!fx->active) return;
    Vector2 from = centers[fx->move.side], target = centers[fx->move.target_side];
    float travel = clamp01((fx->elapsed-0.10f)/(BATTLE_FX_IMPACT_TIME-0.10f));
    float impact = clamp01((fx->elapsed-BATTLE_FX_IMPACT_TIME)/(BATTLE_FX_DURATION-BATTLE_FX_IMPACT_TIME));
    Color color = fx->kind == BATTLE_FX_FIRE ? ORANGE : fx->kind == BATTLE_FX_SOLAR ? GOLD :
        fx->kind == BATTLE_FX_PETAL ? PINK : fx->kind == BATTLE_FX_SPARK ? SKYBLUE : (Color){125,185,65,255};
    if (fx->move.boosted && fx->elapsed < BATTLE_FX_IMPACT_TIME)
        burst(from,clamp01(fx->elapsed/BATTLE_FX_IMPACT_TIME),GOLD,8);
    if (fx->elapsed >= 0.10f && fx->elapsed < BATTLE_FX_IMPACT_TIME) {
        if (fx->kind == BATTLE_FX_SOLAR) {
            Vector2 tip = lerp(from,target,travel);
            DrawLineEx(from,tip,15,(Color){235,172,49,255});
            DrawLineEx(from,tip,6,(Color){255,243,169,255});
        } else if (fx->kind == BATTLE_FX_WHIP) {
            Vector2 last = from;
            for (int i = 1; i <= 12; i++) {
                float t = travel*i/12;
                Vector2 p = lerp(from,target,t); p.y += grid(sinf(t*PI*4)*16*(1-t));
                DrawLineEx(last,p,6,color); last = p;
            }
        } else if (fx->category == MOVE_CATEGORY_RANGED) {
            for (int i = 0; i < 3; i++) {
                Vector2 p = lerp(from,target,clamp01(travel-i*0.09f));
                p.y += (i-1)*12;
                if (fx->kind == BATTLE_FX_THORN)
                    DrawTriangle((Vector2){grid(p.x+(fx->move.side == 0 ? 15 : -15)),grid(p.y)},
                        (Vector2){grid(p.x),grid(p.y-6)},(Vector2){grid(p.x),grid(p.y+6)},color);
                else if (fx->kind == BATTLE_FX_FIRE) {
                    pixel(p,15,RED); pixel((Vector2){p.x+3,p.y+3},9,YELLOW);
                } else {
                    pixel(p,12,color); pixel((Vector2){p.x+6,p.y-6},6,color);
                }
            }
        }
    }
    if (fx->elapsed >= BATTLE_FX_IMPACT_TIME) {
        if (!fx->move.hit) {
            burst((Vector2){target.x,target.y-70},impact,(Color){200,210,190,255},5);
        } else if (fx->kind == BATTLE_FX_BUFF || fx->kind == BATTLE_FX_DEBUFF) {
            Color tint = fx->kind == BATTLE_FX_BUFF ? LIME : (Color){175,105,203,255};
            for (int i = 0; i < 3; i++) {
                float x = target.x+(i-1)*30, y = target.y-20+(fx->kind == BATTLE_FX_BUFF ? -1 : 1)*impact*65;
                int direction = fx->kind == BATTLE_FX_BUFF ? -1 : 1;
                pixel((Vector2){x,y},6,Fade(tint,1-impact));
                pixel((Vector2){x-6,y-direction*6},6,Fade(tint,1-impact));
                pixel((Vector2){x+6,y-direction*6},6,Fade(tint,1-impact));
            }
        } else {
            if (fx->kind == BATTLE_FX_BITE && impact < 0.55f) {
                for (int i = 0; i < 5; i++) {
                    float gap = 28*(1-clamp01(impact*3));
                    pixel((Vector2){target.x-35+i*16,target.y-12-gap},12,(Color){255,240,195,255});
                    pixel((Vector2){target.x-35+i*16,target.y+12+gap},12,(Color){255,240,195,255});
                }
            }
            burst(target,impact,fx->move.critical ? GOLD : color,fx->move.critical ? 14 : 8);
        }
        /* Recuperação e dano passivo também recebem partículas discretas. */
        for (int side = 0; side < 2; side++) {
            const Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
            int index = team->active_index;
            if (index < 0 || index >= team->count) continue;
            int delta = fx->hp_after[side][index]-fx->hp_before[side][index];
            if (delta > 0) burst(centers[side],impact,(Color){130,225,145,255},5);
            else if (delta < 0 && (side != fx->move.target_side || !fx->move.hit || fx->move.damage == 0))
                burst(centers[side],impact,(Color){170,100,200,255},5);
        }
    }
}
