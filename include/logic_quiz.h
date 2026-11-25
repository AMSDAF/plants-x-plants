/* Compartilha as perguntas de lógica entre os golpes e a revanche da campanha. */
#ifndef LOGIC_QUIZ_H
#define LOGIC_QUIZ_H
#include <stdbool.h>

enum { LOGIC_QUIZ_QUESTION_COUNT = 13 };
typedef struct {
    bool p;
    bool has_q;
    bool q;
    const char *expression;
    bool answer;
} LogicQuizQuestion;

const LogicQuizQuestion *logic_quiz_question(int index);
int logic_quiz_pick(unsigned int *seen_mask);
#endif
