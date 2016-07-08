#include <iostream>
#include <queue>
#include <list>
#include <time.h>
using namespace std;

#define NUM_OF_FRAMES 3
#define NUM_OF_PATTERN 256
#define NUM_OF_PAGES 9
#define random(x) (rand()%x)

int find_element(int f, int *frames);
int find_empty(int *frames);
int find_frame_OPT(int index, int *frames, int *pattern);
int change_LRU(list<int> *l, int frame_number);

int swap_in_FIFO(int f, int *frames, queue<int> *q);
int swap_in_OPT(int index, int *frames, int *pattern);
int swap_in_LRU(int f, int *frames, list<int> *l);

void FIFO(int *frames, int *pattern);
void LRU(int *frames, int *pattern);
void OPT(int *frames, int *pattern);

int main(int argc, char const *argv[])
{
    srand((int)time(0));
    int *frames = new int[NUM_OF_FRAMES];
    for (int i = 0; i < NUM_OF_FRAMES; ++i) {
        frames[i] = -1;
    }
    int *pattern = new int[NUM_OF_PATTERN];
    for (int i = 0; i < NUM_OF_PATTERN; ++i) {
        pattern[i] = random((NUM_OF_PAGES + 1));
    }
    FIFO(frames, pattern);
    LRU(frames, pattern);
    OPT(frames, pattern);
    delete[] frames;
    delete[] pattern;
    return 0;
}

void FIFO(int *frames, int *pattern){
    double count = 0;
    queue<int> q;
    for (int i = 0; i < NUM_OF_PATTERN; ++i) {
        count += swap_in_FIFO(pattern[i], frames, &q);
    }
    cout << "The page fault rate of FIFO is " << count/NUM_OF_PATTERN<<endl;
}

void LRU(int *frames, int *pattern){
    double count = 0;
    list<int> l;
    for (int i = 0; i < NUM_OF_PATTERN; ++i) {
        count += swap_in_LRU(pattern[i], frames, &l);
    }
    cout << "The page fault rate of LRU is " << count/NUM_OF_PATTERN<<endl;
}

void OPT(int *frames, int *pattern) {
    double count = 0;
    for (int i = 0; i < NUM_OF_PATTERN; ++i) {
        count += swap_in_OPT(i, frames, pattern);
    }
    cout << "The page fault rate of LRU is " << count/NUM_OF_PATTERN<<endl;
}

int swap_in_FIFO(int f, int *frames, queue<int> *q) {
    
    if(find_element(f, frames)){
        return 0;
    }
    if(find_empty(frames) > -1){
        frames[find_empty(frames)] = f;
        q->push(find_empty(frames));
        return 1;
    }
    
    int i = q->front();
    q->pop();
    q->push(i);
    frames[i] = f;
    return 1;
}

int swap_in_LRU(int f, int *frames, list<int> *l) {
    if(find_element(f, frames)){
        change_LRU(l, find_element(f, frames));
        return 0;
    }
    if(find_empty(frames)){
        frames[find_empty(frames)] = f;
        l->push_back(find_empty(frames));
        return 1;
    }
    int i = l->front();
    l->pop_front();
    l->push_back(i);
    frames[i] = f;
    return 1;
}


int swap_in_OPT(int index, int *frames, int *pattern) {
    int f = pattern[index];
    if(find_element(f, frames)){
        return 0;
    }
    if(find_empty(frames)){
        frames[find_empty(frames)] = f;
        return 1;
    }
    int i = find_frame_OPT(index, frames, pattern);
    frames[i] = f;
    return 1;
}

int find_frame_OPT(int index, int *frames, int *pattern) {
    int *buffer = new int[NUM_OF_PAGES];
    
    for (int i = 0; i < NUM_OF_PAGES; ++i) {
        buffer[i] = 0;
    }
    for (int i = 0; i < NUM_OF_FRAMES; ++i) {
        buffer[i] = 1;
    }
    int count = NUM_OF_FRAMES;
    int i;
    for (i = index; i < NUM_OF_PATTERN; ++i) {
        if (buffer[pattern[i]]) {
            count--;
        }
        if(count == 0) {
            return find_element(pattern[i], frames);
        }
    }
    delete[] buffer;
    return find_element(pattern[i-1], frames);
    
}


int change_LRU(list<int> *l, int frame_number) {
    for (list<int>::iterator i = l->begin(); i != l->end(); ++i) {
        if( *i == frame_number){
            l->erase(i);
            l->push_back(frame_number);
        }
    }
    
    return 0;
}



int find_element(int f, int *frames) {
    for (int i = 0; i < NUM_OF_FRAMES; ++i) {
        if(frames[i] == f) {
            return i;
        }
    }
    return 0;
}

int find_empty(int *frames) {
    for (int i = 0; i < NUM_OF_FRAMES; ++i) {
        if(frames[i] == -1) {
            return i;
        }
    }
    return -1;
}