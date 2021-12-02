#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

// Definitions for number of philosophers,
// left philosopher and right philosopher
#define PHILOSOPHER_NUM 5
#define LEFT ((id-1)+PHILOSOPHER_NUM) % 5
#define RIGHT (id+1) % 5
#define MAX_MEALS_SEC 5
#define MAX_THINK_SEC 10

enum { THINKING, HUNGRY, EATING } state[PHILOSOPHER_NUM];
// Mutex, condition variable, threads
pthread_mutex_t lock;
pthread_cond_t cond[PHILOSOPHER_NUM];
pthread_t phil[PHILOSOPHER_NUM];

// Funtion declarations
void think(int id);
void pickup_forks(int id);
void eat(int id);
void return_forks(int id);
void* philosopher(void* num);

int main() {
    int ph_num[PHILOSOPHER_NUM];

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init error\n");
        return 1;
    }

    //Init cond variables
    for (int i = 0; i < PHILOSOPHER_NUM; i++) {
        if (pthread_cond_init(&cond[i], NULL) != 0) {
            printf("\n cv init error\n");
            return 1;
        }
    }

    //Create threads
    for (int i = 0; i < PHILOSOPHER_NUM; i++) {
        ph_num[i] = i;
        pthread_create(&phil[i], NULL, philosopher, &ph_num[i]);
    }

    //Join threads
    for (int i = 0; i < PHILOSOPHER_NUM; i++) {
        pthread_join(phil[i], NULL);
    }

    //Destroy the mutex and conditional variables
    pthread_mutex_destroy(&lock);
    for (int i = 0; i < PHILOSOPHER_NUM; i++) {
        pthread_cond_destroy(&cond[i]);
    }
    return 0;
}

void think(int id) {
    int thinkTime = ((rand()) % MAX_THINK_SEC) + 1;

    printf("Philosopher %d will be thinking for %d seconds\n", id, thinkTime);
    sleep(thinkTime);
    printf("Philosopher %d finished thinking\n", id);
}

void pickup_forks(int id) {
    int left = LEFT;
    int right = RIGHT;
    pthread_mutex_lock(&lock);
    state[id] = HUNGRY;

    while ((state[id] == HUNGRY) && ((state[left] == EATING) || (state[right] == EATING))) {
        printf("Philosopher %i is waiting forks to eat \n", id);
        pthread_cond_wait(&cond[id], &lock);
    }
    state[id] = EATING;
    pthread_mutex_unlock(&lock);
}

void eat(int id) {
    int eatingTime = ((rand()) % MAX_MEALS_SEC) + 1;

    printf("Philosopher %d has forks and will be eating for %d seconds\n", id, eatingTime);
    sleep(eatingTime);
    printf("Philosopher %d finished his/her eating ", id);
}

void return_forks(int id) {
    int left = LEFT;
    int right = RIGHT;
    pthread_mutex_lock(&lock);
    state[id] = THINKING;

    printf("and put down forks\n");
    pthread_cond_signal(&cond[left]);
    pthread_cond_signal(&cond[right]);
    pthread_mutex_unlock(&lock);
}

void* philosopher(void* num) {
    int id = *((int*)num);

    think(id);
    pickup_forks(id);
    eat(id);
    return_forks(id);
}