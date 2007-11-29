#ifndef CT_ALLOC_H
#define CT_ALLOC_H

/* simple wrappers for malloc and free */

struct ctArc* ct_arcMalloc( void* );
void ct_arcFree( struct ctArc*, void* );

struct ctNode* ct_nodeMalloc( void* );
void ct_nodeFree( struct ctNode*, void* );

struct ctBranch* ct_branchMalloc( void* );
void ct_branchFree( struct ctBranch*, void* );




#endif
