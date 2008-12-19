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


#ifndef CT_BRANCH_H
#define CT_BRANCH_H

/**
\file ctBranch.h

\brief Defines ctBranch, ctBranchList and related functions.
	
This file defines the ctBranch structure, which forms the branch decomposition. It also defines ctBranchList.
*/

#include <stdlib.h> /* size_t */

struct ctBranch;
struct ctContext;

/**	\brief List of branches.

    Doubly-linked, null-terminated list of child branches. Iterate a list of
    children like this:

    \code
    for ( ctBranch* ch = branch->children.head; ch != NULL; ch = ch->nextChild ) ...
    \endcode
*/
typedef struct ctBranchList
{
    /** 
     * Start of list. To iterate, follow ctBranch nextChild and prevChild
     * pointers. 
     **/
    struct ctBranch *head;

} ctBranchList;


/** \brief Branch of the branch decomposition.

    This is a branch of the branch decomposition. It is the only object in
    the branch decomposition data structure.  Each branch stores a list of
    child branches which are (according to \ref ct_priorityFunc ) of less
    importance than their pairent.
 */
typedef struct ctBranch
{
	/** Extremal vertex. Could be a minimum or a maximum critical point. */
	size_t extremum;

	/** Saddle vertex. */
	size_t saddle;
	
	/** Parent branch, where saddle is attached. */
	struct ctBranch *parent;

	/** List of child branches. */
	ctBranchList children;
	
	/** Part of ctBranchList */
	struct ctBranch *nextChild;

	/** Part of ctBranchList */
	struct ctBranch *prevChild;

	/** User data */
	void * data;
	
} ctBranch;



/** Create an empty branch list */
ctBranchList  ctBranchList_init   ();

/** 
 * Add branch br to a branch list. br will be inserted in order of ascending
 * function value of the saddle. The order does \em not respect the total order
 * provided \ref ct_init  
 **/
void  ctBranchList_add    ( ctBranchList * self, ctBranch * br, struct ctContext * ctx );

/** Remove branch br from a branch list. */
void  ctBranchList_remove ( ctBranchList * self, ctBranch * br );

/** Merge two sorted branch lists. */
void  ctBranchList_merge  ( ctBranchList * self, ctBranchList * other, struct ctContext * ctx );

/** 
 * Allocate a new branch using the allocator specified by \ref
 * ct_branchAllocator 
 **/
ctBranch*  ctBranch_new    ( size_t extremum, size_t saddle, struct ctContext* ctx );

/** Delete a branch using the deallocator specified by \ref ct_branchAllocator */
void  ctBranch_delete ( ctBranch * self, struct ctContext* ctx );


#endif

