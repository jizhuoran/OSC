//
//  main.c
//  Producer-Consumer Problem
//
//  Created by jizhuoran on 16/6/29.
//  Copyright © 2016年 jizhuoran. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

typedef int buffer_item;
#define BUFFER_SIZE 5

void *produce();
void *consume();
void sleep_rand();
void buffer_initialization();

buffer_item buffer[BUFFER_SIZE];

pthread_mutex_t mutex;
int count = 0;

int main(int argc, const char * argv[]) {
    int sleep_time;
    int number_of_producer;
    int number_of_consumer;
    
    printf("Please enter the sleep time: ");
    scanf("%d",&sleep_time);
    printf("Please enter the number of producer: ");
    scanf("%d",&number_of_producer);
    printf("Please enter the number of consumer: ");
    scanf("%d",&number_of_consumer);
    
    pthread_t *producer = malloc(sizeof(pthread_t) * number_of_producer);
    pthread_t *consumer = malloc(sizeof(pthread_t) * number_of_consumer);
    
    buffer_initialization();
    for (int i = 0; i < number_of_producer; ++i) {
        pthread_create(&producer[i], NULL, produce, NULL);
    }
    for (int i = 0; i < number_of_consumer; ++i) {
        pthread_create(&consumer[i], NULL, consume, NULL);
    }
    
    sleep(sleep_time);
    
    return 0;
    
}

void buffer_initialization() {
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        buffer[i] = -1;
    }
}

void *produce() {
    while (1) {
        pthread_mutex_lock(&mutex);
        if ((count+1)+1 != 5) {
            buffer[count] = 1;
            printf("producing \n");
            printf("The buffer is %d, %d, %d, %d, %d\n\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
            sleep_rand();
            count++;
            pthread_mutex_unlock(&mutex);
        } else {
            pthread_mutex_unlock(&mutex);
        }
        
        
    }
    
}
void *consume() {
    while (1) {
        pthread_mutex_lock(&mutex);
        if (count != 0) {
            count--;
            buffer[count] = -1;
            printf("consuming \n");
            printf("The buffer is %d, %d, %d, %d, %d\n\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
            sleep_rand();
            pthread_mutex_unlock(&mutex);
        }else {
            pthread_mutex_unlock(&mutex);
        }
    }
}
void sleep_rand() {
    int time = rand() % 2 + 1;
    sleep(time);
}
