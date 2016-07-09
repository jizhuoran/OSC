#include <iostream>
#include<stdio.h>

#define NUM_OF_FRAMES 256
#define PAGE_SIZE 256
using namespace std;
int page_table[NUM_OF_FRAMES];
bool frames_table[NUM_OF_FRAMES];
int frames[NUM_OF_FRAMES][PAGE_SIZE];
int find_free_frames();
FILE * fp;


void la2pa(uint16_t la);

int main(int argc, char const *argv[]) {
    for (int i = 0; i < NUM_OF_FRAMES; ++i) {
        page_table[i] = -1;
        frames_table[i] = true;
    }
    fp=fopen("BACKING_STORE.bin","r+");
    uint16_t temp;
    for (int i = 0; i < 100; ++i) {
        cin>>temp;
        la2pa(temp);
    }
    return 0;
}

void la2pa(uint16_t la) {
    int pte = (la >> 8) & 0xFF;
    int offset = la & 0xFF;
    if(page_table[pte] == -1) {
        page_table[pte] = find_free_frames();
        int8_t *temp = new int8_t[PAGE_SIZE];
        fseek(fp, sizeof(int8_t) * PAGE_SIZE * (pte), SEEK_SET);
        fread(temp,sizeof(int8_t) ,PAGE_SIZE,fp);
        for (int i = 0; i < PAGE_SIZE; ++i) {
            frames[ page_table[pte] ][i] = temp[i];
        }
        delete[] temp;
    }
    cout<<"Virtual address: "<<la<<"Physical address: "<<(page_table[pte] << 8) + offset <<" Value: "<<frames[ page_table[pte] ][offset]<<endl;
}

int find_free_frames() {
    for (int i = 0; i < NUM_OF_FRAMES; ++i) {
        if(frames_table[i]) {
            frames_table[i] = false;
            return i;
        }
    }
    cout<<" ERROR "<<endl;
    exit(0);
}
