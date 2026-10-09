#pragma once
#include "drawing_program/drawing_program_control.h"
#include <pthread.h>
#include <sys/types.h>
#define SKETCH_CONTROL_MAX_REQUEST 16384
#define SKETCH_CONTROL_RECEIPTS 128
typedef struct SketchReceipt { char id[65]; char *request; char *response; } SketchReceipt;
struct SketchControl {
    int listener; pthread_t thread; pthread_mutex_t mutex; pthread_cond_t ready;
    bool stopping, pending, allow_write; char *request, *response;
    char path[104], instance[33], output_root[512]; dev_t device; ino_t inode;
    SketchReceipt receipts[SKETCH_CONTROL_RECEIPTS]; int receipt_count;
};
