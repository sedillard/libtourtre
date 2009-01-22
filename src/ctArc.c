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
#include "ctContext.h"

ctArc * ctArc_new(ctNode * h, ctNode * l, ctContext * ctx)
{
	ctArc * a = (*(ctx->arcAlloc))(ctx->cbData);
	a->hi = h;
	a->lo = l;
	a->nextUp = a->nextDown = a->prevUp = a->prevDown = NULL;
	a->branch = NULL;
	a->children = ctBranchList_init();
	a->uf = a;
	a->data = NULL;
	return a;
}

void ctArc_delete( ctArc * a, ctContext * ctx )
{
	(*(ctx->arcFree))(a,ctx->cbData);
}
	
ctArc * ctArc_find( ctArc * self )
{
	/* find parent */
	ctArc * c = self->uf;
	while(c != c->uf) {
		c = c->uf;
	}

	{ /* do path compression */
		ctArc *s = self->uf,*t;
		while(s != c) { 
			t = s->uf;
			s->uf = c;
			s = t;
		}
		self->uf = c;
	}
	return c;
}
	
	
void ctArc_union(ctArc * a, ctArc * b)
{
	a->uf = b->uf; 
}
