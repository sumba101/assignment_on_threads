#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>
#include <pthread.h>

#define one 1
#define zero 0

void swap(int* a, int* b);

int partition (int arr[], int low, int high);

void quickSort(int arr[], int low, int high);

int * shareMem(size_t size);

void quicksort_C(int *arr, int l, int r);

struct arg{
    int l;
    int r;
    int* arr;
};

void *threaded_quicksort(void* a);

void runSorts(int n);

int main(){

    int n;
    scanf("%d", &n);
    runSorts(n);
}

void runSorts(int n) {

    struct timespec ts;

    //getting shared memory
    int *arr = shareMem(sizeof(int)*(n+one));
    for(long long int i=zero;i<n;i+=one) scanf("%d", arr+i);


    printf("Running concurrent_quicksort for n = %d\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

    //multiprocess
    quicksort_C(arr, zero, n-one);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t1 = en-st;

    int brr[n+one];
    for(int i=zero;i<n;i+=one) brr[i] = arr[i];

    pthread_t tid;
    struct arg a;
    a.l = zero;
    a.r = n-one;
    a.arr = brr;
    printf("Running threaded_quicksort for n = %d\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    //multithreaded
    pthread_create(&tid, NULL, threaded_quicksort, &a);
    pthread_join(tid, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t2 = en-st;

    printf("Running normal_quicksort for n = %d\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    // normal
    quickSort(brr, zero, n-one);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t3 = en - st;

    printf("normal_quicksort ran:\n\t[ %Lf ] times faster than concurrent_mergesort\n\t[ %Lf ] times faster than threaded_concurrent_mergesort\n\n\n", t1/t3, t2/t3);
    shmdt(arr);
}

void *threaded_quicksort(void *a) {
    struct arg *args = (struct arg*) a;

    int l = args->l;
    int r = args->r;
    int *arr = args->arr;
    if(l>r) return NULL;

    if(r-l+one<=5){
        for(int i=l;i<r;i+=one)
        {
            int j=i+one;
            for(;j<=r;j+=one)
                if(arr[j]<arr[i] && j<=r)
                {
                    int temp = arr[i];
                    arr[i] = arr[j];
                    arr[j] = temp;
                }
        }
        return NULL;
    }

    int pi=partition(arr,l,r);

    struct arg a1;
    a1.l = l;
    a1.r = pi-1;
    a1.arr = arr;
    pthread_t tid1;
    pthread_create(&tid1, NULL, threaded_quicksort, &a1);

    struct arg a2;
    a2.l = pi+1;
    a2.r = r;
    a2.arr = arr;
    pthread_t tid2;
    pthread_create(&tid2, NULL, threaded_quicksort, &a2);

    //wait for the two halves to get sorted
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    return NULL;
}

void quicksort_C(int *arr, int l, int r) {
    if(l>r) _exit(one);

    //insertion sort
    if(r-l+one<=5){
        for(int i=l;i<r;i+=one)
        {
            int j=i+one;
            for(;j<=r;j+=one)
                if(arr[j]<arr[i] && j<=r)
                {
                    int temp = arr[i];
                    arr[i] = arr[j];
                    arr[j] = temp;
                }
        }
        return;
    }

    int pid1 = fork();
    int pid2;

    int pi = partition(arr, l, r);

    if(pid1==zero){
        quicksort_C(arr, l, pi - one);
        _exit(one);
    }
    else{
        pid2 = fork();
        if(pid2==zero){
            //sort right half array
            quicksort_C(arr, pi + one, r);
            _exit(one);
        }
        else{
            //wait for the right and the left half to get sorted
            waitpid(pid1, NULL, zero);
            waitpid(pid2, NULL, zero);
        }
    }
}

int *shareMem(size_t size) {
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int*)shmat(shm_id, NULL, zero);
}

void quickSort(int *arr, int low, int high) {
    if(low>high) _exit(one);

    if(high-low+one<=5){
        for(int i=low;i<high;i+=one)
        {
            int j=i+one;
            for(;j<=high;j+=one)
                if(arr[j]<arr[i] && j<=high)
                {
                    int temp = arr[i];
                    arr[i] = arr[j];
                    arr[j] = temp;
                }
        }
        return;
    }

    int pi = partition(arr, low, high);

    quickSort(arr, low, pi - one);
    quickSort(arr, pi + one, high);

}

int partition(int *arr, int low, int high) {

    ///Setting random element as the last element which shall be used as pivot
    srand(time(NULL));
    int random = low + rand() % (high - low);
    swap(&arr[random], &arr[high]);

    int pivot = arr[high]; /// pivot
    int i = (low - one); /// Index of smaller element

    for (int j = low; j <= high- one; j+=one)
    {
        /// If current element is smaller than the pivot
        if (arr[j] < pivot)
        {
            i+=one; // increment index of smaller element
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + one], &arr[high]);
    return (i + one);
}

void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}



