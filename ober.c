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
    long double maxwait;
    int ridetime;

    int pno;
    pthread_t rider_thread;
    pthread_cond_t cond;
}rider;

typedef struct Driver{
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

    for (int k = 0; k < cab_no; ++k) {  ///initializing the mutex locks of drivers
        pthread_mutex_init(&d[k].lock,NULL);
        d[k].waitstate=true;
    }

    for (int i = 0; i < rider_no ; ++i) {
        printf("Enter cabtype(0 for pool 1 for premier)\n max wait time and \n ride time of rider number %d\n",i+1);
        int t;
        scanf("%d %Lf %d",&t,&r[i].maxwait,&r[i].ridetime);
        r[i].cabtype=(t)?true:false;
        r[i].pno=i+1;
    }

    for (int j = 0; j < rider_no; ++j) { ///threads for each rider
        pthread_create(&r[j].rider_thread,NULL,rider_f,&r[j]);
    }

    for (int l = 0; l < rider_no; ++l) { ///wait till all riders are done
        pthread_join(r[l].rider_thread,NULL);
    }

    printf("All riders have been processed \n");
    sem_destroy(&server); ///clear up the semaphore
}

void *rider_f(void *args) {
    rider *current=(rider *)args;

    int driver_no = 0;

    bool booked=false;
    

    if(current->cabtype){ ///this one means premier
        while(!booked){
            for (int i = 0; i < cab_no ; ++i) { ///look for a cab
                if(d[i].waitstate ){///if a cab is not used yet
                    bookcab(current,&d[i],&booked);
                    driver_no=i;
                }
            }
        }

    } else{ ///this one means pool type
        while(!booked){
            for (int i = 0; i < cab_no ; ++i) { ///look for a cab
                if(d[i].onPool1 && !d[i].onPool2){///if a cab is pool1 type without pool2
                    bookcab(current,&d[i],&booked);
                    driver_no=i;
                }
            }
            for (int j = 0; j < cab_no ; ++j) { ///if a pool1 type cab is not found, find a waiting cab
                if(d[j].waitstate){
                    bookcab(current,&d[j],&booked);
                    driver_no=j;
                }
            }
        }

    }

    if (!booked){ ///if max wait time has elapsed and rider hasnt booked something yet
        printf("The passenger number %d has waited for too long and has hence Timed out \n",current->pno);
        return NULL;
    }

    if(booked){ ///the rider has received
        printf("Rider %d has started his ride\n",current->pno);
        sleep(current->ridetime);
        printf("Rider %d has finished his ride\n",current->pno);
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
    printf("Payment is complete\n");
    sem_post(&server);
}

void bookcab(rider *current, driver *pDriver, bool *booked) {
    pthread_mutex_lock(&pDriver->lock); ///lock up the driver
    pDriver->waitstate=false; ///Driver is getting booked and is not waiting

    if(current->cabtype){ ///its premier driver
        pDriver->onPrem=true;
        pDriver->onPool1= pDriver->onPool2=false;

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

