#include <stdio.h>
#include <stdlib.h>
#define SEED config[0]
#define INIT_TIME config[1]
#define FIN_TIME config[2]
#define ARRIVE_MIN config[3]
#define ARRIVE_MAX config[4]
#define QUIT_PROB config[5]
#define CPU_MIN config[6]
#define CPU_MAX config[7]
#define DISK1_MIN config[8]
#define DISK1_MAX config[9]
#define DISK2_MIN config[10]
#define DISK2_MAX config[11]
#define SYSTEM_EXIT 0
#define ARRIVAL 1
#define CPU_ARRIVAL 2
#define CPU_EXIT 3
#define DISK1_ARRIVAL 4
#define DISK1_EXIT 5
#define DISK2_ARRIVAL 6
#define DISK2_EXIT 7
#define TERMINATE 8

int config[12];

typedef struct events {
    int time;
    int job;
    int action;
    struct events *next;
} Event;
typedef struct jobs {
    int id;
    struct jobs *next;
} Job;

void arrival(Event **eventPtr, Job **cpuPtr);
void cpu_arrival(Event ** eventPtr, Job **cpuPtr);
void cpu_exit(Event **eventPtr, Job **cpuPtr, Job **disk1Ptr, Job **disk2Ptr);
void disk1_arrival(Event **eventPtr, Job **disk1Ptr);
void disk2_arrival(Event **eventPtr, Job **disk2Ptr);
void disk1_exit(Event **eventPtr, Job **disk1Ptr, Job **cpuPtr);
void disk2_exit(Event **eventPtr, Job **disk2Ptr, Job **cpuPtr);
void offerEvent(int time, int job, int action, Event **headPtr);
void removeEvent(Event **headPtr);
void offerJob(int id, Job **headPtr);
void removeJob(Job **headPtr);
int countJobs(Job *serverHead);
void writeLog(FILE *log, Event *eventHead);
void init();
int randInt(int low, int high);
void printEvents(Event *eventHead);
void printJobs(Job *jobHead);

int main(int argc, char** argv) {
    
    // create log file, add configuration to it
    FILE *log = fopen("log.txt","w+");
    FILE *cfile = fopen("config.txt","r");
    char c;
    while ((c = fgetc(cfile)) != EOF) {
        fputc(c, log);
    }
    fprintf(log,"\r\n\r\n%s\r\n","LOG:");
    fclose(cfile);
    
    init(); // load configuration values
    Event *eventHead = NULL; // declare/initialize heads of queues
    Job *cpuHead = NULL;
    Job *disk1Head = NULL;
    Job *disk2Head = NULL;
    
    int time = INIT_TIME; // init simulation counters and place first/last job
    int running = 1;
    int numJobs = 0;
    int totalJobs = 0;
    double cpuSize = 0;
    double disk1Size = 0;
    double disk2Size = 0;
    double cpuBusy = 0;
    double disk1Busy = 0;
    double disk2Busy = 0;
    double jobSize = 0;
    
    offerEvent(INIT_TIME,1,ARRIVAL,&eventHead);
    offerEvent(FIN_TIME,0,TERMINATE,&eventHead);
    
    // main loop
    while(running == 1) {
        // process events occurring at current time:
        while(eventHead != NULL && eventHead->time == time) {
            // log action:
            writeLog(log,eventHead);
            // execute action:
            if (eventHead->action == ARRIVAL) {
                arrival(&eventHead,&cpuHead);
                ++numJobs;
                ++totalJobs;
            }
            else if (eventHead->action == CPU_ARRIVAL) {
                cpu_arrival(&eventHead,&cpuHead);
            }
            else if (eventHead->action == CPU_EXIT) {
                cpu_exit(&eventHead, &cpuHead, &disk1Head, &disk2Head);
            }
            else if (eventHead->action == DISK1_ARRIVAL) {
                disk1_arrival(&eventHead, &disk1Head);
            }
            else if (eventHead->action == DISK1_EXIT) {
                disk1_exit(&eventHead, &disk1Head, &cpuHead);
            }
            else if (eventHead->action == DISK2_ARRIVAL) {
                disk2_arrival(&eventHead, &disk2Head);
            }
            else if (eventHead->action == DISK2_EXIT) {
                disk2_exit(&eventHead, &disk2Head, &cpuHead);
            }
            else if (eventHead->action == SYSTEM_EXIT) {
                --numJobs;
            }
            else if(eventHead->action == TERMINATE) {
                running = 0;
                break;
            }
            removeEvent(&eventHead);
        }
        
        // stats and bookkeeping:
        cpuSize += countJobs(cpuHead);
        disk1Size += countJobs(disk1Head);
        disk2Size += countJobs(disk2Head);
        if (cpuHead != NULL) cpuBusy += 1;
        if (disk1Head != NULL) disk1Busy += 1;
        if (disk2Head != NULL) disk2Busy += 1;
        jobSize += numJobs;
        ++time;
        
        // printf("Jobs: %d\n",numJobs);
    }
    
    // print stats
    puts("RESULTS: (see log.txt for config and log.)\n");
    printf("Avg. CPU queue: %f\n",cpuSize/(FIN_TIME-INIT_TIME+1));
    printf("Avg. DISK 1 queue: %f\n",disk1Size/(FIN_TIME-INIT_TIME+1));
    printf("Avg. DISK 2 queue: %f\n\n",disk1Size/(FIN_TIME-INIT_TIME+1));
    printf("CPU utilization: %f\n",cpuBusy/(FIN_TIME-INIT_TIME+1));
    printf("DISK 1 utilization: %f\n",disk1Busy/(FIN_TIME-INIT_TIME+1));
    printf("DISK 2 utilization: %f\n\n",disk2Busy/(FIN_TIME-INIT_TIME+1));
    printf("CPU avg. response time: %d\n",CPU_MIN+((CPU_MAX-CPU_MIN)/2));
    printf("DISK 1 avg. response time: %d\n",DISK1_MIN+((DISK1_MAX-DISK1_MIN)/2));
    printf("DISK 2 avg. response time: %d\n\n",DISK2_MIN+((DISK2_MAX-DISK1_MIN)/2));
    printf("CPU throughput: %f\n",(float)1/(CPU_MIN+((CPU_MAX-CPU_MIN)/2)));
    printf("DISK 1 throughput: %f\n",(float)1/(DISK1_MIN+((DISK1_MAX-DISK1_MIN)/2)));
    printf("DISK 2 throughput: %f\n\n",(float)1/(DISK2_MIN+((DISK2_MAX-DISK2_MIN)/2)));
    printf("Number of jobs: %d\n", totalJobs);
    printf("Avg. active jobs: %f\n", jobSize/(FIN_TIME-INIT_TIME+1));
    
    fclose(log);
    
    return (EXIT_SUCCESS);
}

void arrival(Event **eventPtr, Job **cpuPtr) {
    // send job to cpu, create cpu arrival event if first in queue
    if (*cpuPtr == NULL) { 
        offerEvent((*eventPtr)->time+1,(*eventPtr)->job,CPU_ARRIVAL,eventPtr);
    }
    offerJob((*eventPtr)->job,cpuPtr);
    
    // generate new job
    offerEvent((*eventPtr)->time+randInt(ARRIVE_MIN,ARRIVE_MAX),
            (*eventPtr)->job+1,ARRIVAL,eventPtr);
}

void cpu_arrival(Event **eventPtr, Job **cpuPtr) {
    // create event for this job to exit cpu
    offerEvent((*eventPtr)->time+randInt(CPU_MIN,CPU_MAX),
            (*cpuPtr)->id,CPU_EXIT,eventPtr);
}

void cpu_exit(Event **eventPtr, Job **cpuPtr, Job **disk1Ptr, Job **disk2Ptr) {
    removeJob(cpuPtr);
    // if cpu has next job and no arrival is queued, create arrival event
    if ((*cpuPtr) != NULL && (*eventPtr)->next->action != CPU_ARRIVAL) {
        offerEvent((*eventPtr)->time+1,(*cpuPtr)->id,
                CPU_ARRIVAL,eventPtr);
    }
    // check if job will exit system this time; if so, generate exit event
    if (randInt(1,100) < QUIT_PROB) {
        offerEvent((*eventPtr)->time,(*eventPtr)->job,SYSTEM_EXIT,eventPtr);
        return;
    }
    // determine which disk will serve job, enter it into that queue
    int destination;
    if (countJobs(*disk1Ptr) < countJobs(*disk2Ptr)) destination = 1;
    else if (countJobs(*disk1Ptr) > countJobs(*disk2Ptr)) destination = 2;
    else destination = randInt(1,2);
    if (destination == 1) {
        if (*disk1Ptr == NULL) {
            offerEvent((*eventPtr)->time+1,(*eventPtr)->job,
                    DISK1_ARRIVAL,eventPtr);
        }
        offerJob((*eventPtr)->job,disk1Ptr);
    }
    else {
        if (*disk2Ptr == NULL) { 
            offerEvent((*eventPtr)->time+1,(*eventPtr)->job,
                    DISK2_ARRIVAL,eventPtr);
        }
        offerJob((*eventPtr)->job,disk2Ptr);
    }
}

void disk1_arrival(Event **eventPtr, Job **disk1Ptr) {
    // create event for this job to exit disk 1
    offerEvent((*eventPtr)->time+randInt(DISK1_MIN,DISK1_MAX),
            (*disk1Ptr)->id,DISK1_EXIT,eventPtr);
}

void disk2_arrival(Event **eventPtr, Job **disk2Ptr) {
    // create event for this job to exit disk 2
    offerEvent((*eventPtr)->time+randInt(DISK2_MIN,DISK2_MAX),
            (*disk2Ptr)->id,DISK2_EXIT,eventPtr);
}

void disk1_exit(Event **eventPtr, Job **disk1Ptr, Job **cpuPtr) {
    removeJob(disk1Ptr);
    // if disk queue has next job and no arrival is queued, create arrival event
    if ((*disk1Ptr) != NULL && (*eventPtr)->next->action != DISK1_ARRIVAL) {
        offerEvent((*eventPtr)->time+1,(*disk1Ptr)->id,
                DISK1_ARRIVAL,eventPtr);
    }
    // send disk job back to cpu
    if (*cpuPtr == NULL) {
        offerEvent((*eventPtr)->time+1,(*eventPtr)->job,
                CPU_ARRIVAL,eventPtr);
    }
    offerJob((*eventPtr)->job,cpuPtr);
}

void disk2_exit(Event **eventPtr, Job **disk2Ptr, Job **cpuPtr) {
    removeJob(disk2Ptr);
    // if disk queue has next job and no arrival is queued, create arrival event
    if ((*disk2Ptr) != NULL && (*eventPtr)->next->action != DISK2_ARRIVAL) {
        offerEvent((*eventPtr)->time+1,(*disk2Ptr)->id,
                DISK2_ARRIVAL,eventPtr);
    }
    // send disk job back to cpu
    if (*cpuPtr == NULL) {
        offerEvent((*eventPtr)->time+1,(*eventPtr)->job,
                CPU_ARRIVAL,eventPtr);
    }
    offerJob((*eventPtr)->job,cpuPtr);
}

void offerEvent(int time, int job, int action, Event **headPtr) {
    // allocates memory for an event and inserts into priority queue
    // with given head
    Event *event = (Event *) malloc(sizeof(Event));
    event->time = time;
    event->job = job;
    event->action = action;
    event->next = NULL;
    // CASE 1: there is nothing in queue
    if (*headPtr == NULL) {
        *headPtr = event;
        return;
    }
    // CASE 2: new event is sooner than first event in queue
    Event *temp = *headPtr;
    if (event->time < (*headPtr)->time) {
        *headPtr = event;
        event->next = temp;
    }
    // CASE 3: find next later node and enter queue before it
    else {
        while (temp->next != NULL) {
            if (event->time < temp->next->time) {
                event->next = temp->next;
                temp->next = event;
                return;
            }
        temp = temp->next;
        }
        temp->next = event; // reached end of list
    }
}

void removeEvent(Event **headPtr) {
    // CASE 1: queue is empty
    if(*headPtr == NULL) return;
    // CASE 2: make head equal to next pointer, free previous event
    Event *front = *headPtr;
    *headPtr = (*headPtr)->next;
    free(front);
}

void offerJob(int id, Job **headPtr) {
    // allocates memory for job and inserts into queue with given head
    Job *job = (Job *) malloc(sizeof(Job));
    job->id = id;
    job->next = NULL;
    // CASE 1: queue is empty
    if (*headPtr == NULL) {
        *headPtr = job;
        return;
    }
    // CASE 2: insert event at end of queue
    Job *temp = *headPtr;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp-> next = job;
    
}

void removeJob(Job **headPtr) {
    // CASE 1: queue is empty
    if(*headPtr == NULL) return;
    // CASE 2: make head equal to next pointer, free previous event
    Job *front = *headPtr;
    *headPtr = (*headPtr)->next;
    free(front);
}

int countJobs(Job *serverHead) {
    // counts jobs in a given server queue
    int count = 0;
    while (serverHead != NULL) {
        ++count;
        serverHead = serverHead->next;
    }
    return count;
}

void writeLog(FILE *log, Event *eventHead) {
    int time = eventHead->time;
    int job = eventHead->job;
    int action = eventHead->action;
    char *events[] = {
        "exits system.","enters system.","arrives at CPU.","finishes at CPU.",
        "arrives at DISK 1.","finishes at DISK 1.","arrives at DISK 2.",
        "finishes at DISK 2."
    };
    if (action == TERMINATE) {
        fprintf(log,"TIME %d: Simulation terminates.\r\n",time);
        // printf("TIME %d: Simulation terminates.\n",time);
    }
    else {
        fprintf(log,"TIME %d: Job %d %s\r\n",time,job,events[action]);
        // printf("TIME %d: Job %d %s\n",time,job,events[action]);
    }
}

void init() {
    // loads config values from text file into global array
    FILE *fPtr = fopen("config.txt","r");
    fscanf(fPtr,"%*s%d%*s%d%*s%d%*s%d%*s%d%*s%d%*s%d%*s%d%*s%d%*s%d%*s%d%*s%d",
            &config[0],&config[1],&config[2],&config[3],&config[4],&config[5],
            &config[6],&config[7],&config[8],&config[9],&config[10],&config[11]);
    fclose(fPtr);
    srand(SEED); // seed random number generator
}

int randInt(int low, int high) {
    // generates random integer in range low to high
    return (low + rand()%(high + 1 - low));
}

void printEvents(Event *eventHead) {
    // for debugging queue
    puts("PRIORITY QUEUE:");
    Event *temp = eventHead;
    int i = 1;
    while (temp != NULL) {
        printf("Position %d: %d\n",i,temp->job);
        ++i;
        temp = temp->next;
    }
}

void printJobs(Job *jobHead) {
    // for debugging queue
    puts("SERVER QUEUE:");
    Job *temp = jobHead;
    int i = 1;
    while (temp != NULL) {
        printf("Position %d: %d\n",i,temp->id);
        ++i;
        temp = temp->next;
    }
}