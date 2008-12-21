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

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <tourtre.h>
#include <ctBranch.h>

#include "trilinear.h"

typedef
struct WorkingData_ {
  Values data;
  int data_size[3];
  TrilinearGraph *graph;
} WorkingData;

static
double 
value (size_t i, void* wd)
{
  return tl_value( ((WorkingData*)wd)->graph, i );
}

static
size_t 
neighbors( size_t i, size_t* nbrs, void* wd )
{
  return tl_get_neighbors( ((WorkingData*)wd)->graph, i, nbrs );
}


void
print_header_comment (FILE *f )
{

fprintf(f,"\
# contour tree file format:\n\
# \n\
#     <branches>\n\
#     'saddles' <nsaddles>\n\
#     <saddles>\n\
# \n\
# branches : '(' <ext> <sdl> <nchildren> <branches> ')'\n\
#          | ''\n\
# \n\
# ext : Integer, the extremum\n\
# \n\
# sdl : Integer, the saddle\n\
# \n\
# nchildren : number of child branches\n\
# \n\
# nsaddles : Integer, number of saddle records\n\
# \n\
# saddles : saddle_rec '\\n' <saddles>\n\
#         | ''\n\
# \n\
# saddle_rec : <id> <after> <type> <corner> <loc_x> <loc_y> <loc_z> <value>\n\
# \n\
# id: Integer. The vertex id used to identify the saddle. These are numbered\n\
#     continguosly starting from the offset of the last voxel.\n\
# \n\
# after: \n\
#     Integer. The id of the vertex (voxel or saddle) immediately preceeding\n\
#     this one in the total order.\n\
# \n\
# type:\n\
#     0 = YZ face saddle\n\
#     1 = XZ face saddle\n\
#     2 = XY face saddle\n\
#     3 or 4 = Body Saddle\n\
# \n\
# corner:\n\
#     Integer. The vertex id of the voxel at the bottom left front corner of the\n\
#     face or cell containing the saddle.\n\
# \n\
# loc_x, loc_y, loc_z:\n\
#     Floats. Approximate location of the saddle. These may be exactly the same\n\
#     as the location of corner if the saddle is created by perturbation.\n\
# \n\
# value:\n\
#     Float. Approximate saddle value. This may be exactly the same as the value\n\
#     at loc if the saddle is created by perturbation.\n\
#\n\
#\n\
# IMPORTANT NOTE:\n\
# \n\
# If you just want to know approximately where saddles are and what their\n\
# values are, you don't need to pay attention to the 'after' field. If you need\n\
# to recreate the total order of graph vertices that was used to create the\n\
# tree, you must use the following total order on the voxel vertices: \n\
#     a comes before b  <==>  ( f(a) == f(b) && a < b ) || ( f(a) < f(b) ) \n\
# \n\
# The saddles are output in sorted order, so you can use the 'after' field to\n\
# merge these two sorted lists.\n\
#\n");

}

void
print_branch( ctBranch *b, int d, FILE* f)
{
    fprintf(f,"\n");
    for (int i=0; i<d; ++i) fprintf(f,"    ");
    fprintf(f,"(%u %u  ",b->extremum,b->saddle);
    for (ctBranch *c = b->children.head; c; c=c->nextChild) 
        print_branch(c,(d+1),f);
    fprintf(f,")");
}

void
print_saddles( TrilinearGraph *g, size_t nverts, size_t nsaddles, size_t order[], FILE* f)
{
    
    SaddleInfo info;
    fprintf(f,"saddles %u\n", nsaddles);
    for (size_t i=1; i < nverts; ++i ) { /* can't have saddle as first vertex */
        if ( tl_get_saddle_info(g, order[i], &info ) ) {
            fprintf(f,"%u %u %d %u %lf %lf %lf %lf\n", 
                order[i], order[i-1], info.type, info.where, 
                info.location[0], info.location[1], info.location[2],
                info.value );
        }
    }
}


void 
compute_contour_tree
(   uint8_t *data,
    size_t data_size[3],
    FILE *out )
{
    printf("sizes are %d,%d,%d\n",data_size[0],data_size[1],data_size[2]);

    size_t num_verts, *sorted_verts;

    printf("creating trilinear graph\n");
    TrilinearGraph* graph = 
        tl_create_trilinear_graph( data_size, data, &num_verts, &sorted_verts);
    
    WorkingData wd = { data, {data_size[0],data_size[1],data_size[2]}, graph }; 

    ctContext *ct = ct_init( num_verts, sorted_verts, value, neighbors, (void*)(&wd) ); 

    printf("sweep and merge\n");
    ct_sweepAndMerge(ct);
    ctBranch *root = ct_decompose(ct); 
    
    assert(root);
    print_header_comment(out);
    print_branch(root,0,out); 
    fprintf(out,"\n");
    print_saddles(graph, num_verts,
        num_verts - data_size[0]*data_size[1]*data_size[2], 
        sorted_verts, out);

    tl_cleanup(graph); 
    ct_cleanup(ct);
}


static
void
print_usage_and_exit()
{
printf( "\
tltree : compute the contour tree of a 3D image with trilinear interpolation\n\
\n\
usage: tltree ncols nrows nstacks infile [outfile]\n\
\n\
    The first three arguments give the size of the image.  The input file\n\
    should contain the image, stored as a sequence of bytes, one byte per\n\
    voxel, in normal image order (columns, then rows, then 'stacks' or images.)\n\n"
);
    exit(EXIT_FAILURE);
}


int
main (int argc, char *argv[]) 
{
    print_header_comment(stdout);
    if (argc < 5) print_usage_and_exit();
   
    int sizei[3];
    for (int i=0; i<3; ++i) sizei[i] = atoi(argv[i+1]);

    for (int i=0; i<3; ++i) {
        if (sizei[i] < 1) {
            fprintf(stderr,"Invalid size argument: %d\n",sizei[i]); 
            exit(EXIT_FAILURE);
        }
    }

    size_t size = sizei[0]*sizei[1]*sizei[2];
    size_t sizeu[3] = {sizei[0],sizei[1],sizei[2]};
   
    struct stat s;
    stat(argv[4],&s);
    
    if (s.st_size != size) {
        fprintf(stderr,"I was told the file %s would have %d*%d*%d bytes, but it does not.\n",
            argv[4],sizei[0],sizei[1],sizei[2]);
        exit(EXIT_FAILURE);
    }

    uint8_t *image = malloc(size);
    FILE *in = fopen(argv[4],"rb");
    
    if (!in) {
        fprintf(stderr,"Couldn't open file %s for reading.\n",argv[4]);
        exit(EXIT_FAILURE);
    }

    size_t nread = fread(image,1,size,in);
    if (nread != size) {
        fprintf(stderr,"Read error\n");
        exit(EXIT_FAILURE);
    }
    fclose(in);


    FILE *out = 0;
    if ( argc > 5 ) {
        out = fopen(argv[5],"w");
        if (!out) {
            fprintf(stderr,"Couldn't open file %s for writing.\n",argv[4]);
            exit(EXIT_FAILURE);
        }
    } else {
        out = stdout; 
    }

    compute_contour_tree( image, sizeu, out );
    fclose(out); 
}


