/*
Copyright (c) 2009, Scott E. Dillard
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

/* NOTE: this is C99 */

#include <stdbool.h> /* for bool, true, false */
#include <stdint.h>  /* for int64_t */
#include <string.h>  /* for memcpy */
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "trilinear.h"
#include "sglib.h"

#define CONSOLE_FEEDBACK 0
#define CHATTY 0

#define OPT 0 //0 actually means yes

/* In the past there has been an issue where one CPU's implementation of sqrt
 * is such that a saddle is detected, but on another CPU that saddle is not
 * detected. This becomes a problem if you compute the contour tree on one
 * computer and then use that data to another computer.  If
 * DOUBLE_CHECK_SADDLES is true, then another test for saddles, one that
 * uses only integers, is applied. This should be consistent, and if the result
 * of this test disagrees with the result of the floating point test, a warning
 * is emitted. 
 */
#define DOUBLE_CHECK_SADDLES 0



typedef 
enum SortOrder
{ 
    SORT_NATURALLY = 0 ,
    SORT_BEFORE ,
    SORT_AFTER 
} SortOrder;


struct Saddle
{ 
    size_t key;
        /* the lower,left,front vertex of the cell containing this saddle */

    Saddle *left,*right; /* tree pointers */
    int color; /* red or black */ 

    size_t vert; 
        /* this saddle's vertex id */

    SaddleType type;
    double value;
    double pos[3]; /* approximate */

    int orientation; 
        /* For pairs of body saddles: 
         *   Indicates the vector (a diagonal of the cube) separating them 
         * For face saddles: 
         *   If the saddle is 'fake' (caused by perturbation) this indicates
         *   whether the 'saddle' is at the lower (0) or upper (1) vertex of the
         *   face. 
         */

    SortOrder sort; 
        /* Does this saddle have a prescirbed position in the total order? */
    size_t whom;
        /* If so, whom does it need to sort after/before? */

    /* TODO this thing is way bigger than it needs to be. The fields sort,
     * orientation, type and color can all be packed into a word */
};



/* A search tree is used to map cells to saddles. As an optimization, we break
 * the domain up into 'blocks', and use a separate search tree for each block,
 * to balance memory-vs-speed.
 *
 * The search key is the lower-left-front (lex. least) vertex of the
 * face/cell
 */

#define DEFAULT_BLOCK_SIZES {8,8,8}

struct BlockMap
{ 
  Saddle **blocks; 
  /* The tree is referenced by a pointer to the root node. Saddles are nodes,
   * so an array of trees is an array of Saddle pointers. */
}; 
typedef struct BlockMap BlockMap;


#define NO_ORIENTATION 0xffffffff


#define SADDLE_TREE_COMPARATOR(A,B) ( ((int)(A)->key) - ((int)(B)->key) )
SGLIB_DEFINE_RBTREE_PROTOTYPES(Saddle,left,right,color,SADDLE_TREE_COMPARATOR);
SGLIB_DEFINE_RBTREE_FUNCTIONS(Saddle,left,right,color,SADDLE_TREE_COMPARATOR);


struct TrilinearGraph
{
    Values data;
    size_t domain_size[3]; /* NOTE! If any of these are zero, underflow will happen */
    size_t nvoxels; /* product of domain_sizes */

    size_t offsets[3]; /* = { 1, domain_size[0], domain_size[0]*domain_size[1] } */

    BlockMap* block_maps[5]; /* one for each SaddleType */
    size_t block_size[3], nblocks[3];

    /* a list of saddles, to be indexed contiguously with the rest of the
    * vertices in the voxel grid */
    Saddle* *saddles; 
    size_t nsaddles, saddles_mem; 
        /* usage and size of saddles array */

    /* The total order on graph vertices. */
    size_t nverts; /* = nvoxels + nsaddles */
    size_t* order; /* ordinal -> vertex id */
    size_t* iorder; /* vertex id -> ordinal */

    int nsaddle_types[5];
};


int
tl_get_saddle_info( TrilinearGraph *g, size_t i, SaddleInfo *info )
{
    if (i < g->nvoxels || i >= g->nverts) return 0;
    Saddle *s = g->saddles[ i - g->nvoxels ];
    info->type = s->type;
    info->where = s->key;
    memcpy(info->location,s->pos,3*sizeof(double));
    info->value = s->value;
    return 1;
}




/* local functions */
static Saddle*  check_for_saddle ( TrilinearGraph* g, SaddleType t, size_t x, size_t y, size_t z );
static void     find_saddles ( TrilinearGraph* graph );

static Saddle*  create_saddle ( TrilinearGraph* graph, SaddleType type, size_t loc[3], double value, 
                                int orientation, SortOrder sort, size_t whom, double pos[3] );

static void     convert_index_inv ( const size_t size[], size_t id, size_t *x, size_t *y, size_t *z );
static size_t   convert_index_a ( const size_t size[], const size_t index[] );
static size_t   convert_index ( const size_t size[], size_t x, size_t y, size_t z );

static int      find_body_saddles ( Values data, double saddle_values[ 2 ], size_t verts[8], 
                                    int *orientation, double pos[2][3] );

static bool     find_face_saddle ( TrilinearGraph* g, const size_t v[8], const size_t fv[4], 
                                   double *saddle_val, int *orientation, SortOrder *sort, 
                                   size_t *whom, double *pos0, double *pos1 );

static int      orient_cell_to_saddles ( double pos[2][3], double values[2] ) ;
static double   eval_trilinear ( double pos[3], double c[8] ) ;

static Saddle** block_lookup ( const BlockMap* bm, const size_t block_size[3], 
                               const size_t nblocks[3], const size_t loc[3] );

static double   square_root ( double x );
static bool     compare ( TrilinearGraph *g, size_t, size_t );
static void     sort_grid_verts ( TrilinearGraph *g , size_t a[], size_t n,  size_t *t );
static void     sort_saddles ( TrilinearGraph *g, size_t a[], size_t n, size_t *t );
static void     merge_saddles_into_order ( TrilinearGraph *g, size_t verts[], size_t saddles[] );

#if DOUBLE_CHECK_SADDLES
typedef int64_t Int; /* This should probably use arbitrary precision arithmetic */
static void find_body_saddles_int ( Values data, size_t verts[8], int *saddle0, int *saddle1, int *saddle2 ) ;
static void check_greater_zero ( /*input*/ Int r, Int a, Int ax, Int c,  /*output*/ bool *o1, bool *o2 );
static void check_less_one ( /*input*/Int r, Int a, Int ax, Int c, /*ouput*/ bool *o1, bool *o2 ) ;
#endif



TrilinearGraph* 
tl_create_trilinear_graph
(   /*input*/  
    const uint32_t domain_size[3], /* the extents of the 3D voxel grid */
    Values data,  /* the 3D voxel grid */

    /*output*/ 
    size_t *num_inds,
    size_t *sorted_inds[] 
        /* a list of contiguous indexes of graph vertices, in the range 0..n-1,
         * sorted by total_order */
)
{
    TrilinearGraph* tg = (TrilinearGraph*) malloc(sizeof(*tg));
    tg->data = data;

    const size_t block_size[3] = DEFAULT_BLOCK_SIZES;

    /* copy sizes */
    tg->nvoxels = 1;
    for (size_t i=0;i<3;++i) {
        tg->domain_size[i] = domain_size[i];
        tg->nvoxels *= domain_size[i];
        tg->block_size[i] = block_size[i];
        tg->nblocks[i] = (domain_size[i]/block_size[i])+1 ;
    }

    tg->offsets[0] = 1;
    tg->offsets[1] = tg->domain_size[0];
    tg->offsets[2] = tg->domain_size[0] * tg->domain_size[1];

    /* init block maps */
    size_t block_bytes = tg->nblocks[0] * tg->nblocks[1] * tg->nblocks[2] * sizeof(Saddle*);
    for (size_t i=0; i<5; ++i) {
        tg->block_maps[i] = malloc(sizeof(BlockMap));
    
        /* allocate block array */
        tg->block_maps[i]->blocks = (Saddle**) malloc (block_bytes);
        memset(tg->block_maps[i]->blocks,0,block_bytes);
    }

    /* find saddles */
    #if CONSOLE_FEEDBACK && CHATTY
    printf("\nlooking for saddles\n"); 
    #endif
    tg->saddles_mem = 256; 
    tg->saddles = (Saddle**)malloc(sizeof(Saddle*)*tg->saddles_mem);
    tg->nsaddles = 0;
    find_saddles(tg);

    #if CONSOLE_FEEDBACK 
    /* count how many saddles of each type (just curious) */
    for (size_t i=0; i<5; ++i) tg->nsaddle_types[i] = 0;
    for (size_t i=0; i<tg->nsaddles; ++i) 
        tg->nsaddle_types[tg->saddles[i]->type] += 1;
    printf("found %d potential face saddles\n",
        tg->nsaddle_types[0]+tg->nsaddle_types[1]+tg->nsaddle_types[2]);
    printf("found %d body saddles\n",
        tg->nsaddle_types[3]+tg->nsaddle_types[4]);
    #endif

    /* create vertices of the abstract 'trilinear graph' */
    tg->nverts = tg->nvoxels + tg->nsaddles;
    tg->order = malloc(sizeof(size_t) * tg->nverts);
    tg->iorder = 0; /* malloc(sizeof(size_t) * tg->nverts); */
    /* memset(tg->iorder, -1, sizeof(size_t)*tg->nverts); */
    
    #if CONSOLE_FEEDBACK && CHATTY 
    printf("\ninitial sort (grid verts)\n");
    #endif

    /* inialize the ordering to the identity map.
     * stable sorting then yields symbolic perturbation */
    for ( size_t i=0; i<tg->nvoxels; ++i ) tg->order[i] = i;

    /* first sort the vertices of the voxel grid */
    size_t aux_size = tg->nvoxels / 2 + 1; /* temp space for merge sort */
    size_t *aux = (size_t*)malloc(aux_size*sizeof(size_t));
    sort_grid_verts(tg,tg->order,tg->nvoxels,aux);


    #if CONSOLE_FEEDBACK && CHATTY 
    printf("\ninitial sort (saddles)\n");
    #endif

    /* then sort the saddles */
    aux_size = tg->nsaddles / 2 + 1;
    aux = realloc(aux,aux_size*sizeof(size_t));
    size_t *saddle_inds = (size_t*)malloc(tg->nsaddles*sizeof(size_t));
    for ( size_t i=0; i<tg->nsaddles; ++i ) saddle_inds[i] = i;
    sort_saddles(tg,saddle_inds,tg->nsaddles,aux);
    free(aux);


    #if CONSOLE_FEEDBACK && CHATTY 
    printf("\nmerging sorted lists\n");
    #endif

    /* finally merge the two sorted lists. */
    merge_saddles_into_order(tg,tg->order,saddle_inds);
    free(saddle_inds);

    /* Output. Ownership of tg->order is passed to caller. */
    *sorted_inds = tg->order;
    *num_inds = tg->nverts;
    return tg;
}


void tl_cleanup( TrilinearGraph *g )
{
    for( size_t i=0; i<g->nsaddles; ++i ) free(g->saddles[i]);
    free(g->saddles);
    /* free(g->iorder); */
}



/* block_lookup : returns a reference to the splay tree root for the block
 * containing location loc. The splay tree is a pointer to the root node, and
 * so the reference to this is a pointer to a pointer */

static inline
Saddle** 
block_lookup
(   const BlockMap* bm, 
    const size_t block_size[3], 
    const size_t nblocks[3], 
    const size_t loc[3] )
{
    size_t d[3];
    for (size_t i=0;i<3;++i) d[i] = loc[i] / block_size[i];
    size_t idx = ( d[2] * nblocks[1] + d[1] ) * nblocks[0] + d[0];
    return bm->blocks+idx;
}




  
static const size_t yz_face_verts[4] = { 0,2,4,6 };
static const size_t xz_face_verts[4] = { 0,1,4,5 };
static const size_t xy_face_verts[4] = { 0,1,2,3 };


int 
tl_total_order( TrilinearGraph *g, int a, int b )
{
    return g->iorder[a] < g->iorder[b];
}

/* Comparison with symbolic perturbation. Does NOT yield a consistent, correct
 * total order. Just a starting point. */
static 
bool
compare ( TrilinearGraph *g, size_t a, size_t b )
{
   double av = a < g->nvoxels ? g->data[a] : g->saddles[a-g->nvoxels]->value;
   double bv = b < g->nvoxels ? g->data[b] : g->saddles[b-g->nvoxels]->value;
   return (av==bv) ? a < b : av < bv ;
}


/* 
 * Determine if a face saddle exists within a face, and if so compute its
 * value. This also finds 'fake' saddles that are created through symbolic
 * perturbation.
 */
static 
bool 
find_face_saddle 
(   TrilinearGraph  *g, 
    const size_t  v[8],  //the vertices of the cell
    const size_t fv[4],  //which of the cell vetices make up this face?
        //output
    double *saddle_val, 
    int *orientation  ,
    SortOrder *sort ,
    size_t *whom ,
    double *loc0 ,
    double *loc1 )
{
  Values f = g->data;
  bool e[4] = {
    compare( g, v[fv[0]], v[fv[1]] ),
    compare( g, v[fv[0]], v[fv[2]] ),
    compare( g, v[fv[3]], v[fv[1]] ),
    compare( g, v[fv[3]], v[fv[2]] )
  };

  if ( e[0] == e[1] && e[1] == e[2] && e[2] == e[3] ) {
    double val[4] = { f[v[fv[0]]], f[v[fv[1]]], f[v[fv[2]]], f[v[fv[3]]] };

    /*check for real (numerical) saddle*/
    double d = val[0] - val[1] - val[2] + val[3];
    if (d != 0) {
        double y = ( val[0] - val[1] ) / d;
        double x = ( val[0] - val[2] ) / d;
        if (y > 0 && y < 1 && x > 0 && x < 1) { /*found real saddle*/
            *saddle_val = val[0]*(1-x-y) + val[1]*x + val[2]*y + x*y*d;

            assert( *saddle_val != val[0] );
            assert( *saddle_val != val[1] );
            assert( *saddle_val != val[2] );
            assert( *saddle_val != val[3] );
            *orientation = NO_ORIENTATION;
            *sort = SORT_NATURALLY;
            *whom = (size_t)-1;
            *loc0 = x;
            *loc1 = y;

            return true;
        }
    } 
    
    if ( e[0] ) {

        /*
         *   1 <---- 0      
         *   ^       |  Saddle needs to sort 
         *   |       |  after 0 and (1)
         *   | s     v  
         *  (1)----> 1      
         */

        *saddle_val = val[0];
        *orientation = 0;
        *sort = SORT_AFTER;
        *whom = v[fv[0]];
    } else {

        /* 
         *   0 ---->(0)  
         *   |     s ^  Saddle needs to sort 
         *   |       |  before 1 and (0)
         *   v       |  
         *   1 <---- 0  
         */

        *saddle_val = val[3];
        *orientation = 1;
        *sort = SORT_BEFORE;
        *whom = v[fv[3]];
    }

    return true;
  } else {
    return false;
  }
}





static inline
double 
eval_trilinear( double pos[3], double c[8] ) 
{
  return 
    c[0] * pos[0] * pos[1] * pos[2] 
  + c[1] * pos[0] * pos[1] 
  + c[2] * pos[1] * pos[2] 
  + c[3] * pos[0] * pos[2] 
  + c[4] * pos[0] 
  + c[5] * pos[1] 
  + c[6] * pos[2] 
  + c[7];
}



/* 
 * Returns a corner of the main diagonal, the one which is adjacent to the
 * first saddle given in the argument arrays.
 */
static 
int 
orient_cell_to_saddles( double pos[2][3], double values[2] ) 
{
  return (pos[0][0] < pos[1][0] ? 0 : 1) 
       | (pos[0][1] < pos[1][1] ? 0 : 2) 
       | (pos[0][2] < pos[1][2] ? 0 : 4);
}


static
double 
square_root(double n)
{
    /* if sqrt is implemented by an special instruction (e.g. SSE) then
     * we might get different answers on different computers when asking
     * the question "does this cell contain a saddle" */
    return sqrt(n);
    /* maybe this will be more consistent across cpus
    double x = 1;
    int i;
    for (int i = 0; i < 5; i++) {
        x = 0.5*(x+n/x);
    }
    return x;
    */
}


int 
find_body_saddles 
(   Values data, 
    double saddle_values[ 2 ], 
    size_t verts[8], 
    int *orientation ,
    double pos[2][3] )
{
  double v[8]; /*function values at vertices*/
  for (size_t i=0; i<8; i++) v[i] = data[ verts[i] ]; 

  double ax,ay,az;

  #if DOUBLE_CHECK_SADDLES
  int s0,s1,s2; 
    /* flags indicating saddles. 0 is double-root, 1 and 2 are the singular
     * roots */
  #endif

  double c[8] =  /* trilinear coeffecients */
    {
    /*0 a*/ v[1] + v[2] + v[4] + v[7] - v[0] - v[6] - v[5] - v[3],
    /*1 b*/ v[0] - v[1] + v[3] - v[2],
    /*2 c*/ v[0] - v[2] - v[4] + v[6],
    /*3 d*/ v[0] - v[1] - v[4] + v[5],
    /*4 e*/ v[1] - v[0],
    /*5 f*/ v[2] - v[0],
    /*6 g*/ v[4] - v[0],
    /*7 h*/ v[0] 
    };
  
  /*
  ax = a*e - b*d;
  ay = a*f - b*c;
  az = a*g - c*d;
  */
  
  ax = c[0]*c[4] - c[1]*c[3],
  ay = c[0]*c[5] - c[1]*c[2],
  az = c[0]*c[6] - c[2]*c[3];

  #if DOUBLE_CHECK_SADDLES
  find_body_saddles_int( data, verts, &s0, &s1, &s2 );
  #endif

  if (c[0] != 0) {
    if ( ax*ay*az < 0 ) {
      size_t nsaddles = 0;
      double discr = square_root( -ax * ay * az );

      pos[nsaddles][0] =  -( c[2] + discr / ax ) / c[0];
      pos[nsaddles][1] =  -( c[3] + discr / ay ) / c[0];
      pos[nsaddles][2] =  -( c[1] + discr / az ) / c[0];
      
      if (0 < pos[nsaddles][0] && pos[nsaddles][0] < 1.0 &&
          0 < pos[nsaddles][1] && pos[nsaddles][1] < 1.0 &&
          0 < pos[nsaddles][2] && pos[nsaddles][2] < 1.0 ) 
      {
        saddle_values[nsaddles] = eval_trilinear( pos[nsaddles], c );

        #if DOUBLE_CHECK_SADDLES
        if (!s1) { 
          fprintf( stderr,
            "S1: ints say no saddle, but floats say there's a saddle \
            at ( %f , %f , %f )\n", 
            pos[nsaddles][0],
            pos[nsaddles][1],
            pos[nsaddles][2]);
        }        
        #endif

        ++nsaddles;
      } else {
        #if DOUBLE_CHECK_SADDLES
        if (s1) {
          fprintf(stderr,
            "S1: ints say saddle, but floats say no saddle, pos is \
            ( %f , %f , %f )\n",
            pos[nsaddles][0],
            pos[nsaddles][1],
            pos[nsaddles][2]);
        }              
        #endif
      }

      pos[ nsaddles ][ 0 ] =  -( c[2] - discr / ax ) / c[0];
      pos[ nsaddles ][ 1 ] =  -( c[3] - discr / ay ) / c[0];
      pos[ nsaddles ][ 2 ] =  -( c[1] - discr / az ) / c[0];

      if (0 < pos[nsaddles][0] && pos[nsaddles][0] < 1.0 &&
          0 < pos[nsaddles][1] && pos[nsaddles][1] < 1.0 &&
          0 < pos[nsaddles][2] && pos[nsaddles][2] < 1.0 ) 
      {
        saddle_values[nsaddles] = eval_trilinear( pos[nsaddles], c );
        #if DOUBLE_CHECK_SADDLES
        if (!s2) {
          fprintf(stderr,
            "S2: ints say no saddle, but floats say there's a saddle \
            at ( %f , %f , %f )\n",
            pos[nsaddles][0],
            pos[nsaddles][1],
            pos[nsaddles][2]);
        }
        #endif
        ++nsaddles;
      } else {
        #if DOUBLE_CHECK_SADDLES
        if (s2) {
          fprintf(stderr,
            "S2: ints say saddle, but floats say no saddle, pos is \
            ( %f , %f , %f )\n",
            pos[nsaddles][0],
            pos[nsaddles][1],
            pos[nsaddles][2]);
        }    
        #endif
      }
                              
      if (nsaddles == 2) 
        *orientation = orient_cell_to_saddles(pos,saddle_values);
      return nsaddles;
    } else {
      return 0;
    }
  } else {
    if ( c[1]*c[2]*c[3] != 0 ) {
      pos[0][0] = (c[2]*c[4] - c[1]*c[6] - c[3]*c[5] ) / (2*c[1]*c[3] );
      pos[0][1] = (c[3]*c[5] - c[1]*c[6] - c[2]*c[4] ) / (2*c[1]*c[2] );
      pos[0][2] = (c[1]*c[6] - c[2]*c[4] - c[3]*c[5] ) / (2*c[2]*c[3] );
      
      if (0 < pos[0][0] && pos[0][0] < 1.0 &&
          0 < pos[0][1] && pos[0][1] < 1.0 &&
          0 < pos[0][2] && pos[0][2] < 1.0  )	
      {
        saddle_values[ 0 ] = eval_trilinear( pos[0], c );
        #if DOUBLE_CHECK_SADDLES
        if (!s0) {
          fprintf(stderr,
            "S0: ints say no saddle, but floats say there's a saddle \
            at ( %f , %f , %f )\n",
            pos[0][0],pos[0][1],pos[0][2]);
        }
        #endif
        return 1;
      } else {
        #if DOUBLE_CHECK_SADDLES
        if (s0) {
          fprintf(stderr,
            "S0: ints say saddle, but floats say no saddle, pos is \
            ( %f , %f , %f )\n",
            pos[0][0],pos[0][1],pos[0][2]);
        }         
        #endif
        return 0;
      }
    } else {
      return 0;
    }
  }
}



static inline
size_t 
convert_index
(   const size_t size[], 
    size_t x, 
    size_t y, 
    size_t z )
{
  return (z * size[1] + y) * size[0] + x;
}


static inline
size_t 
convert_index_a
(   const size_t size[], 
    const size_t index[] )
{
  return (index[2] * size[1] + index[1]) * size[0] + index[0];
}


static inline
void 
convert_index_inv
(   const size_t size[], 
    size_t id, 
    size_t *x, 
    size_t *y, 
    size_t *z )
{
  size_t size01 = size[0] * size[1];
  *z = id / size01;
  *y = (id - (*z)*size01) / size[0];
  *x = id - (*z)*size01 - (*y)*size[0];
}


static 
Saddle* 
create_saddle
(   TrilinearGraph* g, 
    SaddleType type, 
    size_t loc[3], 
    double value, 
    int orientation,
    SortOrder sort,
    size_t whom,
    double pos[3] )
{
    /*create saddle record*/
    Saddle *s = (Saddle*)malloc(sizeof(Saddle));
        /* saddle records are not tiny. malloc overhead is negligible. */

    /*find block in which we'll place the loc->saddle mapping*/
    Saddle** root = block_lookup( g->block_maps[type], g->block_size, g->nblocks, loc ); 

    /*fill out saddle info*/
    s->type = type;
    s->value = value;
    s->orientation = orientation;
    s->sort = sort;
    s->whom = whom;

    for (int i=0; i<3; ++i) s->pos[i] = pos[i]+loc[i];

    /*map loc to saddle*/
    s->key = convert_index_a(g->domain_size, loc);
    sglib_Saddle_add(root,s);

    /*append saddle to list*/
    ++g->nsaddles;
    if (g->nsaddles == g->saddles_mem) {
        g->saddles = 
            realloc(g->saddles, (g->saddles_mem *= 2)*sizeof(Saddle*) );
    }
    s->vert = g->nvoxels + g->nsaddles - 1;
    g->saddles[ g->nsaddles-1 ] = s;
    
    return s;
}






static 
void 
find_saddles ( TrilinearGraph* g )
{
  /*find all cell saddles and face saddles, then store them in the BlockMap */
    const Values data = g->data;
  
    const size_t size[3] = 
        {   g->domain_size[0], 
            g->domain_size[1], 
            g->domain_size[2] 
        };

    for (size_t z = 0; z < size[2]; ++z) 
    for (size_t y = 0; y < size[1]; ++y)
    for (size_t x = 0; x < size[0]; ++x) {
        #if CONSOLE_FEEDBACK
        size_t index = convert_index(size,x,y,z);
        const int div = 10000;
        if ( (index%div) == 0 ) { 
            printf("\r%d / %d   ", 
                (index/div)+1, 
                (g->nvoxels)/div); fflush(stdout); } 
        #endif
        
        size_t verts[8];
        double values[8];
        
        size_t loc[3] = {x,y,z};
        
        size_t c = 0;
        for (size_t k = z; k <= z+1; ++k) 
        for (size_t j = y; j <= y+1; ++j) 
        for (size_t i = x; i <= x+1; ++i) {
            if (i < size[0] &&
                j < size[1] &&
                k < size[2] )
            { 
                verts[c] = convert_index(size,i,j,k);
                values[c] = data[ verts[c] ];
            } else {
                verts[c] = 0xffffffff;
                values[c] = HUGE_VAL;
            }
            ++c;
        }
        
        double saddle_value = NAN;
        int orientation = NO_ORIENTATION;
        SortOrder sort = SORT_NATURALLY;
        size_t whom = (size_t)-1;

        /* find YZ face saddle */
        if ( y < size[1]-1 && z < size[2]-1 ) {
            double pos[3]={0,0,0};
            if ( find_face_saddle( 
                    g, verts, yz_face_verts, 
                    &saddle_value, &orientation, &sort, &whom, pos+1,pos+2 ) ) 
            {
                for (int i=0; i<3; ++i) pos[i] += loc[i];
                create_saddle( g, YZ_FACE_SADDLE, loc, saddle_value, orientation, sort, whom, pos );
            }
        }
        
        /* find XZ face saddle */
        if ( x < size[0]-1 && z < size[2]-1 ) {
            double pos[3]={0,0,0};
            if ( find_face_saddle( 
                    g, verts, xz_face_verts, 
                    &saddle_value, &orientation, &sort, &whom, pos+0, pos+2 ) ) 
            {
                create_saddle( 
                    g, XZ_FACE_SADDLE, loc, saddle_value, orientation, sort, whom, pos );
            }
        }
        
        /* find XY face saddle */
        if ( x < size[0]-1 && y < size[1]-1 ) {
            double pos[3]={0,0,0};
            if ( find_face_saddle( 
                    g, verts, xy_face_verts, 
                    &saddle_value, &orientation, &sort, &whom, pos+0, pos+1 ) ) 
            {
                create_saddle( 
                    g, XY_FACE_SADDLE, loc, saddle_value, orientation, sort, whom, pos );
            }
        }
        if ( x < size[0]-1 && y < size[1]-1 && z < size[2]-1 ) {
            /*find cell saddles*/
            double saddle_values[2];
            double pos[2][3];
            size_t nsaddles = find_body_saddles(data,saddle_values,verts,&orientation,pos);
            if (nsaddles == 1) {
                create_saddle( g, LO_BODY_SADDLE, loc, saddle_values[0], NO_ORIENTATION, 
                               SORT_NATURALLY, (size_t)-1, pos[0] );
            }
            if (nsaddles == 2) { /*figure out which saddle is lower and which is higher*/
                if (saddle_values[0] < saddle_values[1]) {
                    create_saddle( g, LO_BODY_SADDLE, loc, saddle_values[0], orientation , 
                                   SORT_NATURALLY, (size_t)-1, pos[0]);
                    create_saddle( g, HI_BODY_SADDLE, loc, saddle_values[1], ~orientation&7 , 
                                   SORT_NATURALLY, (size_t)-1, pos[1]);
                } else {
                    create_saddle( g, LO_BODY_SADDLE, loc, saddle_values[1], ~orientation&7 , 
                                   SORT_NATURALLY, (size_t)-1, pos[1]);
                    create_saddle( g, HI_BODY_SADDLE, loc, saddle_values[0], orientation , 
                                   SORT_NATURALLY, (size_t)-1, pos[0]);
                }
            }		
        }
    } 


    for (size_t z = 0; z < size[2]-1; ++z) 
    for (size_t y = 0; y < size[1]-1; ++y)
    for (size_t x = 0; x < size[0]-1; ++x) {
        Saddle *s[3] = { check_for_saddle(g,YZ_FACE_SADDLE,x,y,z) ,
                         check_for_saddle(g,XZ_FACE_SADDLE,x,y,z) ,
                         check_for_saddle(g,XY_FACE_SADDLE,x,y,z) };
        if ( s[0] && s[1] && s[2] &&
             s[0]->orientation == 0 &&
             s[1]->orientation == 0 &&
             s[2]->orientation == 0 )
        {
            double pos[3]={0,0,0};
            Saddle *bs = check_for_saddle(g,LO_BODY_SADDLE,x,y,z);
            if (! bs )  {

                /*        0--------0
                *        /|       /|
                *       / |      / |
                *      1--------0  |
                *      ^  1-----|--0
                *      | /      | / Body saddle s needs 
                *      |/'s     |/  to sort below (1)
                *     (1)-->--- 1
                */

                size_t v = convert_index(g->domain_size,x,y,z);
                create_saddle(g,
                    HI_BODY_SADDLE, (size_t[]){x,y,z},
                    g->data[v], 0, SORT_BEFORE, v, pos );
            }
        }
    }

    for (size_t z = 1; z < size[2]; ++z) 
    for (size_t y = 1; y < size[1]; ++y)
    for (size_t x = 1; x < size[0]; ++x) {
        Saddle *s[3] = { check_for_saddle(g,YZ_FACE_SADDLE, x ,y-1,z-1) ,
                         check_for_saddle(g,XZ_FACE_SADDLE,x-1, y ,z-1) ,
                         check_for_saddle(g,XY_FACE_SADDLE,x-1,y-1, z ) };
        if ( s[0] && s[1] && s[2] &&
             s[0]->orientation == 1 &&
             s[1]->orientation == 1 &&
             s[2]->orientation == 1 )
        {
            double pos[3]={1,1,1};
            Saddle *bs = check_for_saddle(g,LO_BODY_SADDLE,x-1,y-1,z-1);
            if (! bs )  {
                
                /*
                *         0---->---0
                *        /|    s ,/|
                *       / |      / ^
                *      1--------0  |
                *      |  1-----|--0
                *      | /      | / Body saddle s needs 
                *      |/       |/  to sort above (0)
                *      1--------1
                * 
                */  

                size_t v = convert_index(g->domain_size,x,y,z);
                create_saddle(g,
                    LO_BODY_SADDLE, (size_t[]){x-1,y-1,z-1},
                    g->data[v], 7, SORT_AFTER, v, pos );
            }
        }
    }

    #if CONSOLE_FEEDBACK
    printf("\n"); 
    #endif
}







static
Saddle* 
check_for_saddle 
(   TrilinearGraph *g, 
    SaddleType t, 
    size_t x, 
    size_t y, 
    size_t z )
{
    BlockMap* bm = g->block_maps[t]; 
    size_t loc[] = {x,y,z};
    Saddle** tree = block_lookup(bm,g->block_size,g->nblocks,loc);
    assert(tree);
    size_t index = convert_index(g->domain_size,x,y,z);
    Saddle k = {.key=index};
    Saddle *s = sglib_Saddle_find_member(*tree,&k);
    assert( !s || s->key == index );
    return s;
}




/* get_nbrs: returns the number of neigbors, and stores them in nbrs paramter */

int 
tl_get_neighbors 
(   TrilinearGraph* graph, 
    size_t index, 
    size_t* nbrs ) 
{
  size_t *size = graph->domain_size;
  size_t nnbrs = 0; /*number of neighbors found*/
  
  if (index < graph->nvoxels ) { /*REGULAR VERTEX*/
    size_t x,y,z;

    convert_index_inv(size,index,&x,&y,&z);
    
    if (x > 0        ) nbrs[nnbrs++] = convert_index(size,x-1,y,z); 
    if (x < size[0]-1) nbrs[nnbrs++] = convert_index(size,x+1,y,z);
    if (y > 0        ) nbrs[nnbrs++] = convert_index(size,x,y-1,z);
    if (y < size[1]-1) nbrs[nnbrs++] = convert_index(size,x,y+1,z);
    if (z > 0        ) nbrs[nnbrs++] = convert_index(size,x,y,z-1);
    if (z < size[2]-1) nbrs[nnbrs++] = convert_index(size,x,y,z+1);

    /* For each face that this vertex is on, check for a saddle... */
    
    #if OPT //OPTIMIZATION

    /* ...but we don't really need to! Every component of a vertex's lower link
     * is explored just by looking at other vertices. Checking for face and
     * body saddles is redundant. 
     */

    Saddle *s;
    /*4 adjacent YZ faces*/
    for (size_t j = y-1; j <= y; j++) 
    for (size_t k = z-1; k <= z; k++) {
      if (j >= size[1]-1 || k >= size[2]-1) continue;
      if ((s=check_for_saddle( graph, YZ_FACE_SADDLE, x,j,k)))
        nbrs[nnbrs++] = s->vert;
    }
    
    /*4 adjacent XZ faces*/
    for (size_t i = x-1; i <= x; i++) 
    for (size_t k = z-1; k <= z; k++) {
      if (i >= size[0]-1 || k >= size[2]-1) continue;
      if ((s=check_for_saddle( graph, XZ_FACE_SADDLE, i,y,k)))
        nbrs[nnbrs++] = s->vert;
    }
    
    /*4 adjacent XY faces*/
    for (size_t i = x-1; i <= x; i++) 
    for (size_t j = y-1; j <= y; j++) {
      if (i >= size[0]-1 || j >= size[1]-1) continue;
      if ((s=check_for_saddle( graph, XY_FACE_SADDLE, i,j,z)))
        nbrs[nnbrs++] = s->vert;
    }
    
    /*8 adjacent cells*/
    for (size_t i = 0; i < 2; i++)
    for (size_t j = 0; j < 2; j++)
    for (size_t k = 0; k < 2; k++) {
      if(i+x-1 >= size[0]-1 || j+y-1 >= size[1]-1 || k+z-1 >= size[2]-1) continue;
      else {
        int orientation = i|(j<<1)|(k<<2);
        if ((s=check_for_saddle( graph, LO_BODY_SADDLE, i+x-1,j+y-1,k+z-1)) && s->orientation != orientation )
          nbrs[nnbrs++] = s->vert;
        if ((s=check_for_saddle( graph, HI_BODY_SADDLE, i+x-1,j+y-1,k+z-1)) && s->orientation != orientation )
          nbrs[nnbrs++] = s->vert;
      }
    }
    #endif 
  
  } else { /*SADDLE*/

    Saddle* saddle = graph->saddles[index - graph->nvoxels]; /*this saddle*/
    Saddle* s; /*another saddle*/

    size_t x,y,z; 
    convert_index_inv(size, saddle->key, &x,&y,&z);
    
    switch (saddle->type) {
      case YZ_FACE_SADDLE : { 
        /*four vertices this face is on*/
        nbrs[nnbrs++] = convert_index(size,x,y,  z  );
        nbrs[nnbrs++] = convert_index(size,x,y+1,z  );
        nbrs[nnbrs++] = convert_index(size,x,y,  z+1);
        nbrs[nnbrs++] = convert_index(size,x,y+1,z+1);
        
        /*two cells this face is on*/
        #if OPT //OPTIMIZATION
        /* This check is redundant. The lower link of a face saddle
         * has at most two connected components, both of which are explored
         * by looking only at the face's vertices. 
         */
        if (x < size[0]-1) {
          if ((s=check_for_saddle( graph, LO_BODY_SADDLE, x,y,z))) nbrs[nnbrs++] = s->vert; 
          if ((s=check_for_saddle( graph, HI_BODY_SADDLE, x,y,z))) nbrs[nnbrs++] = s->vert; 
        }
        if (x > 0) {
          if ((s=check_for_saddle( graph, LO_BODY_SADDLE, x-1,y,z))) nbrs[nnbrs++] = s->vert; 
          if ((s=check_for_saddle( graph, HI_BODY_SADDLE, x-1,y,z))) nbrs[nnbrs++] = s->vert; 
        }
        #endif

      }
      break;
      
      case XZ_FACE_SADDLE : { 
        /*four vertices this face is on*/
        nbrs[nnbrs++] = convert_index(size,x,  y,z  );
        nbrs[nnbrs++] = convert_index(size,x+1,y,z  );
        nbrs[nnbrs++] = convert_index(size,x,  y,z+1);
        nbrs[nnbrs++] = convert_index(size,x+1,y,z+1);
        
        /*two cells this face is on*/
        #if OPT //OPTIMIZATION
        if (y < size[1]-1) {
          if ((s=check_for_saddle( graph, LO_BODY_SADDLE, x,y,z))) nbrs[nnbrs++] = s->vert; 
          if ((s=check_for_saddle( graph, HI_BODY_SADDLE, x,y,z))) nbrs[nnbrs++] = s->vert; 
        }
        if (y > 0) {
          if ((s=check_for_saddle( graph, LO_BODY_SADDLE, x,y-1,z))) nbrs[nnbrs++] = s->vert; 
          if ((s=check_for_saddle( graph, HI_BODY_SADDLE, x,y-1,z))) nbrs[nnbrs++] = s->vert; 
        }
        #endif
        
      }
      break;
      
      case XY_FACE_SADDLE : {
        /*four vertices this face is on*/
        nbrs[nnbrs++] = convert_index(size,x,  y  ,z);
        nbrs[nnbrs++] = convert_index(size,x+1,y  ,z);
        nbrs[nnbrs++] = convert_index(size,x,  y+1,z);
        nbrs[nnbrs++] = convert_index(size,x+1,y+1,z);
        
         
        /*two cells this face is on*/
        #if OPT //OPTIMIZATION
        if (z < size[1]-1) {
          if ((s=check_for_saddle( graph, LO_BODY_SADDLE, x,y,z))) nbrs[nnbrs++] = s->vert; 
          if ((s=check_for_saddle( graph, HI_BODY_SADDLE, x,y,z))) nbrs[nnbrs++] = s->vert; 
        }
        if (z > 0) {
          if ((s=check_for_saddle( graph, LO_BODY_SADDLE, x,y,z-1))) nbrs[nnbrs++] = s->vert; 
          if ((s=check_for_saddle( graph, HI_BODY_SADDLE, x,y,z-1))) nbrs[nnbrs++] = s->vert; 
        }
        #endif
        
      }
      break;
      
              
      case LO_BODY_SADDLE : 
      case HI_BODY_SADDLE : { 
        for (size_t k = 0 ; k < 2; k++) 
        for (size_t j = 0 ; j < 2; j++) 
        for (size_t i = 0 ; i < 2; i++) {
          int o = i | (j<<1) | (k<<2);
          if (saddle->orientation == 0xffffffff || (saddle->orientation) != (~o&7) )
            nbrs[nnbrs++] =  convert_index(size,x+i,y+j,z+k);
        }
        
        /*other saddle*/
        if (saddle->type == HI_BODY_SADDLE) { /*this is a hi saddle*/
          if ((s=check_for_saddle( graph, LO_BODY_SADDLE, x,y,z))) 
            nbrs[nnbrs++] = s->vert;
        } else { /*this is a lo saddle*/
          if ((s=check_for_saddle( graph, HI_BODY_SADDLE, x,y,z)))
            nbrs[nnbrs++] = s->vert;
        }
                
                
        /*6 faces*/
        if ((s=check_for_saddle( graph, YZ_FACE_SADDLE, x  ,y  ,z  ))) nbrs[nnbrs++] = s->vert;
        if ((s=check_for_saddle( graph, XZ_FACE_SADDLE, x  ,y  ,z  ))) nbrs[nnbrs++] = s->vert;
        if ((s=check_for_saddle( graph, XY_FACE_SADDLE, x  ,y  ,z  ))) nbrs[nnbrs++] = s->vert;
        if ((s=check_for_saddle( graph, YZ_FACE_SADDLE, x+1,y  ,z  ))) nbrs[nnbrs++] = s->vert;
        if ((s=check_for_saddle( graph, XZ_FACE_SADDLE, x  ,y+1,z  ))) nbrs[nnbrs++] = s->vert;
        if ((s=check_for_saddle( graph, XY_FACE_SADDLE, x  ,y  ,z+1))) nbrs[nnbrs++] = s->vert;

      }
    }
  }
  return nnbrs;
}


double 
tl_value( TrilinearGraph* g, size_t i )
{
  return 
    (i < g->nvoxels) ? 
      g->data[i] : 
      g->saddles[ i - g->nvoxels ]->value;
}




static inline 
void 
swap( size_t *a, size_t *b ) 
{
  int t = *a;
  *a = *b;
  *b = t;
}


static
void  
sort_grid_verts
(   TrilinearGraph *g , 
    size_t *a,  /* array (+offset) to start sorting from */
    size_t n,   /* num elements to sort */
    size_t *t ) /* aux space */
{
    if (n < 32) { /*insertion sort*/
        for (size_t i=0; i<n; ++i) {
            size_t ai = a[i];
            Value v = g->data[ai];
            int j = ((int)i)-1;
            while (j >= 0 && g->data[a[j]] > v) {
                a[j+1] = a[j];
                --j;
            } 
            a[j+1] = ai;
        }
    } else { /* mergesort: stable sort => symbolic perturbation */
        size_t n2=n/2;
        sort_grid_verts(g,a,n2,t);
        sort_grid_verts(g,a+n2,n-n2,t);

        memcpy(t,a,n2*sizeof(size_t));
        size_t i1=0, i2=n2, ti=0;
        Value v1=g->data[t[i1]], v2=g->data[a[i2]];
        for (;;) {
            if (v1<=v2) {
                a[ti++] = t[i1++];
                if (i1 >= n2) break;
                v1 = g->data[t[i1]]; 
            } else {
                a[ti++] = a[i2++];
                if (i2 >= n) break;
                v2 = g->data[a[i2]]; 
            }
        }
        while (i1<n2) a[ti++] = t[i1++];
        while (i2<n)  a[ti++] = a[i2++];
    }
}



static inline 
bool
compare_saddles( TrilinearGraph *g, size_t a, size_t b)
{
    Saddle *sa = g->saddles[a];
    Saddle *sb = g->saddles[b];
    return 
        (sa->value != sb->value) 
            ? (sa->value < sb->value)
            : (sa->whom != sb->whom)
                ? (sa->whom < sb->whom)
                : (sa->sort < sb->sort);
}



static
void  
sort_saddles
(   TrilinearGraph *g , 
    size_t *a,  /* array (+offset) to start sorting from */
    size_t n,   /* num elements to sort */
    size_t *t ) /* aux space */
{
    if (n < 32) { /*insertion sort*/
        for (int i=0; i<n; ++i) {
            size_t ai = a[i];
            int j = i-1;
            while ( j >= 0 && compare_saddles(g,ai,a[j]) ) {
                a[j+1] = a[j];
                --j;
            }
            a[j+1] = ai;
        }
    } else { /* mergesort: no real reason, I just like it. */
        size_t n2=n/2;
        sort_saddles(g,a,n2,t);
        sort_saddles(g,a+n2,n-n2,t);

        memcpy(t,a,n2*sizeof(size_t));
        size_t i1=0, i2=n2, ti=0;
        size_t s1=t[i1], s2=a[i2];
        for (;;) {
            if ( compare_saddles(g,s1,s2) ) {
                a[ti++] = t[i1++];
                if (i1 >= n2) break;
                s1 = t[i1];
            } else {
                a[ti++] = a[i2++];
                if (i2 >= n) break;
                s2 = a[i2];
            }
        }
        while (i1<n2) a[ti++] = t[i1++];
        while (i2<n)  a[ti++] = a[i2++];
    }
}




/*

    verts       empty           saddles
[*************---------]      [sssssssss]
             ^        ^                ^
             vi      end               si

[***********---------**]      [sssssssss]
           ^        ^                  ^

[********---------s****]      [ssssssss ]
        ^        ^                    ^
[*-***s***s***s***s****]      [s        ]
 ^^                            ^       

*/

static
void
merge_saddles_into_order 
(   TrilinearGraph *g,
    size_t order[],  /* this is the same as g->order */
    size_t saddles_sorted[] )
{
    int end = g->nverts-1;
    int vi = g->nvoxels-1;
    int si = g->nsaddles-1;

    Saddle *s = g->saddles[saddles_sorted[si]];
    while( vi >=0 && si >= 0 ) {
        switch(s->sort) {
            case SORT_NATURALLY:
                while( vi >= 0 && g->data[order[vi]] > s->value ) {
                    order[end--] = order[vi--]; 
                }
                order[end--] = s->vert;
                s = g->saddles[saddles_sorted[--si]];
            break;

            case SORT_BEFORE:
                while( vi >= 0 && order[vi] != s->whom ) {
                    order[end--] = order[vi--]; 
                }
                size_t ovi = order[vi]; /* save order[vi] as ovi, same as whom */
                order[end--] = order[vi--];  /* move whom into place */
                while( s->whom == ovi && s->sort == SORT_BEFORE) {
                    /* move all the saddles that sort before whom into place */
                    order[end--] = s->vert;
                    if (si == 0) break;
                    s = g->saddles[saddles_sorted[--si]];
                }
            break;

            case SORT_AFTER:
                /* this case is a bit complicated */
                while ( vi >= 0 ) {
                    /* first scan down the list of grid verts until we find s->whom */
                    if ( order[vi] == s->whom ) {
                        /* due to careful choice of the initial sorting order of saddles,
                         * we should find BBBBAAAAA at the end of the saddle list, where
                         * the B's are the ones that must come before whom, and the A's 
                         * are those that come after. */
                        while( s->whom == order[vi] && s->sort == SORT_AFTER) {
                            /* move all the A's */
                            order[end--] = s->vert; 
                            if (si == 0) break;
                            s = g->saddles[saddles_sorted[--si]];
                        }
                        order[end--] = order[vi]; /* move whom */
                        while( s->whom == order[vi] && s->sort == SORT_BEFORE) {
                            /* move all the B's */
                            order[end--] = s->vert;
                            if (si == 0) break;
                            s = g->saddles[saddles_sorted[--si]];
                        }
                        --vi;

                        break;
                    } else {
                        order[end--] = order[vi--]; 
                    }
                }
            break;
            default : abort();
        }
    }
}


















#if DOUBLE_CHECK_SADDLES

static 
void 
check_less_one 
( /*input*/
    Int r, Int a, Int ax, Int c, 
  /*ouput*/ 
    bool* o1, bool *o2 ) 
{
  Int acx = ax*(a+c);
  
  assert(a * ax != 0);
  assert(r > 0);
  
  /*trying to determine if +/- sqrt(r) < acx */
  if ( a*ax > 0 ) {
    if ( acx < 0 ) *o1 = false; /* acx is negative, always less than sqrt(r) */
    else *o1 = r < acx*acx;     /* acx is positive, check r < acx^2 */
    if ( acx > 0 ) *o2 = true;  /* acx is positive, s*o -acx always less than sqrt(r) */
    else *o2 = r > acx*acx;
  } else {
    if ( acx < 0 ) *o1 = true;  /* acx is negative, always less than +sqrt(r) */
    else *o1 = r > acx*acx;     /* acx is positive, check r > acx^2 */
    if ( acx > 0 ) *o2 = false; /* acx is positive, s*o -acx always less than sqrt(r) */
    else *o2 = r < acx*acx;
  }
}


static 
void 
check_greater_zero 
( /*input*/ 
    Int r, Int a, Int ax, Int c,  
  /*output*/ 
    bool *o1, bool *o2 )
{
  Int cax = c*ax;
  
  assert(a * ax != 0);
  assert(r > 0);
  
  /*trying to determine if +/- sqrt(r) > cax */
  if ( a*ax > 0 ) {
    if (cax < 0) *o1 = true;  /*cax is negative, always less than sqrt(r) */
    else *o1 = r > cax*cax;   /*cax is positive, check r > cax^2 */
    if (cax > 0) *o2 = false; /*cax is positive, -cax never greater than sqrt(r) */
    else *o2 = r < cax*cax;   /*cax is negative, check r < cax^2 */
  } else {
    if (cax < 0) *o1 = false; /*cax is negative, never greater than sqrt(r) */
    else *o1 = r < cax*cax;
    if (cax > 0) *o2 = true;
    else *o2 = r > cax*cax;
  }
}



/* this is a diagnostic function that checks for the existance of cell saddles,
 * but does not compute their values. s1 and s2 are the 2 singular roots, s0
 * is the double root it uses interger math, for exact, reproducible results.
 * if Value is not an integral type, this is meaningless. 
 * 
 * These calculations need a lot of precision. If Value is 8-bit unsigned ints,
 * then Int needs to be 64 bits. 
 * 
 */
static 
void 
find_body_saddles_int
(   Values data, 
    size_t verts[8], 
    int *saddle0, 
    int *saddle1, 
    int *saddle2 ) 
{
  int v[8]; /*function values at vertices*/

  for (int i=0; i<8; ++i) 
    v[i] = (int) data[ verts[i] ];

  *saddle0 = *saddle1 = *saddle2 = false;

  {
    Int a = v[ 1 ] + v[ 2 ] + v[ 4 ] + v[ 7 ] - v[ 0 ] - v[ 6 ] - v[ 5 ] - v[ 3 ],
        b = v[ 0 ] - v[ 1 ] + v[ 3 ] - v[ 2 ], 
        c = v[ 0 ] - v[ 2 ] - v[ 4 ] + v[ 6 ], 
        d = v[ 0 ] - v[ 1 ] - v[ 4 ] + v[ 5 ], 
        e = v[ 1 ] - v[ 0 ], 
        f = v[ 2 ] - v[ 0 ], 
        g = v[ 4 ] - v[ 0 ],
        ax = a*e-b*d,
        ay = a*f-b*c,
        az = a*g-c*d;

    if ( a != 0 ) {
      Int r = -ax*ay*az;
      if ( r > 0 ) {
        bool xl1, xl2, yl1, yl2, zl1, zl2;
        bool xg1, xg2, yg1, yg2, zg1, zg2;
        check_less_one( r, a, ax, c , &xl1, &xl2 );
        check_less_one( r, a, az, b , &zl1, &zl2 );
        check_less_one( r, a, ay, d , &yl1, &yl2 );
        check_greater_zero( r, a, ax, c , &xg1, &xg2 );
        check_greater_zero( r, a, ay, d , &yg1, &yg2 );
        check_greater_zero( r, a, az, b , &zg1, &zg2 );
        *saddle2 = xl1 && xg1 && yl1 && yg1 && zl1 && zg1 ;
        *saddle1 = xl2 && xg2 && yl2 && yg2 && zl2 && zg2 ;
      }
            
    } else {
      if ( b*c*d != 0 ) {
        if ( b*d > 0 ) {
          if ( c*e - b*g - d*f <= 0 ) goto fail;
        } else {
          if ( c*e - b*g - d*f >= 0 ) goto fail;
        }
        if ( b*c > 0 ) {
          if ( d*f - b*g - c*e <= 0 ) goto fail;
        } else {
          if ( d*f - b*g - c*e >= 0 ) goto fail;
        }
        if ( c*d > 0 ) {
          if ( b*g - c*e - d*f <= 0 ) goto fail;
        } else {
          if ( b*g - c*e - d*f >= 0 ) goto fail;
        }
        if ( abs(c*e - b*g - d*f) >= abs(2*b*d) ) goto fail;
        if ( abs(d*f - b*g - c*e) >= abs(2*b*c) ) goto fail;
        if ( abs(b*g - c*e - d*f) >= abs(2*c*d) ) goto fail;
        *saddle0 = true;
        fail:;
      }
    }
  }
}




#endif











/*  Tricky case:
 
       0--->---0--->---0
      /|      /|      /|
     / v     / ^     / v
    1---<---0---->--1  |
    |  1--<-|--0-->-|--1
    ^ /     v /     ^ /
    |/      |/      |/
    1--->---1---->--1
       0--->---0--->---0--->---0
      /|      /|      /|      /|
     / v     / ^     / v     / ^
    1---<---0---->--1---<---0  |
    |  1--<-|--0-->-|--1--<-|--0
    ^ /     v /     ^ /     v / 
    |/      |/      |/      |/ 
    1--->---1---->--1--->---1   

*/
