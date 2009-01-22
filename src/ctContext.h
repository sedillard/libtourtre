#ifndef CT_CONTEXT_H
#define CT_CONTEXT_H

#include "ctArc.h"
#include "ctNode.h"
#include "ctBranch.h"
#include "ctComponent.h"
#include "ctQueue.h"
#include "ctNodeMap.h"


struct ctContext 
{
    /** 
     * NECESSARY -- Array of vertices, in ascending order. i \< j <==> less(
     * totalOrder[i], totalOrder[j] ) 
     **/
    size_t * totalOrder;	

    /** 
     * NECESSARY -- Estimate of a vertex function value. ctContext.less takes
     * precedence. This is used only to estimate persistence for simplification 
     **/
    double (*value)( size_t, void* );

    /** 
     * OPTIONAL -- Specify the neighbors of vertex v. This function should
     * store the neighbors in the array nbrs, which is of size CT_MAX_VALENCE.
     * This function should return the valence of vertex v 
     **/	    
    size_t (*neighbors)( size_t v, size_t* nbrs, void* );

    /** 
     * OPTIONAL -- Maximum valence of a vertex. The default is 256. The array
     * argument to ctContext.neighbors will contain this much storage. 
     **/
    size_t maxValence;

    /** 
     * OPTIONAL -- Process a vertex in the sweepAndMerge algorithm. This is
     * called when the arc belonging to v is added to the contour tree. One use
     * for this might be to estimate the area/volume of an arc.  If your
     * vertices are evenly spaced, then you can keep an accumulator in
     * ctArc.data and add to it using this function.  This function may not be
     * called for every arc, but when it is called, you can use the vertex v as
     * a <a href="http://citeseer.ist.psu.edu/bajaj98contour.html">path
     * seed</a> for arc a.  It will be called many times for the "big,
     * important arcs."
     **/  
    void (*procVertex)( size_t v, ctArc * a, void*);

    /** 
     * OPTIONAL -- This is called when two arcs are merged by simplification.
     * If you are keeping track of arc properties using procVertex, then you
     * will want to sum up those properties using this function. The first
     * argument is the arc which will remain after the merge, the second
     * argument is the one which will be deleted. 
     **/
    void (*mergeArcs) ( ctArc* keep, ctArc* throwAway, void* );

    /** 
     * OPTIONAL -- Define the simplification priority of an arc. The function
     * is passed a leaf node. Use ctNode_leafArc() to access the leaf arc. Arcs
     * with smaller priority are pruned first. For example, you might use the
     * arc area/volume directly as a simplification priority.
     **/	    
    double (*priority)( ctNode* , void* );

    /** 
     * OPTIONAL -- This is passed as the final argument to all callbacks. Use
     * this for reentrant code. 
     **/
    void * cbData; /* last arg to all callbacks */
    
    
    ctArc* (*arcAlloc)(void*);
    void (*arcFree)(ctArc*,void*);
    
    ctNode* (*nodeAlloc)(void*);
    void (*nodeFree)(ctNode*,void*);
    
    ctBranch* (*branchAlloc)(void*);
    void (*branchFree)(ctBranch*,void*);
    
    
    /* 
     * Working memory used during the construction process. 
     */
    size_t numVerts;
    ctComponent *joinRoot, *splitRoot;
    ctComponent **joinComps, **splitComps;
    size_t *nextJoin, *nextSplit;
    ctArc ** arcMap;
    int arcMapOwned; /* does the library still own arcMap? */
    ctBranch ** branchMap; 
    int branchMapOwned; /* does the library still own branchMap */
    ctNodeMap *nodeMap;

    ctArc *tree; 
};

 
#endif
