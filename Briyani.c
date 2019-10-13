#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

int M, N, K, loaded[1000], slots[1000];
pthread_t chef_id[1000], student_id[1000], table_id[1000];
pthread_mutex_t chef_table_lock[1000], table_lock[1000];

int min(int x, int y)
{
    return x < y ? x : y;
}

void biryani_ready(int index, int w, int r, int p)
{
    for(int i = 0;i<r;i++)
    {
        int served = 0;
        for(int j = 0;j<N;j++)
        {
            if(!pthread_mutex_trylock(&chef_table_lock[j]))
            {
                served = 1;
                printf("Vessel %d by chef %d is served in table %d\n", i+1, index+1, j+1);
                fflush(stdout);
                loaded[j] = p;
                break;
            }
        }
        i-=1-served;
    }
}

void *chef(void *index)
{
    int *temp=((int*)index);

    int in = *temp;
    while(1)
    {
        int w = rand()%4+2, r = rand()%10+1, p = rand()%4+2;
        printf("Chef %d will now take %d seconds to prepare %d vessel(s) each with portions for %d student(s)\n", in+1, w, r, p);
        fflush(stdout);
        sleep(w);
        biryani_ready(in, w, r, p);
        return NULL;
    }

    return NULL;
}

void ready_to_serve(int index)
{
    pthread_mutex_unlock(&table_lock[index]);
    while(slots[index]);
    pthread_mutex_lock(&table_lock[index]);
}

void *serving_table(void *index)
{
    int *temp=((int*)index);
    int in = (*temp);

    while(1)
    {
        while(!loaded[in]);

        slots[in] = min(rand()%10+1, loaded[in]);
        printf("Serving table %d made %d slot(s) available\n", in+1, slots[in]);
        fflush(stdout);

        ready_to_serve(in);

        loaded[in] -= slots[in];

        if(!loaded[in])
            pthread_mutex_unlock(&chef_table_lock[in]);
        sleep(1);
    }

    return NULL;
}

void student_in_slot(int index, int table)
{
    printf("Student %d is having a meal at table %d\n", index+1, table+1);
    sleep(2);
    slots[table]--;
}

void wait_for_slot(int index)
{
    int served = 0;
    printf("Student %d is waiting for a slot\n", index+1);
    while(!served)
    {
        for(int i = 0;i<N;i++)
        {
            if(!pthread_mutex_trylock(&table_lock[i]))
            {
                student_in_slot(index, i);
                served = 1;
                pthread_mutex_unlock(&table_lock[i]);
                break;
            }
        }
    }
}

void *student(void *index)
{
    int *temp=((int*)index);
    int in = *temp;
    wait_for_slot(in);

    return NULL;
}


int main()
{
    srand(time(NULL));
    printf("Number of chefs: ");
    scanf("%d", &M);
    printf("Number of serving tables: ");
    scanf("%d", &N);
    printf("Number of students: ");
    scanf("%d", &K);
    int chefin[1000], servein[1000], studentin[1000];
    for(int i = 0;i<M;i++)
    {
        chefin[i] = i;
        if(pthread_create(&(chef_id[i]), NULL, &chef, &chefin[i]))
            printf("Something went wrong while creating thread for chef %d\n", i+1);
        usleep(100);
    }
    for(int i = 0;i<N;i++)
    {
        servein[i] = i;
        if(pthread_mutex_init(&chef_table_lock[i], NULL))
        {
            printf("Something went wrong\n");
            return 0;
        }
        if(pthread_mutex_init(&table_lock[i], NULL))
        {
            printf("Something went wrong\n");
            return 0;
        }
        pthread_mutex_lock(&table_lock[i]);
        if(pthread_create(&(table_id[i]), NULL, &serving_table, &servein[i]))
            printf("Something went wrong while creating thread for serving table %d\n", i+1);
        usleep(100);
    }
    for(int i = 0;i<K;i++)
    {
        studentin[i] = i;
        if(pthread_create(&(student_id[i]), NULL, &student, &studentin[i]))
            printf("Something went wrong while creating thread for student %d\n", i+1);
        usleep(100);
    }

    for(int i = 0;i<K;i++)
        pthread_join(student_id[i], NULL);
    for(int i = 0;i<N;i++)
        pthread_mutex_destroy(&chef_table_lock[i]), pthread_mutex_destroy(&table_lock[i]);


    return 0;
}