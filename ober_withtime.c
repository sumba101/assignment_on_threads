//
// Created by spawnfire on 09/10/19.
//
#include <semaphore.h>
#include<stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#define maxcount 1000

int cab_no,rider_no,server_no;

sem_t server; ///semaphore for the servers

typedef struct Rider{
    bool cabtype; ///0 if pool 1 if premier
    long int maxwait;
    int ridetime;

    int pno;
    pthread_t rider_thread;

}rider;

typedef struct Driver{
    int pno;
    bool waitstate;
    bool onPrem;
    bool onPool1;
    bool onPool2;

    pthread_mutex_t lock;
}driver;


driver d[maxcount];
rider r[maxcount];

void bookcab(rider *current, driver *pDriver, bool *booked);

void accept_payment(rider *current);

void *rider_f(void *args);

void initialize_driver(int no);

int main(){

    printf("Enter number of cabs, riders and servers ");
    scanf("%d %d %d",&cab_no,&rider_no,&server_no);
    sem_init(&server,0,server_no); ///initialize the servers number

    for (int k = 1; k <= cab_no; ++k) {  ///initializing the mutex locks of drivers
        pthread_mutex_init(&d[k].lock,NULL);
        d[k].waitstate=true;
        d[k].pno=k;
    }

    for (int i = 1; i <= rider_no ; ++i) {
        printf("Enter cabtype(0 for pool 1 for premier)\n max wait time and \n ride time of rider number %d\n",i);
        int t;
        scanf("%d %ld %d",&t,&r[i].maxwait,&r[i].ridetime);
        r[i].cabtype=(t)?true:false;
        r[i].pno=i;
    }

    for (int j = 1; j <= rider_no; ++j) { ///threads for each rider
        pthread_create(&r[j].rider_thread,NULL,rider_f,&r[j]);
    }

    for (int l = 1; l <= rider_no; ++l) { ///wait till all riders are done
        pthread_join(r[l].rider_thread,NULL);
    }

    printf("All riders have been processed \n");
    sem_destroy(&server); ///clear up the semaphore
}

void *rider_f(void *args) {
    rider *current=(rider *)args;

    int driver_no = 0;

    bool booked=false;

    long int timer=-1;
    if(current->cabtype){ ///this one means premier

        time_t time1;
        time_t time2;
        time(&time1); ///initializing the time variable

        while(current->maxwait > timer && !booked){
            for (int i = 1; i <= cab_no && current->maxwait > timer; ++i) { ///look for a cab
                if(d[i].waitstate ){///if a cab is not used yet
                    bookcab(current,&d[i],&booked);
                    driver_no=i;
                }

                time(&time2);
                timer=time2-time1; ///time elapsed in waiting mode
            }
        }

    } else{ ///this one means pool type

        time_t time1;
        time_t time2;
        time(&time1); ///initializing the time variable

        while(current->maxwait > timer && !booked){
            for (int i = 1; i <= cab_no && current->maxwait > timer; ++i) { ///look for a cab

                if((d[i].onPool1 && !d[i].onPool2) ||d[i].waitstate){///if a cab is pool1 type without pool2
                    bookcab(current,&d[i],&booked);
                    driver_no=i;
                }
                time(&time2);
                timer=time2-time1; ///time elapsed in waiting mode
            }
        }

    }

    if(timer>=current->maxwait && !booked){ ///if max wait time has elapsed and rider hasnt booked something yet
        printf("The passenger number %d has waited for too long and has hence Timed out \n",current->pno);
        return NULL;
    }

    if(booked){ ///the rider has received
        printf("Rider %d has started his ride with Driver %d\n",current->pno,rider_no);
        sleep(current->ridetime);
        printf("Rider %d has finished his ride with Driver %d\n",current->pno,rider_no);
        initialize_driver(driver_no);
        accept_payment(current);
    }

    return NULL;
}

void initialize_driver(int no) {
    pthread_mutex_lock(&d[no].lock); ///lock up the driver
    if(d[no].onPrem || d[no].onPool1){
        d[no].onPrem=false;
        d[no].waitstate=true;
        d[no].onPool1=false;
        d[no].onPool2=false;
    }
    if(d[no].onPool2){
        d[no].onPool2=false;
        d[no].onPool1=true;
    }
    pthread_mutex_unlock(&d[no].lock); ///unlock the driver
}

void accept_payment(rider *current) {
    sem_wait(&server);
    printf("Payment of rider %d is being processed \n",current->pno);
    sleep(2);
    printf("Payment of rider %d is completed \n",current->pno);
    sem_post(&server);
}

void bookcab(rider *current, driver *pDriver, bool *booked) {
    pthread_mutex_lock(&pDriver->lock); ///lock up the driver
    pDriver->waitstate=false; ///Driver is getting booked and is not waiting

    if(current->cabtype){ ///its premier driver
        pDriver->onPrem=true;
        pDriver->onPool1= pDriver->onPool2=false;
        printf("Passenger %d is assigned driver %d for Premier ride",current->pno,pDriver->pno);
    } else{ ///its share driver
        if(pDriver->onPool1){
            pDriver->onPool2=true;
            pDriver->onPool1=false;
        } else{
            pDriver->onPool1=true;
            pDriver->onPool2=false;
        }
        pDriver->onPrem=false;
    }

    (*booked)= true;
    pthread_mutex_unlock(&pDriver->lock); ///free up driver
}

