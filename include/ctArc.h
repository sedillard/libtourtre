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


#ifndef CT_ARC_H
#define CT_ARC_H


/**
\file ctArc.h

\brief Defines ctArc and related functions.

This file defines the ctArc structure, which works together with ctNode to form the contour tree
*/


#include "ctBranch.h"

struct ctNode; /* fwd decl */
struct ctContext; 

/**

\brief Arc of the contour tree

The ctArc structure is an arc of the contour tree. Together with ctNode, it forms the contour tree data structure.
It connects two ctNode objects, hi and lo. You can store extra information in the data field.
For example, you might want to keep track of which vertices map to this arc. This information can be obtained using
the \ref ct_vertexFunc callback. This will be called once for each vertex which maps to this arc. If you do store
information in the data field, you will also want to implement the \ref ct_arcMergeFunc callback, which should accumulate
the data stored in two merged arcs.

*/

typedef struct ctArc
{
	/** Node at the top of the arc */
	struct ctNode *hi;
	
	/** Node at the bottom of the arc */
	struct ctNode *lo;
	
	/** Next arc in the list of arcs attached to the lo node */
	struct ctArc *nextUp;
	
	/** Previous arc in the list of arcs attached to the lo node */
	struct ctArc *prevUp;
	
	/** Next arc in the list of arcs attached to the hi node */
	struct ctArc *nextDown;
	
	/** Previous arc in the list of arcs attached to the hi node */
	struct ctArc *prevDown;
	
	/** User data */
	void * data; /* user data */
	
        /** 
         * Branch that this arc becomes after decomposition. Only valid after
         * ct_decompose is called. */
	ctBranch * branch; 
	
        /** 
         * Temporary storage for branches that will be children of this
         * arc/branch. Don't use. Access through ctBranch instead.  */
	ctBranchList children;
	
	/** Union-find pointer. Don't touch. */
	struct ctArc * uf;
	

} ctArc;	


/** 
 * Allocate a new ctArc, that spans nodes h and l. The node is allocated
 * using the allocator specified by \ref ct_arcAllocator */
ctArc*  ctArc_new    ( struct ctNode * h, struct ctNode * l, struct ctContext * ctx );

/** 
 * Delete a ctArc structure using the deallocator specified by \ref
 * ct_arcAllocator*/
void  ctArc_delete ( ctArc * self, struct ctContext * ctx );

/** Union-find find */
ctArc*  ctArc_find   ( ctArc * self );

/** Union-find union */
void  ctArc_union  ( ctArc * a, ctArc * b );


#endif

