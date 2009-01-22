#ifndef CT_NODEMAP_H
#define CT_NODEMAP_H

#include <stdlib.h>
struct ctNode;
struct ctPriorityQ;
struct ctContext;

typedef struct ctNodeMap ctNodeMap;

struct ctNode* ctNodeMap_find( ctNodeMap *m, size_t index );

void ctNodeMap_insert( ctNodeMap **m, size_t index, struct ctNode *node );

void ctNodeMap_delete( ctNodeMap* );

void ctNodeMap_push_leaves( struct ctNodeMap*, struct ctPriorityQ*, 
                            struct ctContext* );

#endif

