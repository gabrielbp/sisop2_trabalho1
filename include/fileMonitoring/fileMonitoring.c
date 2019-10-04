#include "fileMonitoring.h"

#define MAX_EVENT_MONITOR 2048
#define NAME_LENGTH 32 //size of file name
#define MONITOR_EVENT_SIZE (sizeof(struct inotify_event)) //size of 1 event
#define BUFFER_LEN MAX_EVENT_MONITOR * (MONITOR_EVENT_SIZE + NAME_LENGTH) //buffer max length

typedef struct monitorCallbacks{
    void* (*fileCreateCallback)(struct inotify_event*);
    void* (*fileModifyCallback)(struct inotify_event*);
    void* (*fileDeleteCallback)(struct inotify_event*);
} T_MonitorCallbacks;

typedef struct activeMonitor{
    int isValid;
    int threadIndex;
    int monitorDescriptor;
    T_MonitorCallbacks callbacks;
} T_ActiveMonitor;

int InotifyInstance = -1;
int isInotifyInitialized = 0;
T_ActiveMonitor activeMonitors[MAX_ACTIVE_MONITORS];

void InitializeMonitorArray();
int findMonitorSlot();

/*
    Inicializa o INotify.
    Retorna 0 se executou com sucesso e -1 se ocorreu um erro
*/
int Initialize_INotify(){
    if(isInotifyInitialized == 0){
        int init = inotify_init();
        if(init < 0){
            printf("[Initialize_INotify] Erro ao inicializar INotify");
            return -1;
        }
        else {

            InitializeMonitorArray();

            InotifyInstance = init;
            isInotifyInitialized = 1;
            return 0;
        }
    } else {
        printf("[Initialize_INotify] Erro: INotify ja foi inicializado");
        return -1;
    }
}

/*
    Remove todos os monitores ativos e encerra o INotify.
    Retorna 0 se executou com sucesso e -1 se ocorreu um erro
*/
int Close_INotify(){
    if(isInotifyInitialized != 0){
        int i = 0;
        int monitorRemoveStatus;
        for(i = 0; i<MAX_ACTIVE_MONITORS; i++){
            if(activeMonitors[i].isValid == 1){
                monitorRemoveStatus = RemoveMonitor(i);
                if(monitorRemoveStatus == -1){
                    return -1;
                }
            }
        }

        close(InotifyInstance);
        return 0;
    } else {
        printf("[Close_INotify] Erro: INotify nao foi inicializado");
        return -1;
    }
}

/*
    Adiciona um monitor do INotify no diretório especificado por directory_path e cria uma nova thread na qual o monitor ficará rodando lendo eventos.
    Quando um evento é detectado, é chamada uma das 3 funções callback especificadas pelo parâmetros fileCreateCallback, fileModifyCallback e fileDeleteCallback, respectivamente.
    Retorna o índice do monitor no array de monitores ativos se executou com sucesso e -1 se ocorreu algum erro.
*/
int AddMonitor(char* directory_path, void* (*fileCreateCallback)(struct inotify_event*), void* (*fileModifyCallback)(struct inotify_event*), void* (*fileDeleteCallback)(struct inotify_event*)){
    if(isInotifyInitialized == 0){ 
        if(Initialize_INotify() == -1)
            return -1; 
    }

    int newMonitorIndex = findMonitorSlot();
    if(newMonitorIndex == -1){
        return -1;
    }

    int newMonitorDescriptor = inotify_add_watch(InotifyInstance, directory_path, IN_CREATE|IN_MODIFY|IN_DELETE);
    if(newMonitorDescriptor == -1){
        printf("[AddMonitor] Erro ao criar o monitor do Inotify\n");
        return -1;
    }

    T_ActiveMonitor newMonitor;
    newMonitor.isValid = 0;
    newMonitor.monitorDescriptor = newMonitorDescriptor;
    newMonitor.callbacks.fileCreateCallback = fileCreateCallback;
    newMonitor.callbacks.fileDeleteCallback = fileDeleteCallback;
    newMonitor.callbacks.fileModifyCallback = fileModifyCallback;

    activeMonitors[newMonitorIndex] = newMonitor;

    int newThreadIndex = createNewThread(MonitorMainLoop, &(activeMonitors[newMonitorIndex]));
    if(newThreadIndex == -1){
        return -1;
    }
    newMonitor.threadIndex = newThreadIndex;
    activeMonitors[newMonitorIndex].isValid = 1;

    return newMonitorIndex;
}


/*
    Fecha o monitor especificado pelo parâmetro monitorDescriptor e mata a thread correspondente
    Retorna 0 se executou com sucesso e -1 se ocorreu algum erro.
*/
int RemoveMonitor(int monitorIndex){
    if(isInotifyInitialized == 0){
        printf("[RemoveMonitor] Erro: INotify nao foi inicializado");
        return -1;
    }

    T_ActiveMonitor monitorToBeRemoved = activeMonitors[monitorIndex];
    if(monitorToBeRemoved.isValid == 0){
        printf("[RemoveMonitor] Erro: monitor invalido");
        return -1;
    }

    int killThreadStatus = killThread(monitorToBeRemoved.threadIndex);
    if(killThreadStatus == -1){
        return -1;
    }

    int monitorRemoveStatus = inotify_rm_watch(InotifyInstance, monitorToBeRemoved.monitorDescriptor);
    if(monitorRemoveStatus == -1){
        printf("[RemoveMonitor] Erro: nao foi possivel remover o monitor");
        return -1;
    }

    activeMonitors[monitorIndex].isValid = 0;

    return 0;
}


/*
    Implementa o funcionamento principal de um monitor: um loop infinito que detecta modificações em algum diretório
*/
void* MonitorMainLoop(void* monitor){
    char buffer[BUFFER_LEN];
    int i = 0;
    T_ActiveMonitor thisMonitor = *(T_ActiveMonitor*)monitor;

    while(1){
        i = 0;
        int total_read = read(InotifyInstance, buffer, BUFFER_LEN);
        if(total_read < 0)
            printf("[MonitorMainLoop] Erro ao ler");
        
        while(i < total_read){
            struct inotify_event *event=(struct inotify_event*)&buffer[i];
            if(event->len){
                if(event->mask & IN_CREATE){
                    thisMonitor.callbacks.fileCreateCallback(event);
                } else if(event->mask & IN_MODIFY){
                    thisMonitor.callbacks.fileModifyCallback(event);
                } else if(event->mask & IN_DELETE){
                    thisMonitor.callbacks.fileDeleteCallback(event);
                }
                i+=MONITOR_EVENT_SIZE+event->len;
            }
        }
    }
}


/*
    Retorna o primeiro índice vago do array de monitores ativos
    Se o array estiver cheio, retorna -1;
*/
int findMonitorSlot(){
    int i = 0;
    for(i = 0; i<MAX_ACTIVE_MONITORS; i++){
        if(activeMonitors[i].isValid == 0){
            return i;
        }
    }
    printf("[findMonitorSlot]Erro: numero maximo de monitores simultaneos atingido");
        return -1;
}


/*
    Inicializa o array de monitores ativos marcando todos como invalidos
*/
void InitializeMonitorArray(){
    int i = 0;
    for(i = 0; i<MAX_ACTIVE_MONITORS; i++){
        activeMonitors[i].isValid = 0;
    }
}



/*DEBUG========================================================================

void createdCallback(struct inotify_event* event){
    if(event->mask & IN_ISDIR){
        printf("Directory %s was created\n", event->name);
    } else {
        printf("File %s was created\n", event->name);
    }
}

void modifiedCallback(struct inotify_event* event){
    if(event->mask & IN_ISDIR){
        printf("Directory %s was modified\n", event->name);
    } else {
        printf("File %s was modified\n", event->name);
    }
}

void deletedCallback(struct inotify_event* event){
    if(event->mask & IN_ISDIR){
        printf("Directory %s was deleted\n", event->name);
    } else {
        printf("File %s was deleted\n", event->name);
    }
}

int main(int argc, char** argv){
    
    AddMonitor("/home/amaury/Desktop", createdCallback, modifiedCallback, deletedCallback);
    AddMonitor("/home/amaury/Desktop/testeste3", createdCallback, modifiedCallback, deletedCallback);
    pthread_exit(NULL);
    return 0;
}

FIM DEBUG========================================================================*/