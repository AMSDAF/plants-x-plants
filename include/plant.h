/*
 * plant.h
 * Define as estruturas de golpes, plantas e equipes.
 * Compartilhado pelas regras de combate e pela interface.
 * Não depende da raylib.
 */
#ifndef PLANT_H
#define PLANT_H

#define MAX_MOVES 3
#define MAX_TEAM_SIZE 4
#define MIN_TEAM_SIZE 2

typedef enum {
    PLANT_STATUS_POISON,
    PLANT_STATUS_BURN,
    PLANT_STATUS_LUCK,
    PLANT_STATUS_PARALYZED,
    PLANT_STATUS_HOST,
    PLANT_STATUS_SPEED_BOOST,
    PLANT_STATUS_ATTACK_BOOST,
    PLANT_STATUS_DEFENSE_BOOST,
    PLANT_STATUS_ACCURACY_BOOST,
    PLANT_STATUS_ATTACK_DOWN,
    PLANT_STATUS_DEFENSE_DOWN,
    PLANT_STATUS_SPEED_DOWN,
    PLANT_STATUS_COUNT
} PlantStatusType;

#define PLANT_STATUS_LUCK_CRITICAL_MULTIPLIER 2.0f
#define PLANT_STATUS_DEFAULT_BUFF_MULTIPLIER 1.5f

typedef struct {
    PlantStatusType type;
    int remaining_turns; /* Zero indica que o efeito não expira por turnos. */
    float value;         /* Multiplicador ou potência interpretada pelo efeito. */
    int source_side;      /* -1 quando o efeito não tem uma planta aplicadora. */
    int source_plant_index;
} PlantStatus;

// Estrutura de um golpe
typedef enum {
    MOVE_CATEGORY_PHYSICAL,
    MOVE_CATEGORY_RANGED,
    MOVE_CATEGORY_STATUS
} MoveCategory;

typedef enum {
    MOVE_EFFECT_DAMAGE,
    MOVE_EFFECT_ATTRIBUTE
} MoveEffect;

typedef enum {
    MOVE_STAT_ATTACK,
    MOVE_STAT_DEFENSE,
    MOVE_STAT_SPEED,
    MOVE_STAT_COUNT
} MoveStat;

typedef enum {
    MOVE_TARGET_SELF,
    MOVE_TARGET_OPPONENT
} MoveTarget;

typedef enum {
    MOVE_STAT_CHANGE_ADD_TO_CAP,
    MOVE_STAT_CHANGE_RATIO_DEBUFF
} MoveStatChangeMode;

typedef struct {
    char name[32];
    MoveCategory category;
    MoveEffect effect;
    MoveTarget target;
    MoveStat stat;
    MoveStatChangeMode stat_change_mode;
    int power;
    int accuracy;
    int critical_chance;      // Chance de crítico em porcentagem.
    int priority;              // Prioridade de ação; valores maiores agem antes.
    float effect_step;
    float effect_cap;
} Move;

// Estrutura de uma planta individual (instância em batalha)
typedef struct {
    int id;                  // Identificador da espécie
    char name_pt[32];        // Nome da planta em português
    
    // Atributos de Combate
    int max_hp;
    int current_hp;
    int attack;
    int defense;
    int speed;
    int base_attack;
    int base_defense;
    int base_speed;
    int stat_debuff_amount[MOVE_STAT_COUNT];
    
    Move moves[MAX_MOVES];   // Golpes que esta planta conhece
    PlantStatus statuses[PLANT_STATUS_COUNT];
    int status_count;
    int ability_turn_count;
    int fruit_count;
} Plant;

// Estrutura de uma equipe de plantas
typedef struct {
    Plant plants[MAX_TEAM_SIZE];
    int count;               // Quantas plantas tem na equipe atualmente
    int active_index;        // Índice da planta que está lutando agora
} Team;

// --- ADICIONE ESTAS DECLARAÇÕES AQUI ---
Plant* Team_GetActivePlant(Team *team);
int Team_HasLivingPlants(const Team *team);
int Team_SelectSpecies(Team *team, int index, int species_id);
int Team_RemoveReserve(Team *team, int index);
void Team_Restore(Team *team);

// Funções utilitárias da planta e da equipe
int Plant_IsAlive(const Plant *plant);
void Plant_TakeDamage(Plant *plant, int damage);
int Plant_AddStatus(Plant *plant, PlantStatusType type, int remaining_turns, float value);
/* Registra a planta aplicadora, usada pelo combate para encerrar Hospedeiro. */
int Plant_AddStatusFrom(Plant *plant, PlantStatusType type, int remaining_turns,
                        float value, int source_side, int source_plant_index);
int Plant_HasStatus(const Plant *plant, PlantStatusType type);
int Plant_GetStatusValue(const Plant *plant, PlantStatusType type, float *value);
int Plant_GetStatusSource(const Plant *plant, PlantStatusType type,
                          int *source_side, int *source_plant_index);
int Plant_RemoveStatus(Plant *plant, PlantStatusType type);
void Plant_TickStatuses(Plant *plant);
void Plant_ClearStatuses(Plant *plant);

Plant Plant_CreateFromSpecies(int species_id);

#endif
