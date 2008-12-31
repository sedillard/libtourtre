#include "ctNodeMap.h"
#include "sglib.h"

#define CT_NODEMAP_COMP(X,Y) ((X)->key - (Y)->key)

#include <tourtre.h>
#include "ctQueue.h"

SGLIB_DEFINE_RBTREE_PROTOTYPES(ctNodeMap,left,right,color,CT_NODEMAP_COMP)

struct ctNodeMap
{
    size_t key;
    ctNodeMap *left;
    ctNodeMap *right;
    ctNode *node;
    char color;
};

SGLIB_DEFINE_RBTREE_FUNCTIONS(ctNodeMap,left,right,color,CT_NODEMAP_COMP)

void
ctNodeMap_insert( ctNodeMap **map, size_t key, ctNode *node )
{
    ctNodeMap *n = malloc(sizeof(ctNodeMap));
    n->left = n->right = 0;
    n->key = key;
    n->node = node;

    sglib_ctNodeMap_add(map,n);
}

ctNode*
ctNodeMap_find( ctNodeMap *map, size_t key )
{
    ctNodeMap k, *m;
    k.key = key;
    m = sglib_ctNodeMap_find_member(map,&k);
    return (m && m->key==key) ? m->node : 0;
}

void
ctNodeMap_delete( ctNodeMap *map ) 
{ 
    struct sglib_ctNodeMap_iterator it;
    ctNodeMap *i = sglib_ctNodeMap_it_init(&it,map);
    for (; i; i=sglib_ctNodeMap_it_next(&it) ) free(i); 
}


void 
ctNodeMap_push_leaves( ctNodeMap *map, ctPriorityQ *pq, struct ctContext* ctx )
{
    struct sglib_ctNodeMap_iterator it;
    ctNodeMap *i = sglib_ctNodeMap_it_init(&it,map);
    for (; i; i=sglib_ctNodeMap_it_next(&it)) {
        if (ctNode_isLeaf(i->node))
            ctPriorityQ_push(pq,i->node,ctx);
    }
}
