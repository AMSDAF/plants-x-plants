/* Armazena e sorteia perguntas de lógica sem depender da interface ou da campanha. */
#include "logic_quiz.h"
#include <stddef.h>
#include <stdlib.h>

static const LogicQuizQuestion questions[LOGIC_QUIZ_QUESTION_COUNT] = {
    {true, true, false, "P OU Q", true},
    {true, true, false, "P E Q", false},
    {false, false, false, "NÃO P", true},
    {true, true, true, "P E Q", true},
    {false, true, false, "P OU Q", false},
    {false, true, true, "P XOR Q", true},
    {true, true, true, "P XOR Q", false},
    {true, true, false, "P E NÃO Q", true},
    {false, true, true, "P OU NÃO Q", false},
    {false, true, false, "NÃO P E NÃO Q", true},
    {true, true, false, "NÃO P OU Q", false},
    {false, false, false, "NÃO P", true},
    {true, false, false, "NÃO P", false}
};

const LogicQuizQuestion *logic_quiz_question(int index)
{
    if (index < 0 || index >= LOGIC_QUIZ_QUESTION_COUNT) return NULL;
    return &questions[index];
}

int logic_quiz_pick(unsigned int *seen_mask)
{
    if (!seen_mask) return -1;
    unsigned int all = (1u << LOGIC_QUIZ_QUESTION_COUNT) - 1u;
    if ((*seen_mask & all) == all) *seen_mask = 0;
    int available[LOGIC_QUIZ_QUESTION_COUNT], count = 0;
    for (int i = 0; i < LOGIC_QUIZ_QUESTION_COUNT; i++)
        if (!(*seen_mask & (1u << i))) available[count++] = i;
    int choice = available[rand() % count];
    *seen_mask |= 1u << choice;
    return choice;
}
