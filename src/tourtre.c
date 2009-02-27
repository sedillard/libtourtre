/*
Copyright (c) 2006, Scott E. Dillard
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "tourtre.h"

#include <stdio.h>

#include "ctQueue.h"
#include "ctMisc.h"
#include "ctComponent.h"
#include "ctAlloc.h"
#include "ctContext.h"
#include "ctNodeMap.h"
#include "sglib.h"

/* local functions */
static
ctComponent* ct_sweep ( size_t start, 
                        size_t end, 
                        int inc, 
                        ctComponentType type, 
                        ctComponent *comps[],
                        size_t * next, 
                        ctContext * ctx );

static
void    
ct_augment ( ctContext * ctx );

static
ctArc* ct_merge ( ctContext * ctx );

static
void  
ct_checkContext ( ctContext * ctx );




ctContext* 
ct_init
(   size_t  numVerts,
    size_t  *totalOrder, 
    double  (*value)( size_t v, void* ),
    size_t  (*neighbors)( size_t v, size_t* nbrs, void* ),
     void*  cbData
)
{
    ctContext * ctx = (ctContext*) malloc(sizeof(ctContext));
    
    /* zero-out the struct */
    memset(ctx, 0, sizeof(ctContext) );
    
    /* set default values */
    ctx->maxValence = 256;
    ctx->arcAlloc = ct_arcMalloc;
    ctx->arcFree = ct_arcFree;
    ctx->nodeAlloc = ct_nodeMalloc;
    ctx->nodeFree = ct_nodeFree;
    ctx->branchAlloc = ct_branchMalloc;
    ctx->branchFree = ct_branchFree;
    
    /* get parameter values */
    ctx->numVerts = numVerts;
    ctx->totalOrder = totalOrder;
    ctx->value = value;
    ctx->neighbors = neighbors;
    ctx->cbData = cbData;
    
    /* create working mem */
    ctx->numVerts = numVerts;
    ctx->joinRoot = NULL;
    ctx->splitRoot = NULL;
   
    
    ctx->joinComps = calloc( sizeof(ctComponent*), ctx->numVerts );
    memset(ctx->joinComps,0x0,sizeof(ctComponent*)*ctx->numVerts );
    
    ctx->splitComps = calloc( sizeof(ctComponent*), ctx->numVerts );
    memset(ctx->splitComps,0x0,sizeof(ctComponent*)*ctx->numVerts );

    ctx->nextJoin = calloc( sizeof(size_t), ctx->numVerts );
    memset(ctx->nextJoin,CT_NIL,sizeof(size_t)*ctx->numVerts );
    
    ctx->nextSplit = calloc( sizeof(size_t), ctx->numVerts );
    memset(ctx->nextSplit,CT_NIL,sizeof(size_t)*ctx->numVerts );

    ctx->arcMap = 0;
    ctx->arcMapOwned = 1;
    ctx->nodeMap = 0;
    ctx->branchMapOwned = 1;
    ctx->branchMap = 0;
    ctx->tree = 0;
    
    return ctx;
}

void ct_cleanup( ctContext * ctx )
{
    if ( ctx->joinComps  ) free( ctx->joinComps );
    if ( ctx->splitComps ) free( ctx->splitComps );
    if ( ctx->nextJoin   ) free( ctx->nextJoin );
    if ( ctx->nextSplit  ) free( ctx->nextSplit );
    
    if ( ctx->arcMapOwned && ctx->arcMap != NULL ) free(ctx->arcMap);
    if ( ctx->branchMapOwned && ctx->branchMap != NULL ) free(ctx->branchMap);
    if ( ctx->nodeMap != NULL ) ctNodeMap_delete(ctx->nodeMap);

    if (ctx->tree) ct_deleteTree(ctx->tree,ctx); 
}


void ct_joinSweep( ctContext * ctx )
{
ct_checkContext(ctx);
{
    ctx->joinRoot = 
        ct_sweep( 0,ctx->numVerts,+1,
            CT_JOIN_COMPONENT, ctx->joinComps, ctx->nextJoin, ctx  );
}
}

void ct_splitSweep( ctContext * ctx )
{
ct_checkContext(ctx);
{
    ctx->splitRoot = 
        ct_sweep( ctx->numVerts-1,-1,-1, 
            CT_SPLIT_COMPONENT, ctx->splitComps, ctx->nextSplit, ctx );
}
}

ctArc * ct_mergeTrees( ctContext * ctx )
{
    assert(ctx->splitRoot && ctx->joinRoot && 
        "Did you call ct_mergeTrees without first calling \
        ct_joinSweep and ct_splitSweep?");

    ct_augment( ctx );
    return ctx->tree=ct_merge( ctx );
}


ctArc * ct_sweepAndMerge( ctContext * ctx )
{
ct_checkContext(ctx);
{
    ct_joinSweep(ctx);
    ct_splitSweep(ctx);
    ct_augment( ctx );
    return ctx->tree=ct_merge( ctx );
}
}



static
ctComponent* 
ct_sweep
(   size_t start, 
    size_t end, 
    int inc, 
    ctComponentType type,
    ctComponent *comps[],
    size_t* next, 

    ctContext* ctx )
{
ct_checkContext(ctx);
{
    size_t itr = 0, i = 0, n;
    ctComponent * iComp;
    int numExtrema = 0;
    int numSaddles = 0;
    size_t * nbrs = calloc ( ctx->maxValence, sizeof(size_t) );

    for ( itr = start; itr != end; itr += inc ) {
        size_t numNbrs;
        int numNbrComps;
        
        i = ctx->totalOrder[itr];
        
        iComp = NULL;
        numNbrs = (*(ctx->neighbors))(i,nbrs,ctx->cbData);
        numNbrComps = 0;
        for (n = 0; n < numNbrs; n++) {
            size_t j = nbrs[n];
            
            if ( comps[j] ) {
                ctComponent * jComp = ctComponent_find( comps[j] );

                if (iComp != jComp) {
                    if (numNbrComps == 0) {
                        numNbrComps++;
                        iComp = jComp;
                        comps[i] = iComp;
                        next[iComp->last] = i;
                    } else if (numNbrComps == 1) {
                        /* create new component */
                        ctComponent * newComp = ctComponent_new(type); 
                        newComp->birth = i;
                        ctComponent_addPred( newComp, iComp );
                        ctComponent_addPred( newComp, jComp );

                        /* finish the two existing components */
                        iComp->death = i;
                        iComp->succ = newComp;
                        ctComponent_union(iComp, newComp);

                        jComp->death = i;
                        jComp->succ = newComp;
                        ctComponent_union(jComp, newComp);

                        next[ jComp->last ] = i;

                        iComp = newComp;
                        comps[i] = newComp;
                        newComp->last = i;

                        numSaddles++;
                        numNbrComps++;
                        
                    } else {
                        /*finish existing arc */
                        jComp->death = i;
                        jComp->succ = iComp;
                        ctComponent_union(jComp,iComp);
                        ctComponent_addPred(iComp,jComp);
                        next[jComp->last] = i;
                    }
                }
            }
        } /*  for each neighbor */

        if (numNbrComps == 0) {
            /* this was a local maxima. create a new component */
            iComp = ctComponent_new(type);
            iComp->birth = i;
            comps[i] = iComp;
            iComp->last = i;
            numExtrema++;
        } else if (numNbrComps == 1) {
            /* this was a regular point. set last */
            iComp->last = i;
        }

    } /* for each vertex */

    /* tie off end */
    iComp = ctComponent_find( comps[i] );
    iComp->death = i;

    /* terminate path */
    next[i] = CT_NIL;

    free(nbrs);
    return iComp;
}
}



static
void 
ct_augment( ctContext * ctx )
{
ct_checkContext(ctx);
{
    ctComponent **joinComps = ctx->joinComps;
    ctComponent **splitComps = ctx->splitComps;
    size_t itr;

    int addedToJoin = 0, addedToSplit = 0;
  
    for ( itr = 1; itr < ctx->numVerts-1; itr++ ) {

        size_t i = ctx->totalOrder[itr];
        ctComponent * joinComp = joinComps[i];
        ctComponent * splitComp = splitComps[i];

        if (joinComp->birth == i && splitComp->birth != i) {
            
            ctComponent * newComp = ctComponent_new(CT_SPLIT_COMPONENT);
            newComp->birth = i;
            newComp->death = splitComp->death;
            splitComp->death = i;

            if (splitComp->succ) {
                ctComponent_removePred( splitComp->succ, splitComp );
                ctComponent_addPred( splitComp->succ, newComp );
            }

            newComp->succ = splitComp->succ;
            ctComponent_addPred(newComp, splitComp);
            splitComp->succ = newComp;

            if (splitComp == ctx->splitRoot) ctx->splitRoot = newComp;

            addedToSplit++;

        } else if ( splitComp->birth == i && joinComp->birth != i ) {

            ctComponent * newComp = ctComponent_new(CT_JOIN_COMPONENT);
            newComp->death = i;
            newComp->birth = joinComp->birth;
            joinComp->birth = i;

            while( joinComp->pred ) {
                ctComponent * p = joinComp->pred;
                ctComponent_removePred(joinComp,p);
                ctComponent_addPred(newComp,p);
                p->succ = newComp;
            }

            ctComponent_addPred(joinComp,newComp);
            newComp->succ = joinComp;

            addedToJoin++;
        }

    }
    free(joinComps);
    free(splitComps);
    ctx->joinComps = ctx->splitComps = 0;
}
}


typedef 
struct ComponentMap
{
    ctComponent **map;
    size_t size;
} ComponentMap;


#define CT_COMPONENT_COMPARE1(X,Y) ((X)->birth - Y)
#define CT_COMPONENT_COMPARE(X,Y) ((X)->birth==(Y)->birth ? 0 : (X)->birth < (Y)->birth ? -1 : 1)

static
ctComponent*
ComponentMap_find ( ComponentMap *m, size_t i )
{
    int found;
    size_t ind = 0;
    
    SGLIB_ARRAY_BINARY_SEARCH(
        ctComponent*, m->map, /*what*/
        0,m->size, /*where*/
        i, /*who*/
        CT_COMPONENT_COMPARE1, /*how*/
        found,ind); /*output*/
    assert(found);
    return m->map[ind];
}


static
void
ct_queueLeaves( ctLeafQ *lq, ctComponent *c_, ComponentMap *map )
{
    size_t list_mem_size=256, list_size=0;
    ctComponent**list = malloc(list_mem_size*sizeof(ctComponent*));

    size_t stack_mem_size = 1024, stack_size=1;
    ctComponent **stack = (ctComponent**) malloc( stack_mem_size * sizeof(ctComponent*) );
    stack[0] = c_;
     
    while(stack_size) {
        ctComponent *c = stack[--stack_size];  

        /* add to list */
        list[list_size++] = c;
        if (list_size == list_mem_size) {
            list_mem_size *= 2;
            list = realloc(list,list_mem_size*sizeof(ctComponent*));
        }

        if (ctComponent_isLeaf(c)) {
            ctLeafQ_pushBack(lq,c);
        } else {
            ctComponent *pred = c->pred;
            while (pred->nextPred) pred = pred->nextPred;
            for (; pred != NULL; pred = pred->prevPred ) {
                stack[stack_size++] = pred; 
                if (stack_size == stack_mem_size) {
                    stack_mem_size *= 2;
                    stack = realloc(stack,stack_mem_size*sizeof(ctComponent*));
                } 
            }
        }
    }

    SGLIB_ARRAY_SINGLE_QUICK_SORT(ctComponent*,list,list_size,CT_COMPONENT_COMPARE);
    {   size_t i=0; 
        for (i=0; i<list_size-1; ++i) assert(list[i]->birth <= list[i+1]->birth);
    }
    

    free(stack);
    map->map = list;
    map->size = list_size;
}









static
ctArc * 
ct_merge( ctContext * ctx )
{
ct_checkContext(ctx);
{
    /* these are temp variables used in the loop */
    ctNode * hi = NULL;
    ctNode * lo = NULL;
    ctArc * arc = NULL;

    /* save some keystrokes on ctx-> */
    ctComponent *joinRoot = ctx->joinRoot;
    ctComponent *splitRoot = ctx->splitRoot;
    size_t *nextJoin = ctx->nextJoin;
    size_t *nextSplit = ctx->nextSplit;
    ComponentMap joinMap,splitMap;
    ctArc ** arcMap;
    ctLeafQ * leafQ = ctLeafQ_new(0);

    /* these are set to the above variables, depending of if the leaf is from
     * the join or split tree */
    ComponentMap  *otherMap;
    size_t *next; 

    /* these phantom components take care of some special cases */
    ctComponent * plusInf = ctComponent_new(CT_JOIN_COMPONENT);
    ctComponent * minusInf = ctComponent_new(CT_SPLIT_COMPONENT);

    ctComponent_addPred( plusInf, joinRoot );
    plusInf->birth = joinRoot->death;
    joinRoot->succ = plusInf;

    ctComponent_addPred( minusInf, splitRoot );
    minusInf->birth = splitRoot->death;
    splitRoot->succ = minusInf;

    ct_queueLeaves(leafQ, plusInf,  &joinMap);
    ct_queueLeaves(leafQ, minusInf, &splitMap);

    arcMap = ctx->arcMap = (ctArc**) calloc( ctx->numVerts, sizeof(ctArc*) );
    memset( (void*)ctx->arcMap, 0, sizeof(ctArc*)*ctx->numVerts );

    while(1) {
        assert(! ctLeafQ_isEmpty(leafQ) );
        {
            /* pop leaf from q */
            ctComponent * leaf = ctLeafQ_popFront(leafQ);

            if (leaf->death == CT_NIL) { /* all done */
                arcMap[ leaf->birth ] = arc;
                break;
            }
    
            /* which tree is this comp from? */
            if ( leaf->type == CT_JOIN_COMPONENT ) {
                /* comp is join component */
                otherMap = &splitMap;
                
                next = nextJoin;
                lo = ctNodeMap_find(ctx->nodeMap,leaf->birth);
                hi = ctNodeMap_find(ctx->nodeMap,leaf->death);
                if (!lo) {
                    lo = ctNode_new(leaf->birth,ctx);
                    ctNodeMap_insert(&ctx->nodeMap,leaf->birth,lo);
                }
                if (!hi) {
                    hi = ctNode_new(leaf->death,ctx);
                    ctNodeMap_insert(&ctx->nodeMap,leaf->death,hi);
                }
            } else { /* split component */
                otherMap = &joinMap;

                next = nextSplit;
                hi = ctNodeMap_find(ctx->nodeMap,leaf->birth);
                lo = ctNodeMap_find(ctx->nodeMap,leaf->death);
                if (!hi) {
                    hi = ctNode_new(leaf->birth,ctx);
                    ctNodeMap_insert(&ctx->nodeMap,leaf->birth,hi);
                }
                if (!lo) {
                    lo = ctNode_new(leaf->death,ctx);
                    ctNodeMap_insert(&ctx->nodeMap,leaf->death,lo);
                }
            }
            
            /* create arc */
            arc = ctArc_new(hi,lo,ctx);
            ctNode_addDownArc(hi,arc);
            ctNode_addUpArc(lo,arc);
            
            { /* gather up points for new arc */
                size_t c;
                for( c = leaf->birth; c != leaf->death; c = next[c] ) {
                    if (arcMap[c] == NULL) {
                        arcMap[c] = arc;
                        if (ctx->procVertex) (*(ctx->procVertex))( c, arc, ctx->cbData );
                    }
                }
            }
            
            {    /* remove leaf */
                ctComponent *succ = leaf->succ;
                ctComponent *other, *otherSucc, *garbage;

                ctComponent_prune( leaf );
        
                /* remove leaf's counterpart in other tree */
                other = ComponentMap_find( otherMap, leaf->birth );
                otherSucc = ComponentMap_find(otherMap,succ->birth);

                assert(other);
                assert(ctComponent_isRegular(other)) ;
        
                garbage = ctComponent_eatSuccessor(other->pred);

                /*
                if (garbage != plusInf && garbage != minusInf) 
                    ctComponent_delete( garbage );
                */

                if ( ctComponent_isLeaf(succ) && ctComponent_isRegular(otherSucc) )  {
                    ctLeafQ_pushBack(leafQ, succ);
                } else if (ctComponent_isRegular(succ) && ctComponent_isLeaf(otherSucc)) {
                    ctLeafQ_pushBack(leafQ, otherSucc);
                }
        
                /*ctComponent_delete(leaf);*/
            }
        }
    }

    ctx->joinRoot = 0;
    ctx->splitRoot = 0;
  
    {   size_t i;
        for (i=0; i<joinMap.size; ++i) free(joinMap.map[i]);
        for (i=0; i<splitMap.size; ++i) free(splitMap.map[i]);
    }
        
    /* 
    ctComponent_delete( plusInf );
    ctComponent_delete( minusInf );
    */
    ctLeafQ_delete( leafQ );

    free( joinMap.map );
    free( splitMap.map );

    return arc;
}
}
    

    
ctBranch * 
ct_decompose( ctContext * ctx )
{
ct_checkContext(ctx);
if ( ctx->arcMap == 0 ) {
    fprintf(stderr,"ct_decompose : ct_decompose was called after ct_arcMap.");
    return 0;
}

{
    ctBranch * root = 0;
    ctPriorityQ * pq = ctPriorityQ_new();
    ctNodeMap_push_leaves(ctx->nodeMap,pq,ctx);

    for(;;) {
        ctNode * n = ctPriorityQ_pop(pq,ctx);
        
        if (ctNode_isLeaf(n) && ctNode_isLeaf(ctNode_otherNode(n))) { 
            /* all done */
            root = ctBranch_new( ctNode_leafArc(n)->hi->i, ctNode_leafArc(n)->lo->i, ctx );
            root->children = ctNode_leafArc(n)->children;
            ctNode_leafArc(n)->branch = root;
            {
                ctBranch * bc;
                for (bc = root->children.head; bc != NULL; bc = bc->nextChild)
                    bc->parent = root;
            }
            break; /* exit */
        }
        {   ctBranch * b = 0;
            ctNode * o = 0;
            int prunedMax = 0;
    
            if (ctNode_isMax(n)) {
                if ( ctNode_leafArc(n)->nextUp == NULL && 
                     ctNode_leafArc(n)->prevUp == NULL ) 
                {
                    continue;
                }
                b = ctBranch_new( n->i, ctNode_otherNode(n)->i, ctx );
                prunedMax = TRUE;
            } else if (ctNode_isMin(n)) {
                if ( ctNode_leafArc(n)->nextDown == NULL && 
                     ctNode_leafArc(n)->prevDown == NULL ) 
                {
                    continue;
                }
                b = ctBranch_new(n->i,ctNode_otherNode(n)->i, ctx);
                prunedMax = FALSE;
            } else {
                fprintf(stderr,"decompose() : arc was neither max nor min\n");
            }
    
            b->children = ctNode_leafArc(n)->children;
            ctNode_leafArc(n)->branch = b;
            {
                ctBranch * bc;
                for (bc=ctNode_leafArc(n)->children.head; 
                     bc!=NULL; bc=bc->nextChild)
                {
                    bc->parent = b;
                }
            }
    
            o = ctNode_prune(n);
            ctBranchList_add(&(o->children),b, ctx);
    
            if (ctNode_isRegular(o)) {
                ctArc * a = ctNode_collapse(o, ctx); /*lists get merged here*/
                if (prunedMax) {
                    if (ctNode_isMin(a->lo)) ctPriorityQ_push(pq,a->lo,ctx);
                } else {
                    if (ctNode_isMax(a->hi)) ctPriorityQ_push(pq,a->hi,ctx);
                }
            }
        }
    }

    {   /* create branch map */
        size_t i;
        ctx->branchMap = (ctBranch**)calloc(ctx->numVerts,sizeof(ctBranch*));
        memset( (void*)ctx->branchMap, 0, sizeof(ctBranch*)*ctx->numVerts );
        for ( i = 0; i < ctx->numVerts; i++) {
            ctArc * a = ctx->arcMap[i];
            assert(a);
            ctx->branchMap[i] = ctArc_find(a)->branch;
        }
    }
    
    ctPriorityQ_delete(pq);
    ctx->tree = 0;
    return root;
}
}



static
void 
ct_checkContext ( ctContext * ctx ) 
{
    assert( ctx->numVerts > 0 );
    assert( ctx->totalOrder );
    assert( ctx->value );
    assert( ctx->neighbors );
}


ctArc ** 
ct_arcMap(ctContext * ctx)
{
    ctx->arcMapOwned = 0;
    return ctx->arcMap;
}

ctBranch ** 
ct_branchMap( ctContext * ctx )
{
    ctx->branchMapOwned = 0;
    return ctx->branchMap;
}


ctArc* ct_arcMalloc( void* cbData ) { return (ctArc*)malloc(sizeof(ctArc)); }
void ct_arcFree( ctArc* arc, void* cbData ) { free(arc); }

ctNode* ct_nodeMalloc( void* cbData ) { return (ctNode*)malloc(sizeof(ctNode)); }
void ct_nodeFree( ctNode* node, void* cbData ) { free(node); }

ctBranch* ct_branchMalloc( void* cbData ) { return (ctBranch*)malloc(sizeof(ctBranch)); }
void ct_branchFree( ctBranch* branch, void* cbData ) { free(branch); }


    

void 
ct_arcAllocator
(   ctContext * ctx, 
    ctArc* (*allocArc)(void*), 
    void (*freeArc)( ctArc*, void*) ) 
{
    ctx->arcAlloc = allocArc;
    ctx->arcFree = freeArc;
}

void 
ct_nodeAllocator
(   ctContext * ctx, 
    ctNode* (*allocNode)(void*) , 
    void (*freeNode)( ctNode*, void*) ) 
{
    ctx->nodeAlloc = allocNode;
    ctx->nodeFree = freeNode;
}

void 
ct_branchAllocator
(   ctContext * ctx, 
    ctBranch* (*allocBranch)(void*), 
    void (*freeBranch)( ctBranch*, void*) ) 
{
    ctx->branchAlloc = allocBranch;
    ctx->branchFree = freeBranch;
}





/* Sorry excuse for a contour tree iterator.
 */
typedef struct NodePair 
{ 
    ctNode *node; /* The current node */
    ctNode *prev; /* The node we came from. Don't go back there */
} NodePair;


static
NodePair* pushNodePair
(   NodePair **stack, 
    size_t *stack_size, 
    size_t *stack_cap )
{
    if (*stack_size == *stack_cap) 
        *stack = (NodePair*) realloc(
            *stack, sizeof(NodePair) * ( (*stack_cap) *= 2 ) );
    return (*stack) + (++(*stack_size) - 1);
}


ctArc*
ct_copyTree( ctArc *src, int moveData, ctContext *ctx )
{
    ctNode *start = src->lo;
    ctNodeMap *nodeMap=0;
    ctArc *a=0; /* arc iterator in for loops */
    ctArc *anArc=0; /* will return an arc of the new tree */

    size_t stack_cap = 256, stack_size;
    NodePair *stack = (NodePair*)malloc(sizeof(NodePair)*stack_cap);

    stack_size=1; 
    stack[0].node = start;
    stack[0].prev = 0;

    /* build map form vertex ids to new nodes */
    while (stack_size>0) {
        NodePair *np = stack + (--stack_size);
        ctNode *n = np->node, *p = np->prev;
        ctNode *newNode = ctNode_new(n->i,ctx);

        assert(!n->up   || n->up->lo == n); 
        assert(!n->down || n->down->hi == n);
        ctNodeMap_insert(&nodeMap,n->i,newNode); 
        if (moveData) {
            newNode->data=n->data; 
            n->data=newNode;
        }

        for (a=n->up; a!=NULL; a=a->nextUp) {
            if (a->hi != p) {
                NodePair *top = pushNodePair(&stack,&stack_size,&stack_cap);
                top->node = a->hi;
                top->prev = n;
            }
        }
        for (a=n->down; a!=NULL; a=a->nextDown) {
            if (a->lo != p) {
                NodePair *top = pushNodePair(&stack,&stack_size,&stack_cap);
                top->node = a->lo;
                top->prev = n;
            }
        }
    }

    stack_size=1; 
    stack[0].node = start;
    stack[0].prev = 0;

    /* then create new arcs between the new nodes */
    while (stack_size>0) {
        NodePair *np = &stack[--stack_size];
        ctNode *n = np->node, *p = np->prev;
       
        for (a=n->up; a!=NULL; a=a->nextUp) {
            if (a->hi != p) {
                NodePair *top = pushNodePair(&stack,&stack_size,&stack_cap);
                top->node = a->hi;
                top->prev = n;

                {   ctNode *newLo = ctNodeMap_find(nodeMap,n->i),
                           *newHi = ctNodeMap_find(nodeMap,a->hi->i);
                    ctArc *newArc = ctArc_new(newHi,newLo,ctx);
                    ctNode_addUpArc(newLo,newArc);
                    ctNode_addDownArc(newHi,newArc);
                    anArc = newArc;
                    if (moveData) {
                        newArc->data=a->data; 
                        a->data=newArc;
                    }
                }
            }
        }
        for (a=n->down; a!=NULL; a=a->nextDown) {
            if (a->lo != p) {
                NodePair *top = pushNodePair(&stack,&stack_size,&stack_cap);
                top->node = a->lo;
                top->prev = n;

                {   ctNode *newHi = ctNodeMap_find(nodeMap,n->i),
                           *newLo = ctNodeMap_find(nodeMap,a->lo->i);
                    ctArc *newArc = ctArc_new(newHi,newLo,ctx);
                    ctNode_addUpArc(newLo,newArc);
                    ctNode_addDownArc(newHi,newArc);
                    anArc = newArc;
                    if (moveData) {
                        newArc->data=a->data; 
                        a->data=newArc;
                    }
                }
            }
        }
    }
   
    ctNodeMap_delete(nodeMap);
    return anArc;
}


void
ct_arcsAndNodes
(   ctArc *src, 
    ctArc ***arcsOut, 
    size_t *numArcsOut,
    ctNode ***nodesOut,
    size_t *numNodesOut )
{
    ctNode *start = src->lo;
    ctArc *a; /* arc iterator in for loops */

    size_t arcs_size=0, nodes_size=0, arcs_cap=256, nodes_cap=256;
    ctArc **arcs = (ctArc**)malloc(arcs_cap*sizeof(ctArc*));
    ctNode **nodes = (ctNode**)malloc(nodes_cap*sizeof(ctNode*));

    size_t stack_cap = 256, stack_size;
    NodePair *stack = (NodePair*)malloc(sizeof(NodePair)*stack_cap);

    stack_size=1; 
    stack[0].node = start;
    stack[0].prev = 0;

    while (stack_size>0) {
        NodePair *np = &stack[--stack_size];
        ctNode *n = np->node, *p = np->prev;
       
        nodes[nodes_size++] = n;
        if (nodes_size == nodes_cap)
            nodes = (ctNode**)realloc(nodes,sizeof(ctNode*)*(nodes_cap*=2));

        for (a=n->up; a!=NULL; a=a->nextUp) {
            if (a->hi != p) {
                NodePair *top = pushNodePair(&stack,&stack_size,&stack_cap);
                top->node = a->hi;
                top->prev = n;

                arcs[arcs_size++] = a;
                if (arcs_size == arcs_cap)
                    arcs = (ctArc**)realloc(arcs,sizeof(ctArc*)*(arcs_cap*=2));
            }
        }
        for (a=n->down; a!=NULL; a=a->nextDown) {
            if (a->lo != p) {
                NodePair *top = pushNodePair(&stack,&stack_size,&stack_cap);
                top->node = a->lo;
                top->prev = n;

                arcs[arcs_size++] = a;
                if (arcs_size == arcs_cap)
                    arcs = (ctArc**)realloc(arcs,sizeof(ctArc*)*(arcs_cap*=2));
            }
        }
    }
    *arcsOut = (ctArc**)realloc(arcs,sizeof(ctArc*)*arcs_size);
    *numArcsOut = arcs_size;
    *nodesOut = (ctNode**)realloc(nodes,sizeof(ctNode*)*nodes_size);
    *numNodesOut = nodes_size;
}

void
ct_deleteTree( ctArc *a, ctContext *ctx )
{
    size_t narcs,nnodes,i;
    ctArc **arcs;
    ctNode **nodes;
    ct_arcsAndNodes(a,&arcs,&narcs,&nodes,&nnodes);
    for (i=0; i<narcs; ++i) ctArc_delete(arcs[i],ctx);
    for (i=0; i<nnodes; ++i) ctNode_delete(nodes[i],ctx);
    free(arcs);
    free(nodes);
}


void 
ct_priorityFunc( ctContext *ctx, double (*priorityFunc)(ctNode*,void*) )
{
    ctx->priority = priorityFunc;
}

/**/
