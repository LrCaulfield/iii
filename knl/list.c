#include <stdio.h>
#include <stdlib.h>
#include "list.h"

void *nodeCreat(size_t size)
{
    struct Node *node = malloc(size);
    if (!node) return NULL;
    node->next = node->prev = node;
    return node;
}

void listIns(void *node_newP, void *nodeP)
{
    struct Node *node_new = node_newP,
                *node = nodeP;
    node_new->prev = node->prev;
    node_new->next = node;
    node->prev->next = node_new;
    node->prev = node_new;
}

void listRm(void *nodeP)
{
    struct Node *node = nodeP;
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void listExch(void *nodeP)
{
    struct Node *node = nodeP,
                *node_exch = node->next;
    listRm(node_exch);
    listIns(node_exch, node);
}
