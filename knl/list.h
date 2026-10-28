#pragma once

#include <stddef.h>

struct Node {
    struct Node *prev;
    struct Node *next;
};

void *nodeCreat(size_t size);
void nodeShow(void *nodeP);
void listIns(void *node_newP, void *nodeP);
void listRm(void *nodeP);
void listExch(void *nodeP);
