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

#ifndef TOURTRE_H
#define TOURTRE_H

/**
\file tourtre.h
\brief Include this file in your program.
*/


#include <stdlib.h> /* size_t */

#include "ctArc.h"
#include "ctBranch.h"
#include "ctNode.h"


/** \brief Holds all the data.
*/
typedef struct ctContext ctContext; 



/** 
Call this first. This function provides the needed data and callbacks to the
library, so that it can compute the contour tree of your...whatever it is. This
also allocates the working memory for the library. The corresponding
deallocation function is ct_cleanup 

@param numVertices     Number of vertices in the mesh

@param totalOrder      Array of vertex indexes, in sorted order.

@param value           Callback to get function value of a vertex v. Used only
                       for estimating persistence of arcs. 

@param neighbors       Callback to get the neighboring vertices of vertex v.
                       This function should store the indexes of the neighbors
                       in the array nbrs. The size of this array is 256 by
                       default, but this can be increased by calling
                       ct_maxValence.

@param data            User data passed to all callbacks.
*/

ctContext * ct_init( 
    size_t  numVertices,
    size_t  *totalOrder, 
    double  (*value)( size_t v, void* ),
    size_t  (*neighbors)( size_t v, size_t* nbrs, void* ),
    void*  data
);










/** 
 * Informs the library what the maximum valence of a vertex will be. Default
 * is 256. Feel free to make this large, as an array of this size is only
 * allocated once. 
 **/
void ct_maxValence( ctContext * ctx, size_t max ); 

/** 
 * Process a vertex in the sweepAndMerge algorithm. This is called when the
 * arc belonging to v is added to the contour tree. One use for this might be
 * to estimate the area/volume of an arc.  If your vertices are evenly spaced,
 * then you can keep an accumulator in ctArc.data and add to it using this
 * function.  This function may not be called for every arc, but when it is
 * called, you can use the vertex v as a <a
 * href="http://citeseer.ist.psu.edu/bajaj98contour.html">path seed</a> for
 * arc a.  It will be called many times for the "big, important arcs." 
 **/  
void ct_vertexFunc( ctContext * ctx, void (*vertexFunc)( size_t v, ctArc* a, void* ) );


/** 
 * This is called when two arcs are merged by simplification. If you are
 * keeping track of arc properties using ct_vertexFunc, then you will want to
 * sum up those properties using this function. The first argument is the arc
 * which will remain after the merge, the second argument is the one which
 * will be deleted. 
 **/
void ct_arcMergeFunc( ctContext * ctx, void (*arcMergeFunc)( ctArc* a, ctArc* b, void* ) );



/** 
 * Define the simplification priority of an arc. The function is passed a leaf
 * node. Use ctNode_leafArc() to access the leaf arc. Arcs with smaller
 * priority are pruned first. For example, you might use the arc area/volume
 * directly as a simplification priority. 
 **/
void ct_priorityFunc( ctContext * ctx, double (*priorityFunc)( ctNode*, void* ) );


	
/** 
 * Provide your own alloc/free functions for ctArc structures. You will need
 * to use these if you want to subclass ctArc. 
 **/ 	
void ct_arcAllocator( ctContext * ctx, ctArc* (*allocArc)(void*), void (*freeArc)( ctArc*, void*) );

/** 
 * Provide your own alloc/free functions for ctNode structures. You will need
 * to use this if you want to sublcass ctNode. 
 **/
void ct_nodeAllocator( ctContext * ctx, ctNode* (*allocNode)(void*) , void (*freeNode)( ctNode*, void*) );		

/** 
 * Provide your own alloc/free functions for ctBranch structures. You will need
 * to use this if you want to sublcass ctBranch. 
 **/
void ct_branchAllocator( ctContext * ctx, ctBranch* (*allocBranch)(void*), void (*freeBranch)( ctBranch*, void*) );



        
/** 
 * Perform the sweep and merge algorithm. This will take a while. Returns some
 * arc of the contour tree. The constructed tree is owned by the library, and 
 * will be deleted when ct_cleanup is called. If you want your own tree, use
 * ct_copyTree.
 **/
ctArc* ct_sweepAndMerge( ctContext * ctx );


/**
 * Perform just the join sweep. The point of calling this would be to
 * also call the split sweep in another thread; they can be performed
 * concurrently. After both are done, call ct_mergeTrees to get the
 * final contour tree
 **/

void ct_joinSweep( ctContext * ctx );

/**
 * Perform just the split sweep. The point of calling this would be to
 * also call the join sweep in another thread; they can be performed
 * concurrently. After both are done, call ct_mergeTrees to get the
 * final contour tree
 **/

void ct_splitSweep( ctContext * ctx );

/**
 * Call this after ct_joinSweep and ct_splitSweep are finished. It
 * will return an arc of the contour tree, same as ct_sweepAndMerge
 **/

ctArc * ct_mergeTrees( ctContext * ctx );

/** 
 * Perform the branch decomposition.  Returns the root branch.  This consumes
 * the contour tree, so don't access the contour tree or use ct_arcMap after
 * calling ct_decompose. If you need to keep the contour tree around, use
 * ct_copyTree to make a copy of it. The branch-decomposition tree that is
 * returned is YOURS. The library does not free it when ct_cleanup is called.
 **/   
ctBranch*  ct_decompose ( ctContext * ctx );



/** Free the working memory. */   
void  ct_cleanup ( ctContext * ctx );



/** 
 * Retreive the vertex-to-arc mapping. Returns an array of size equal to
 * ctContext.numVerts, where element i points to the arc which contains vertex
 * i. If i is a critical point, the map will point to one of the attached
 * arcs.  Ownership of the array is passed to the calling environment, HOWEVER
 * the array must not be freed or modified if you want to call ct_decompose.
 * Note that after ct_cleanup is called, the arcs that this map originally
 * points to are freed, so you should copy them with ct_copyTree.
 **/
ctArc ** ct_arcMap( ctContext * ctx );


/** 
 * Retreive the vertex-to-branch mapping. Returns an array of size equal to
 * ctContext.numVerts, where element i points to the branch which contains
 * vertex i. If i is a saddle point, the map will point to the parent branch.
 * IMPORTANT -- This function needs the arc map to do its thing. Don't modify
 * or free the result of ct_arcMap before calling ct_branchMap.  Ownership of
 * the array is passed to the calling environment. It is your responsibility
 * to free() this. This allows to you call ct_cleanup and still use your
 * branch map. 
 **/
ctBranch ** ct_branchMap( ctContext * ctx );




/**
 * Constructs a new tree which is a copy of src. If moveData is true,
 * The 'data' fields of the arcs and nodes of the new tree will be copied
 * from those of the old tree, while those fields in the old tree will
 * be modified to point to the corresponding arc/node of the new tree.
 * So the data fields are really 'moved' from the old tree to the new,
 * not just copied. But you can still access the data from the old tree
 * because those fields point to the new tree.
 *
 * This is not an ideal behavior; it would be nice to make a proper copy of
 * the data fields. The issue is that I don't yet have an efficient way to
 * make a copy the arcMap, so that it can be used to look up arcs in the new
 * tree.  Using this (lame, hackish) method, you can do the following:
 * 
 * \code 
 * ctArc *newTree = ct_copyTree(oldTree,TRUE,ctx);
 * ctArc **newArcMap = malloc( numVerts * sizeof(ctArc*) );
 * ctArc **oldArcMap = ct_arcMap(ctx);
 * for (size_t i=0; i<numVerts; ++i) 
 *   newArcMap[i] = oldArcMap[i]->data;
 * \endcode
 *
 * Then you can copy the data fields back to the old tree if you need them for
 * ct_decompose.
 **/
ctArc* ct_copyTree( ctArc *src, int moveData, ctContext *ctx );

	
/**
 * Fetch all the arcs and nodes in a tree. They are returned in arrays
 * arcsOut and nodesOut, which you must free. These are arrays of 
 * ctArc and ctNode pointers, which are returned by reference, hence the ***
 * silliness.
 **/

void ct_arcsAndNodes( ctArc *a, 
                      ctArc ***arcsOut,  size_t *numArcsOut, 
                      ctNode ***nodesOut, size_t *numNodesOut );


/** 
 * Delete a contour tree that you obtained from ct_copyTree. DON'T delete
 * the original tree obtained from ct_sweepAndMerge or ct_mergeTrees; That
 * one belongs to the library and will be freed by ct_cleaup
 **/
void ct_deleteTree( ctArc *a, ctContext *ctx );




/**  
    \mainpage libtourtre: A Contour Tree Library
    \image html tourtre.jpg
    \htmlonly
    <center>
    libtourtre is a library for con<u>tourtre</u>es. Tourtre is the French name
    for a bird. Here is a picture of said bird on a tree. 
    </center>
    \endhtmlonly

    \section Introduction
    
    This library implements the contour tree construction algorithm outlined in
    the paper "Computing Contour Trees in All Dimensions" 
    \htmlonly <a href="http://citeseer.ist.psu.edu/carr99computing.html">[link]</a> \endhtmlonly 
    by Carr, Snoeyink and Axen, and variant of another algorithm
    described in "Multi-Resolution computation and presentation of Contour
    Trees" 
    \htmlonly <a href="http://pascucci.org/pdf-papers/orrery-2005.pdf">[link]</a> \endhtmlonly 
    by Pascucci, Cole-McLaughlin and Scorzelli, which computes the so-called
    "branch decomposition." The algorithms are separate, but the latter depends
    on the former. The library contains three data structures (Arc, Node and
    Branch) that are convenient for representing the tree and its branch
    decomposition.

    \section Features
    
    A contour tree can be extracted from any scalar function defined on a
    domain which satisfies certain properties.  These properties are described in 
    \htmlonly <a href="http://www.cs.ucd.ie/staff/hcarr/home/">Hamish Carr's</a>\endhtmlonly 
    dissertation (if not elsewhere), but it suffices to say that it it works on
    simplicial complexes of genus zero. If your simplicial complex has holes,
    then you're looking for a Reeb graph, which is another algorithm. With some
    preprocessing, it can also be used for bilinearly and trilinearly
    interpolated images. For general image data, your best (easiest) bet is to
    impose a simplicial domain over the image.

    A feature of this library is that it produces a mapping from vertices of
    the domain to arcs (branches) of the contour tree (branch decomposition),
    which can be very handy.

    The algorithms in the library are serial, but the library is reentrant,
    should you need to compute another contour tree in one of the callbacks ...
    or something.

    \section Usage
    
    Include tourtre.h in your program.  Call ct_init() to create a ctContext,
    then call ct_sweepAndMerge() to create the contour tree and (optionally)
    ct_decompose() to transform it into a branch decomposition.

    The contour tree is made up of \link ctNode nodes \endlink and \link ctArc
    arcs \endlink, which form an acyclic graph. The branch decomposition is
    composed of \link ctBranch branches \endlink which form a rooted tree.

    \section Download
    
    The latest version is v15. There is a bug fix in the trilinear example
    program. Thanks to Vijeth Dinesha for spotting it.<br> 
    \htmlonly <a href="http://graphics.cs.ucdavis.edu/~sdillard/libtourtre/libtourtre_v15.tar.gz">Download libtourtre_v15.tar.gz</a>\endhtmlonly 

    There is an API change from v8 (which is still available: 
    \htmlonly <a href="http://graphics.cs.ucdavis.edu/~sdillard/libtourtre/libtourtre_v8.tar.gz">libtourtre_v8.tar.gz</a>. \endhtmlonly 
    ) A comparison callback is no longer required, the total order suffices. 
*/

#endif

