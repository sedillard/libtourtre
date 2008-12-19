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
#include "ctComponent.h"

#include <stdio.h>

ctComponent * ctComponent_new( ctComponentType type )
{
	ctComponent * c = (ctComponent*) malloc(sizeof(ctComponent));
	c->birth = c->death = c->last = CT_NIL;
	c->pred = c->succ = c->nextPred = c->prevPred = NULL;
	c->uf = c;
        c->type = type;
	return c;
}

void ctComponent_delete( ctComponent * self ) 
{
	free(self);
}
	
void ctComponent_addPred(ctComponent * self, ctComponent * c)
{
	c->prevPred = NULL;
	c->nextPred = self->pred;
	if (self->pred) self->pred->prevPred = c;
	self->pred = c;
}
	
void ctComponent_removePred(ctComponent * self, ctComponent * c)
{
	if (self->pred == c) self->pred = c->nextPred;
	if (c->nextPred) c->nextPred->prevPred = c->prevPred;
	if (c->prevPred) c->prevPred->nextPred = c->nextPred;
	c->nextPred = c->prevPred = NULL;
}


/* merges this with successor. this becomes the new, merged component. successor is returned. */
ctComponent * ctComponent_eatSuccessor( ctComponent * self )
{
	assert( self->succ && self->succ->pred == self );
	assert( self->nextPred == NULL );
	
	{
		ctComponent * s = self->succ;
		ctComponent * ss = s->succ;
		if (ss) {
			ctComponent_removePred(ss,s);
			ctComponent_addPred(ss,self);
		}
		self->death = s->death;
		self->succ = s->succ;
		s->succ = s->pred = NULL;
		s->nextPred = s->prevPred = NULL;
		ctComponent_union(self,self);
		
		return s;
	}
}
	
void ctComponent_prune( ctComponent * self )
{
	assert(self->pred == NULL);
	{
		if (self->succ) ctComponent_removePred(self->succ,self);
		self->succ = NULL;
	}
}

int ctComponent_isLeaf( ctComponent * self )
{ 
	return !self->pred; 
}

int ctComponent_isRegular( ctComponent * self )
{ 
	return self->pred && self->pred->nextPred == NULL; 
}


ctComponent * ctComponent_find( ctComponent * self )
{
	/* find parent */
	ctComponent * c = self->uf;
	while(c != c->uf) {
		c = c->uf;
	}
	/* do path compression */
	{
		ctComponent *s = self->uf,*t;
		while(s != c) { 
			t = s->uf;
			s->uf = c;
			s = t;
		}
		self->uf = c;
	}
	return c;
}
	
	
void ctComponent_union(ctComponent * a, ctComponent * b)
{
	a->uf = b->uf; 
}


