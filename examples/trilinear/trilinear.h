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

typedef struct Saddle Saddle;

typedef 
enum SaddleType
{ 
  YZ_FACE_SADDLE = 0, 
  XZ_FACE_SADDLE, 
  XY_FACE_SADDLE, 

  LO_BODY_SADDLE, /* the lesser valued of two body saddles, 
                   * or the lone body saddle */

  HI_BODY_SADDLE  /* the greater valued of two body saddles */

} SaddleType;

typedef 
struct SaddleInfo
{
    SaddleType type;

    size_t where; 
        /* the lower/left/front vertex of the face/cell 
         * containing this saddle */

    double location[3];
        /* Approximate location of saddle */

    double value;

} SaddleInfo;


int 
tl_get_saddle_info ( TrilinearGraph* g, size_t i, SaddleInfo *info );
  
  
  
  
  
  
  
  
  
#endif
