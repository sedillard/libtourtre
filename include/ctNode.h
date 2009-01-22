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


#ifndef CT_NODE_H
#define CT_NODE_H

/**
\file ctNode.h

\brief Defines ctNode and related functions.
	
This file defines the ctNode structure and functions to manipulate the contour tree.
*/


#include <stdlib.h> /* size_t */
#include "ctBranch.h"

struct ctArc;
struct ctContext;

/**

\brief Nodes of the contour tree
    
    This is a critical point in the contour tree. Together with ctArc, this forms the contour tree data structure.
    Nodes serve as connection points for arcs. You can store information in their data field, but you probably want
    to pay more attention to the arcs */

typedef struct ctNode 
{
	/** Critical vertex that this node represents. */
	size_t i;

	/** Doubly-linked, null-terminated list of arcs extended upwards from this node. Iterate like this:

		for (ctArc * a = node->up; a != NULL; a = a->nextUp )...
	*/
	struct ctArc *up; 

	/** Doubly-linked, null-terminated list of arcs extended upwards from this node. Iterate like this:

		for (ctArc * a = node->down; a != NULL; a = a->nextDown )...
	*/
	struct ctArc *down;

	/** \internal Temporary storage for child branches. Used by ct_decompose. Don't access this. */
	ctBranchList children;

	/** User data */
	void * data;
} ctNode;

/** Allocate a new node using the allocator specified by \ref ct_nodeAllocator */
ctNode*  ctNode_new           ( size_t i, struct ctContext* ctx );

/** Delete a node using the deallocator specified by \ref ct_nodeAllocator*/
   void  ctNode_delete        ( ctNode* self, struct ctContext* ctx );

/** Is this node a max? (checks for a null up pointer.) */
    int  ctNode_isMax         ( ctNode* self );

/** Is this node a max? (checks for a null down pointer.) */
    int  ctNode_isMin         ( ctNode* self );
    
/** isMax or isMin  */
    int  ctNode_isLeaf        ( ctNode* self );

/** True if there is exactly one upward arc and one downward arc. False else. */
    int  ctNode_isRegular     ( ctNode* self );

/** If this node is a leaf, this will retrieve the attached arc */
 ctArc*  ctNode_leafArc       ( ctNode* self );

/** If this node is a leaf, this will retrieve the node at the other end of the arc */
ctNode*  ctNode_otherNode     ( ctNode* self );

/** Add an arc to the list of upward arcs */
   void  ctNode_addUpArc      ( ctNode* self, ctArc * a );

/** Add an arc to the list of downward arcs */   
   void  ctNode_addDownArc    ( ctNode* self, ctArc * a );

/** Remove an arc from the list of upward arcs */   
   void  ctNode_removeUpArc   ( ctNode* self, ctArc * a );

/** Remove an arc from the list of downward arcs */
   void  ctNode_removeDownArc ( ctNode* self, ctArc * a );

/** Self is a leaf. Remove it's leafArc from the tree. This does not deallocate self or the leafArc. Returns the otherNode.  */
ctNode*  ctNode_prune         ( ctNode* self );

/** Collapse a regular node. Returns the new, larger arc. Call ctContext.mergeArcs */
 ctArc*  ctNode_collapse      ( ctNode* self, struct ctContext * ctx );


#endif

