/*
Copyright (c) 2014, Martin Åberg All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The names of the copyright holder(s) may not be used to endorse or
   promote products derived from this software without specific prior
   written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stddef.h>
#include <string.h>
#include <martos/martos.h>

void node_unlink(Node *const node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void list_init(List *const list)
{
    list->head.next = &(list->tail);
    list->head.prev = NULL;
    list->tail.next = NULL;
    list->tail.prev = &(list->head);
}

bool list_is_empty(List *const list)
{
    if (&list->head == list->tail.prev) {
        return true;
    } else {
        return false;
    }
}

void list_add_tail(List *const list, Node *const node)
{
    node->next = (Node *) &list->tail.next;
    node->prev = (Node *) list->tail.prev;
    list->tail.prev->next = (MinNode *) node;
    list->tail.prev = (MinNode *) node;
}

Node *list_rem_head(List *const list)
{
    Node *node;

    node = list_get_head(list);

    if (NULL != node) {
        node_unlink(node);
    }
    return node;
}

Node *list_get_head(List *const list)
{
    if (true == list_is_empty(list)) {
        return NULL;
    } else {
        return (Node *) list->head.next;
    }
}

void list_enqueue(List *const list, Node *const node)
{
    Node *nextnode;

    nextnode = (Node *) list->head.next;
    while (NULL != nextnode->next) {
        if (nextnode->prio < node->prio) {
            break;
        }
        nextnode = nextnode->next;
    }
    node->next = nextnode;
    node->prev = nextnode->prev;
    nextnode->prev->next = node;
    nextnode->prev = node;
}

Node *list_find(List *const start, char *const name)
{
    Node *nextnode;

    nextnode = (Node *) start->head.next;
    while (NULL != nextnode->next) {
        if (0 == strcmp(nextnode->name, name)) {
            return nextnode;
        }
        nextnode = nextnode->next;
    }
    return NULL;
}

