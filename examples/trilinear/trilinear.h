#ifndef TRILINEAR_H
#define TRILINEAR_H

#include <stdint.h>
#include <stdlib.h>


/* right now these are best left to 8bit quantities */
typedef uint8_t  Value;
typedef uint8_t* Values;

struct TrilinearGraph;

typedef struct TrilinearGraph TrilinearGraph;

TrilinearGraph* tl_create_trilinear_graph 
  ( 
    /*input*/  
      const uint32_t domain_size[3], 
            /* the extends of the 3D voxel grid */
      Values data,
            /* the 3D voxel grid */

    /*output*/ 
      size_t *num_inds,
      size_t *sorted_inds[] 
        /* a pointer to an array, it will be filled with a list
         * of contiguous indexes of graph vertices, in the
         * range 0..(num_inds-1), sorted by total_order */
  );

/* total_order: returns true if a < b */
int tl_total_order( TrilinearGraph*, int a, int b );

int tl_get_neighbors ( TrilinearGraph* graph, size_t index, size_t* nbrs ); 

double tl_value( TrilinearGraph* g, size_t i );

void tl_cleanup(TrilinearGraph* g);
  
#endif
