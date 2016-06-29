//
//  main.c
//  The_Dining_Philosophers_Problem
//
//  Created by jizhuoran on 16/6/29.
//  Copyright © 2016年 jizhuoran. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define EATING 0
#define THINKING 1

#define STATE int

STATE state[5];
int chopstick[5];


pthread_mutex_t mutex;
pthread_cond_t con_var;

void *at_table(void* i);
void test();

int main(int argc, const char * argv[]) {
    
    for (int i = 0; i < 5; ++i) {
        chopstick[i] = 1;
        state[i] = THINKING;
    }
    
    srand(time(NULL));
    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&con_var, NULL);
    
    pthread_t philosopher[5];
    
    for (int i = 0; i < 5; ++i) {
        pthread_create(&philosopher[i], NULL, at_table, i);
    }
    
    for (int i = 0; i < 5; ++i) {
        pthread_join(philosopher[i], NULL);
    }
    
}

void pickup_forks(int philosopher_number) {
    printf("%d is picking \n", philosopher_number);
    pthread_mutex_lock(&mutex);
    while (!(chopstick[philosopher_number] && chopstick[(philosopher_number+1)%5])) {
        pthread_cond_wait(&con_var, &mutex);
    }
    state[philosopher_number] = EATING;
    chopstick[philosopher_number] = 0;
    chopstick[(philosopher_number + 1) % 5] = 0;
    printf("The philosopher %d is eating\n", philosopher_number);
    printf("And the state of all philosopher_number\n\n");
    for (int i = 0; i < 5 ; ++i) {
        printf("philosopher %d is %d\n", i, state[i]);
    }
    printf("\n");
    
    test();
    
    //printf("At this time, p1 %d p2 %d p3 %d p4 %d p5", state[0], state[1], state[2], state[3], state[4]);
    pthread_mutex_unlock(&mutex);
}

void return_forks(int philosopher_number) {
    pthread_mutex_lock(&mutex);
    
    chopstick[philosopher_number] = 1;
    chopstick[(philosopher_number + 1) % 5] = 1;
    state[philosopher_number] = THINKING;
    
    pthread_cond_signal(&con_var);
    pthread_mutex_unlock(&mutex);
}

void eating() {
    int time = rand() % 2 + 1;
    sleep(time);
}

void *at_table (void* i) {
    while (1) {
        pickup_forks((int)i);
        eating();
        return_forks((int)i);
    }
}

void test() {
    for (int i = 0; i < 5; ++i) {
        if(state[i] == EATING && state[(i+1) % 5] == EATING) {
            return;
        }
    }
}

