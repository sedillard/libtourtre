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

/* internal functions */
ctComponent*  ct_sweep        ( size_t start, size_t end, int inc, ctComponent ** comps, size_t * next, ctContext * ctx );
        void  ct_augment      ( ctContext * ctx );
      ctArc*  ct_merge        ( ctContext * ctx );
        void  ct_checkContext ( ctContext * ctx );





ctContext * ct_init( 
    size_t  numVerts,
    size_t  *totalOrder, 
       int  (*less)( size_t a, size_t b, void* ),
    double  (*value)( size_t v, void* ),
    size_t  (*neighbors)( size_t v, size_t* nbrs, void* ),
     void*  data
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
    ctx->less = less;
    ctx->value = value;
    ctx->neighbors = neighbors;
    ctx->data = data;
    
    /* create working mem */
    ctx->numVerts = numVerts;
    ctx->joinRoot = NULL;
    ctx->splitRoot = NULL;
    
    ctx->joinComps = (ctComponent**) calloc( ctx->numVerts, sizeof(ctComponent*) );
    memset(ctx->joinComps,0x0,sizeof(ctComponent*) * ctx->numVerts);
    
    ctx->splitComps = (ctComponent**) calloc( ctx->numVerts, sizeof(ctComponent*) );
    memset(ctx->splitComps,0x0,sizeof(ctComponent*) * ctx->numVerts);

    ctx->nextJoin = calloc( sizeof(size_t), ctx->numVerts );
    memset(ctx->nextJoin,CT_NIL,sizeof(size_t)*ctx->numVerts );
    
    ctx->nextSplit = calloc( sizeof(size_t), ctx->numVerts );
    memset(ctx->nextSplit,CT_NIL,sizeof(size_t)*ctx->numVerts );

    ctx->arcMap = (ctArc**) calloc( ctx->numVerts, sizeof(ctArc*) );
    memset( (void*)ctx->arcMap, 0, sizeof(ctArc*)*ctx->numVerts );
    
    ctx->nodeMap = (ctNode**) calloc( ctx->numVerts, sizeof(ctNode*) );
    memset( (void*)ctx->nodeMap, 0, sizeof(ctNode*)*ctx->numVerts );
    
    ctx->branchMap = (ctBranch**) calloc( ctx->numVerts, sizeof(ctBranch*) );
    memset( (void*)ctx->branchMap, 0, sizeof(ctBranch*)*ctx->numVerts );
    
    return ctx;
}

void ct_cleanup( ctContext * ctx )
{
    if ( ctx->joinComps  ) free( ctx->joinComps );
    if ( ctx->splitComps ) free( ctx->splitComps );
    if ( ctx->nextJoin   ) free( ctx->nextJoin );
    if ( ctx->nextSplit  ) free( ctx->nextSplit );
    
    if ( ctx->arcMap != NULL )    free(ctx->arcMap);
    if ( ctx->branchMap != NULL ) free(ctx->branchMap);
    if ( ctx->nodeMap != NULL )   free(ctx->nodeMap);
}

ctArc * ct_sweepAndMerge( ctContext * ctx )
{
    ct_checkContext(ctx);
    {
        ctArc * ret;
        
        printf("join sweep\n");
        ctx->joinRoot  = ct_sweep( 0,ctx->numVerts,+1,    ctx->joinComps,  ctx->nextJoin, ctx  );
        printf("split sweep\n");
        ctx->splitRoot = ct_sweep( ctx->numVerts-1,-1,-1, ctx->splitComps, ctx->nextSplit, ctx );
        ct_augment( ctx );
        ret = ct_merge( ctx );
        
        return ret;
    }
}


ctComponent* 
ct_sweep
(   size_t start, 
    size_t end, 
    int inc, 
    ctComponent** comps, 
    size_t* next, 
    ctContext* ctx )
{
    ct_checkContext(ctx);
    {
        size_t itr = 0, i = 0, n;
        ctComponent * iComp;
        int numExtrema = 0;
        int numSaddles = 0;
    
        size_t * nbrs = (size_t*) calloc ( ctx->maxValence, sizeof(size_t) );
    
        for ( itr = start; itr != end; itr += inc ) {
            size_t numNbrs;
            int numNbrComps;
            
            i = ctx->totalOrder[itr];
            
            iComp = NULL;
            numNbrs = (*(ctx->neighbors))(i,nbrs,ctx->data);
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
                            ctComponent * newComp = ctComponent_new(ctx); 
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
                iComp = ctComponent_new();
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
    
    
void ct_augment( ctContext * ctx )
{
    ct_checkContext(ctx);
    {
        ctComponent ** joinComps = ctx->joinComps;
        ctComponent ** splitComps = ctx->splitComps;
    
        size_t itr;
        int addedToJoin = 0, addedToSplit = 0;
    
        for ( itr = 1; itr < ctx->numVerts-1; itr++ ) {
    
            size_t i = ctx->totalOrder[itr];
            ctComponent * joinComp = joinComps[i];
            ctComponent * splitComp = splitComps[i];
    
            if (joinComp->birth == i && splitComp->birth != i) {
    
                ctComponent * newComp = ctComponent_new();
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
    
                ctComponent * newComp = ctComponent_new();
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
    }
}




void ct_queueLeaves( ctLeafQ *lq, ctComponent *c_, ctComponent **map )
{
  size_t stack_mem_size = 1024, stack_size=1;
  ctComponent ** stack = (ctComponent**) malloc( stack_mem_size * sizeof(ctComponent*) );
  stack[0] = c_;
  while(stack_size) {
    ctComponent *c = stack[--stack_size];  
    map[c->birth] = c; /* the mapping will have changed in ct_augment */
    if (ctComponent_isLeaf(c)) {
      ctLeafQ_pushBack(lq,c);
    } else {
      ctComponent *pred = c->pred;
      while (pred->nextPred) pred = pred->nextPred;
      for (; pred != NULL; pred = pred->prevPred ) {
        if (stack_size >= stack_mem_size-1) {
          stack_mem_size *= 2;
          { ctComponent **new_stack = 
             (ctComponent**)realloc(stack,stack_mem_size*sizeof(ctComponent*));
            stack = new_stack;
          }
        }
        stack[stack_size++] = pred;
      }
    }
  }
  free(stack);
} 
    
ctArc * ct_merge( ctContext * ctx )
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
    
        ctComponent **joinMap = ctx->joinComps;
        ctComponent **splitMap = ctx->splitComps;
        
        ctArc ** arcMap = ctx->arcMap;
        ctNode ** nodeMap = ctx->nodeMap;
        ctLeafQ * leafQ = ctLeafQ_new(0);
    
        /* these are set to the above variables, depending of if the leaf is
         * from the join or split tree */
        ctComponent **map, **otherMap;
        size_t *next, *otherNext;
    
        /* these phantom components take care of some special cases */
        ctComponent * plusInf = ctComponent_new();
        ctComponent * minusInf = ctComponent_new();

        memset(joinMap , 0, sizeof(ctComponent*)*ctx->numVerts);
        memset(splitMap, 0, sizeof(ctComponent*)*ctx->numVerts);
            
        ctComponent_addPred( plusInf, joinRoot );
        plusInf->birth = joinRoot->death;
        joinRoot->succ = plusInf;
        joinMap[joinRoot->death] = plusInf;
    
        ctComponent_addPred( minusInf, splitRoot );
        minusInf->birth = splitRoot->death;
        splitRoot->succ = minusInf;
        splitMap[splitRoot->death] = minusInf;

        ct_queueLeaves(leafQ, joinRoot, joinMap);
        ct_queueLeaves(leafQ, splitRoot, splitMap);
                
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
                if ( (*(ctx->less))( leaf->birth,leaf->death,ctx->data ) ) { 
                    /* comp is join component */
                    map = joinMap;
                    otherMap = splitMap;
                    next = nextJoin;
                    otherNext = nextSplit;
                    
                    lo = nodeMap[leaf->birth];
                    hi = nodeMap[leaf->death];
                    
                    if (!lo) {
                        lo = ctNode_new(leaf->birth,ctx);
                        nodeMap[leaf->birth] = lo;
                    }
                    if (!hi) {
                        hi = ctNode_new(leaf->death,ctx);
                        nodeMap[leaf->death] = hi;
                    }
        
                } else { /* split component */
        
                    map = splitMap;
                    otherMap = joinMap;
                    next = nextSplit;
                    otherNext = nextJoin;
                    
                    hi = nodeMap[leaf->birth];
                    lo = nodeMap[leaf->death];
                    
                    if (!hi) {
                        hi = ctNode_new(leaf->birth,ctx);
                        nodeMap[leaf->birth] = hi;
                    }
                    if (!lo) {
                        lo = ctNode_new(leaf->death,ctx);
                        nodeMap[leaf->death] = lo;
                    }
                
                }
                
                /* create arc */
                arc = ctArc_new(hi,lo,ctx);
                ctNode_addDownArc(hi,arc);
                ctNode_addUpArc(lo,arc);
                
                /* printf("arc: %lu to %lu\n", hi->i, lo->i ); */
                
                { /* gather up points for new arc */
                    size_t c;
                    for( c = leaf->birth; c != leaf->death; c = next[c] ) {
                        if (arcMap[c] == NULL) {
                            arcMap[c] = arc;
                            if (ctx->procVertex) (*(ctx->procVertex))( c, arc, ctx->data );
                        }
                    }
                }
                
                {    /* remove leaf */
                    ctComponent *succ = leaf->succ;
                    ctComponent *other, *garbage;
    
                    ctComponent_prune( leaf );
            
                    /* remove leaf's counterpart in other tree */
                    other = otherMap[leaf->birth];
                    assert(other);
                    assert(ctComponent_isRegular(other)) ;
            
                    garbage = ctComponent_eatSuccessor(other->pred);
                    if (garbage != plusInf && garbage != minusInf) ctComponent_delete( garbage );
            
                    if ( ctComponent_isLeaf(succ) 
                       && ctComponent_isRegular( otherMap[succ->birth] ) )  
                    {
                        ctLeafQ_pushBack(leafQ, succ);
                    } else if (ctComponent_isRegular(succ) 
                              && ctComponent_isLeaf(otherMap[succ->birth])) 
                    {
                        ctComponent * osb = otherMap[succ->birth];
                        ctLeafQ_pushBack(leafQ, osb);
                    }
            
                    ctComponent_delete(leaf);
                }
            }
        }
    
        ctx->joinRoot = 0;
        ctx->splitRoot = 0;
        ctComponent_delete( plusInf );
        ctComponent_delete( minusInf );
        ctLeafQ_delete( leafQ );
        
        return arc;
    }
}
    
    
ctBranch * ct_decompose( ctContext * ctx )
{
    ct_checkContext(ctx);
    if ( ctx->arcMap == 0 ) {
        fprintf(stderr,"ct_decompose : ct_decompose was called after ct_arcMap.");
        return 0;
    }
    
    {
        ctPriorityQ * pq = ctPriorityQ_new();
        ctBranch * root = 0;
    
        { /* init priority q with the leaves */
            size_t i;
            for ( i = 0; i < ctx->numVerts; i++)
                if ( ctx->nodeMap[i] && ctNode_isLeaf( ctx->nodeMap[i] ) )
                    ctPriorityQ_push( pq, ctx->nodeMap[i], ctx );
        }
    
        while( 1 ) {
            ctNode * n = ctPriorityQ_pop(pq,ctx);
            
            if (ctNode_isLeaf(n) && ctNode_isLeaf(ctNode_otherNode(n))) { /* all done */
                
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
    
            {
                ctBranch * b = 0;
                ctNode * o = 0;
                int prunedMax = 0;
        
                if (ctNode_isMax(n)) {
                    if ( ctNode_leafArc(n)->nextUp == NULL && ctNode_leafArc(n)->prevUp == NULL) {
                        continue;
                    }
                    b = ctBranch_new( n->i, ctNode_otherNode(n)->i, ctx );
                    prunedMax = TRUE;
                } else if (ctNode_isMin(n)) {
                    if (ctNode_leafArc(n)->nextDown == NULL && ctNode_leafArc(n)->prevDown == NULL) {
                        continue;
                    }
                    b = ctBranch_new(n->i,ctNode_otherNode(n)->i, ctx);
                    prunedMax = FALSE;
                } else {
                    fprintf(stderr, "decompose() : arc was neither max nor min\n");
                }
        
                b->children = ctNode_leafArc(n)->children;
                ctNode_leafArc(n)->branch = b;
                {
                    ctBranch * bc;
                    for (bc = ctNode_leafArc(n)->children.head; bc != NULL; bc = bc->nextChild)
                        bc->parent = b;
                }
        
                o = ctNode_prune(n);
                ctBranchList_add(&(o->children),b, ctx);
        
                if (ctNode_isRegular(o)) {
                    ctArc * a = ctNode_collapse(o, ctx); /* lists get merged in here */
                    if (prunedMax) {
                        if (ctNode_isMin(a->lo)) ctPriorityQ_push(pq,a->lo,ctx);
                    } else {
                        if (ctNode_isMax(a->hi)) ctPriorityQ_push(pq,a->hi,ctx);
                    }
                }
            }
        }
    
        { /* create branch map */
            size_t i;
            ctx->branchMap = (ctBranch**) (ctx->joinComps); /* reuse this memory */
            for ( i = 0; i < ctx->numVerts; i++) {
                ctArc * a = ctx->arcMap[i];
                assert(a);
                ctx->branchMap[i] = ctArc_find(a)->branch;
            }
        }
        
        ctPriorityQ_delete(pq);
        
        free( ctx->arcMap );
        ctx->arcMap = 0;
        
        return root;
    }
}



void ct_checkContext ( ctContext * ctx ) 
{
    assert( ctx->numVerts > 0 );
    assert( ctx->totalOrder );
    assert( ctx->value );
    assert( ctx->neighbors );
    assert( ctx->less );
}


ctArc ** ct_arcMap(ctContext * ctx)
{
    ctArc ** map = ctx->arcMap;
    ctx->arcMap = 0;
    return map;
}

ctBranch ** ct_branchMap( ctContext * ctx )
{
    ctBranch ** map = ctx->branchMap;
    ctx->branchMap = 0;
        ctx->joinComps = 0; /* branchMap and joinComps share memory */
    return map;    
}


ctArc* ct_arcMalloc( void* data ) { return (ctArc*)malloc(sizeof(ctArc)); }
void ct_arcFree( ctArc* arc, void* data ) { free(arc); }

ctNode* ct_nodeMalloc( void* data ) { return (ctNode*)malloc(sizeof(ctNode)); }
void ct_nodeFree( ctNode* node, void* data ) { free(node); }

ctBranch* ct_branchMalloc( void* data ) { return (ctBranch*)malloc(sizeof(ctBranch)); }
void ct_branchFree( ctBranch* branch, void* data ) { free(branch); }


    

void ct_arcAllocator( ctContext * ctx, ctArc* (*allocArc)(void*), void (*freeArc)( ctArc*, void*) ) 
{
    ctx->arcAlloc = allocArc;
    ctx->arcFree = freeArc;
}

void ct_nodeAllocator( ctContext * ctx, ctNode* (*allocNode)(void*) , void (*freeNode)( ctNode*, void*) ) 
{
    ctx->nodeAlloc = allocNode;
    ctx->nodeFree = freeNode;
}

void ct_branchAllocator( ctContext * ctx, ctBranch* (*allocBranch)(void*), void (*freeBranch)( ctBranch*, void*) ) 
{
    ctx->branchAlloc = allocBranch;
    ctx->branchFree = freeBranch;
}



/**/
