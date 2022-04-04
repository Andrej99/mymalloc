#pragma once
#define MAP_ANONYMOUS 0x20 

#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MIN_VELIKOST 2*4 //2x2 za velikost 2x2 za kazalca

void *mymalloc(size_t);
void myfree(void*);

typedef struct _glava{
    void *naslednji_segment;
   __uint16_t prosti;
}glava;
