#ifndef FILEMONITORING
#define FILEMONITORING

#include <sys/inotify.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../threads/threads.h"

#define MAX_EVENT_MONITOR 2048
#define NAME_LENGTH 32 //size of file name
#define MONITOR_EVENT_SIZE (sizeof(struct inotify_event)) //size of 1 event
#define BUFFER_LEN MAX_EVENT_MONITOR * (MONITOR_EVENT_SIZE + NAME_LENGTH) //buffer max length
#define MAX_ACTIVE_MONITORS 256 //número máximo de monitores simultâneos


/*
    Inicializa o INotify.
    Retorna 0 se executou com sucesso e -1 se ocorreu um erro
*/
int Initialize_INotify();

/*
    Encerra o INotify.
    Retorna 0 se executou com sucesso e -1 se ocorreu um erro
*/
int Close_INotify();

/*
    Adiciona um monitor do INotify no diretório especificado por directory_path e cria uma nova thread na qual o monitor ficará rodando lendo eventos.
    Quando um evento é detectado, é chamada uma das 3 funções callback especificadas pelo parâmetros fileCreateCallback, fileModifyCallback e fileDeleteCallback, respectivamente.
    Retorna 0 se executou com sucesso e -1 se ocorreu algum erro.
*/
int AddMonitor(char* directory_path, void* (*fileCreateCallback)(struct inotify_event*), void* (*fileModifyCallback)(struct inotify_event*), void* (*fileDeleteCallback)(struct inotify_event*));


/*
    Fecha o monitor especificado pelo parâmetro monitorDescriptor e mata a thread correspondente
    Retorna 0 se executou com sucesso e -1 se ocorreu algum erro.
*/
int RemoveMonitor(int monitorDescriptor);


/*
    Implementa o funcionamento principal de um monitor: um loop infinito que detecta modificações em algum diretório
*/
void* MonitorMainLoop(void* monitor);

#endif