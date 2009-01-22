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
#include "ctQueue.h"
#include "ctContext.h"


#include <stdio.h>
#include <math.h>


/* simple FIFO leaf Q */

ctLeafQ* ctLeafQ_new ( size_t size )
{
    ctLeafQ * lq = (ctLeafQ*) malloc( sizeof(ctLeafQ) );
    
    size_t size2 = 16;
    while( size2 < size ) size2 *= 2;
    lq->q = (ctComponent**) calloc( size2, sizeof(ctComponent*) );
    lq->head = 0;
    lq->tail = 1;
    lq->size = size2;
    
    if (!lq->q) {
        fprintf(stderr,"ctLeafQ_new: alloc returned null\n");
    }
    
    return lq;
}

void ctLeafQ_delete ( ctLeafQ * self ) 
{
    free( self->q );
    free( self );
}

void ctLeafQ_pushBack ( ctLeafQ * self, ctComponent * c )
{
    
    if ( (self->tail+1)%self->size == self->head ) {
        ctLeafQ * newSelf = ctLeafQ_new( self->size * 2 );
        while( !ctLeafQ_isEmpty(self) ) {
            ctLeafQ_pushBack( newSelf, ctLeafQ_popFront( self ) );
        }
        free( self->q );
        self->q = newSelf->q;
        self->size = newSelf->size;
        self->head = newSelf->head;
        self->tail = newSelf->tail;
        free( newSelf );
    }

    self->q[self->tail] = c;
    self->tail = (self->tail+1)%self->size;
	
}

ctComponent* ctLeafQ_popFront ( ctLeafQ * self )
{
	self->head = (self->head+1)%self->size;
	return self->q[self->head];
}

int ctLeafQ_isEmpty ( ctLeafQ * self )
{
	return ((self->head+1)%self->size) == self->tail;
}










/* priority Q for branch simplifications, using array-based heap */

ctPriorityQ* ctPriorityQ_new ( )
{
	ctPriorityQ * pq = (ctPriorityQ*) malloc(sizeof(ctPriorityQ));
	pq->storage = 16;
	pq->heap = (ctPriorityQ_Item*) calloc( pq->storage, sizeof(ctPriorityQ_Item) );
	pq->size = 0;
	return pq;
}

void ctPriorityQ_delete ( ctPriorityQ * self )
{
	free( self->heap );
	free( self );
}

int ctPriorityQ_isEmpty ( ctPriorityQ * self )
{
	return self->size == 0;
}

void ctPriorityQ_pushHeap ( ctPriorityQ * self, ctPriorityQ_Item item )
{
    /* resize array if necessary */
    if (self->size == self->storage ) {
        ctPriorityQ_Item * newHeap;
        size_t i;

        self->storage *= 2;
        newHeap = (ctPriorityQ_Item*) calloc( self->storage, sizeof(ctPriorityQ_Item) );
        for ( i = 0; i < self->size; i++ ) newHeap[i] = self->heap[i];
        free(self->heap);
        self->heap = newHeap;
    }

    self->heap[ self->size++ ] = item;

    {   /* push heap */
        size_t c = self->size-1;
        while(1) {
            if ( c > 0 ) {
                size_t p = (c-1)/2;
                if ( self->heap[p].p > self->heap[c].p ) {
                    ctPriorityQ_Item tmp = self->heap[p];
                    self->heap[p] = self->heap[c];
                    self->heap[c] = tmp;
                    c = p;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }
}


void ctPriorityQ_push ( ctPriorityQ* self, ctNode* node, ctContext* ctx )
{
    ctPriorityQ_Item item;
    item.n = node;

    if (ctx->priority) {
        /* user defined priority */
        item.p = (*(ctx->priority))( node, ctx->cbData );
    } else {
        /* default: persistence */
        item.p =    
            fabs( (*(ctx->value))(node->i, ctx->cbData) 
                - (*ctx->value)(ctNode_otherNode(node)->i, ctx->cbData) );
    }

    item.o = ctNode_otherNode(node)->i;
    ctPriorityQ_pushHeap(self,item);
}


ctPriorityQ_Item ctPriorityQ_popHeap( ctPriorityQ * self )
{
    ctPriorityQ_Item r = self->heap[0];
    size_t p = 0;

    self->heap[0] = self->heap[--self->size];

    while(1) {
        size_t c = p*2+1;
        if ( c < self->size ) {
            if ( c+1 < self->size && self->heap[c+1].p < self->heap[c].p ) c = p*2+2;
            /* c is the min child */
            
            if ( self->heap[p].p > self->heap[c].p ) {
                ctPriorityQ_Item tmp = self->heap[p];
                self->heap[p] = self->heap[c];
                self->heap[c] = tmp;
                p = c;
            } else break;		
        } else break;
    }
    return r;
}

   
ctNode* ctPriorityQ_pop ( ctPriorityQ * self, ctContext * ctx )
{
    while(1) {
        assert( self->size != 0 );
        {
            ctPriorityQ_Item i = ctPriorityQ_popHeap(self);
            if ( ctNode_otherNode(i.n)->i != i.o ) {
                /* invalidated priority because of other simplifications */
                ctPriorityQ_push(self, i.n, ctx);
                continue;
            }
            /* didn't continue? must be valid */
            return i.n;
        }
    }
}




