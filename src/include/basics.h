#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



typedef void(*nl_deleter_t)(void* object);

#define NL_PANIC(MSG, FILE_NAME, LINE_NO)\
do {\
    fprintf(stderr, "Panic ~ %s, %d: %s\n", FILE_NAME, LINE_NO, MSG);\
    exit(1);\
} while (0);\

#define NL_ALLOC(DATA_TYPE, PTR_NAME, FILE_NAME, LINE_NO)\
DATA_TYPE* PTR_NAME = malloc(sizeof(DATA_TYPE));\
if (!PTR_NAME) {\
    NL_PANIC("Could not allocate object here.", FILE_NAME, LINE_NO)\
}\
\

#define NL_ALLOC_N(N, DATA_TYPE, PTR_NAME, FILE_NAME, LINE_NO)\
DATA_TYPE* PTR_NAME = calloc((size_t)N, sizeof(DATA_TYPE));\
if (!PTR_NAME) {\
    NL_PANIC("Could not allocate multiple object buffer here.", FILE_NAME, LINE_NO)\
}\
\
