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
#include "ctMisc.h"
#include "ctContext.h"

ctNode * ctNode_new(size_t i, ctContext* ctx)
{
	ctNode * n = (*(ctx->nodeAlloc))(ctx->cbData);
	n->i = i;
	n->up = NULL;
	n->down = NULL;
	n->children = ctBranchList_init();
	return n;
}

void ctNode_delete( ctNode * self, ctContext* ctx ) 
{ 
	(*(ctx->nodeFree))(self,ctx->cbData);
}

int ctNode_isMax( ctNode * self ) { return self->up == NULL; }
int ctNode_isMin( ctNode * self ) { return self->down == NULL; }
int ctNode_isLeaf( ctNode * self ) { return self->up == NULL || self->down == NULL; }
int ctNode_isRegular( ctNode * self ) { return self->up && self->up->nextUp == NULL && self->down && self->down->nextDown == NULL; }

ctArc * ctNode_leafArc( ctNode * self )
{
	assert( ctNode_isMax(self) || ctNode_isMin(self));
	return self->up == NULL ? self->down : self->up;
}

ctNode * ctNode_otherNode(ctNode * self)
{
	assert(ctNode_isMax(self) || ctNode_isMin(self));
	return self->up == NULL ? self->down->lo : self->up->hi;
}

void ctNode_addUpArc(ctNode * self, ctArc * a)
{
	a->prevUp = NULL;
	a->nextUp = self->up;
	if (self->up) self->up->prevUp = a;
	self->up = a;
}

void ctNode_addDownArc(ctNode * self, ctArc * a)
{
	a->prevDown = NULL;
	a->nextDown = self->down;
	if (self->down) self->down->prevDown = a;
	self->down = a;
}

void ctNode_removeUpArc(ctNode * self, ctArc * a)
{
	if (self->up == a) self->up = a->nextUp;
	if (a->nextUp) a->nextUp->prevUp = a->prevUp;
	if (a->prevUp) a->prevUp->nextUp = a->nextUp;
	a->nextUp = a->prevUp = NULL;
}

void ctNode_removeDownArc(ctNode * self, ctArc * a)
{
	if (self->down == a) self->down = a->nextDown;
	if (a->nextDown) a->nextDown->prevDown = a->prevDown;
	if (a->prevDown) a->prevDown->nextDown = a->nextDown;
	a->nextDown = a->prevDown = NULL;
}

ctNode * ctNode_prune(ctNode * self)
{
	if (ctNode_isMax(self)) {
		ctNode_removeUpArc(self->down->lo, self->down);
		return self->down->lo;
	} else if (ctNode_isMin(self)) {
		ctNode_removeDownArc(self->up->hi, self->up);
		return self->up->hi;
	} else {
		assert(FALSE);
	}
	return NULL;
}

ctArc * ctNode_collapse( ctNode * self, ctContext * ctx )
{
	assert( ctNode_isRegular(self) );
	
	if (ctx->mergeArcs)
		(*(ctx->mergeArcs))( self->up, self->down, ctx->cbData );

	ctBranchList_merge( &(self->up->children), &(self->down->children), ctx );
	ctBranchList_merge( &(self->up->children), &(self->children), ctx );
	
	ctNode_removeUpArc(self->down->lo, self->down);
	ctNode_addUpArc(self->down->lo, self->up);

	self->up->lo = self->down->lo;
	ctArc_union(self->down, self->up);

	return self->up;
}

