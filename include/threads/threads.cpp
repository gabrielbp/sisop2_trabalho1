#include "threads.h"

T_ActiveThread activeThreads[MAX_ACTIVE_THREADS];
int isInitialized = 0;

void InitializeThreadArray();
int findThreadSlot();

/*
    Inicializa as estruturas de dados necessarias
*/
int InitializeThreads(){
    if(isInitialized == 1){
        printf("[InitializeThreads]Erro: ja foi inicializado");
    }

    InitializeThreadArray();
    return 0;
}

/*
    Cria uma nova thread com a rotina start_routine e o argumento em arg e a adiciona ao array de threads ativas
    Se executou com sucesso, retorna o índice da thread no array. Caso contrário, retorna -1
*/
int createNewThread(void * (*start_routine)(void *), void * arg){
    if(isInitialized == 0){
        if(InitializeThreads() == -1){
            return -1;
        }
    }

    T_ActiveThread newThread;
    int newThreadIndex;
    int status;

    newThreadIndex = findThreadSlot();
    if(newThreadIndex == -1){
        return -1;
    }
    
    status = pthread_create( &newThread.thread, NULL, start_routine, arg);

    if(status != 0){
        printf("[createNewThread]Erro na criacao da nova thread");
        return -1;
    }

    newThread.isValid = 1;
    activeThreads[newThreadIndex] = newThread;
    return newThreadIndex;
}

/*
    Cancela uma thread que esta rodando.
    Retorna 0 se executou com sucesso, -1 se houve um erro
*/
int killThread(int threadIndex){

    if(isInitialized == 0){
        printf("[killThread]Erro: Nao foi inicializado");
        return -1;
    }

    T_ActiveThread threadToBeKilled = activeThreads[threadIndex];

    if(threadToBeKilled.isValid == 0){
        printf("[killThread]Erro: thread invalida");
        return -1;
    }

    /*mata a thread*/
    int killStatus = pthread_kill(threadToBeKilled.thread, SIGKILL);

    if(killStatus == -1){
        printf("[killThread]Erro: nao foi possivel enviar sinal para thread");
        return -1;
    }

    activeThreads[threadIndex].isValid = 0;
    return 0;
}


/*
    Retorna o primeiro índice vago do array de threads ativas
    Se o array estiver cheio, retorna -1;
*/
int findThreadSlot(){
    int i = 0;
    for(i = 0; i<MAX_ACTIVE_THREADS; i++){
        if(activeThreads[i].isValid == 0){
            return i;
        }
    }
    printf("[findThreadSlot]Erro: numero maximo de threads simultaneas atingido");
        return -1;
}


/*
    Inicializa o array de threads ativas marcando todas como invalidas
*/
void InitializeThreadArray(){
    int i = 0;
    for(i = 0; i<MAX_ACTIVE_THREADS; i++){
        activeThreads[i].isValid = 0;
    }
}

