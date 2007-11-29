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


#ifndef CT_LEAFQUEUE_H
#define CT_LEAFQUEUE_H

#include "ctMisc.h"
#include "ctComponent.h"

struct ctContext;
struct ctComponent;
struct ctNode;

typedef struct ctLeafQ {

	struct ctComponent ** q;
	size_t head,tail,size;

} ctLeafQ;


    ctLeafQ*  ctLeafQ_new      ( size_t size );
        void  ctLeafQ_delete   ( ctLeafQ * self );
        void  ctLeafQ_pushBack ( ctLeafQ * self, struct ctComponent * c );
ctComponent*  ctLeafQ_popFront ( ctLeafQ * self );
         int  ctLeafQ_isEmpty    ( ctLeafQ * self );






typedef struct ctPriorityQ_Item {
	struct ctNode * n; /* leaf node */
	double p; /* priority */
	size_t o; /* vertex at the other end of leaf arc (used to check validity */
} ctPriorityQ_Item;


typedef struct ctPriorityQ {
	ctPriorityQ_Item * heap;
	size_t size, storage;
} ctPriorityQ;


ctPriorityQ*  ctPriorityQ_new    ( );
        void  ctPriorityQ_delete ( ctPriorityQ * self );
         int  ctPriorityQ_isEmpty  ( ctPriorityQ * self );
        
/* these are the modified priority q functions described in the Toporrery paper */
		void  ctPriorityQ_push   ( ctPriorityQ * self, struct ctNode * node, struct ctContext * ctx );
     ctNode*  ctPriorityQ_pop    ( ctPriorityQ * self, struct ctContext * ctx);

/* these are plain heap functions, called by the above functions */
ctPriorityQ_Item  ctPriorityQ_popHeap  ( ctPriorityQ * self );
            void  ctPriorityQ_pushHeap ( ctPriorityQ * self, ctPriorityQ_Item item );


#endif
