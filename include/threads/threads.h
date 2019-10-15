#ifndef THREADS
#define THREADS

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define MAX_ACTIVE_THREADS 256

typedef struct activeThread{
    int isValid;
    pthread_t thread;
} T_ActiveThread;

/*
    Cria uma nova thread com a rotina start_routine e o argumento em arg e a adiciona ao array de threads ativas
    Se executou com sucesso, retorna o índice da thread no array. Caso contrário, retorna -1
*/
int createNewThread(void * (*start_routine)(void *), void * arg);

/*
    Cancela uma thread que esta rodando.
    Retorna 0 se executou com sucesso, -1 se houve um erro
*/
int killThread(int threadIndex);

#endif