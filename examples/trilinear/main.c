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
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <zlib.h>

#include <tourtre.h>
#include <ctBranch.h>

#include "trilinear.h"

typedef
struct WorkingData_ {
  Values data;
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
print_header_comment (void *f, void (*writer)(void*,const char*,...) )
{

(*writer)(f,"\
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
# saddle_rec : id after type corner loc_x loc_y loc_z value\n\
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
print_branch( ctBranch *b, int d, void* file, void (*writer)(void*,const char*,...) )
{
    (*writer)(file,"\n");
    for (int i=0; i<d; ++i) (*writer)(file,"    ");
    size_t nchildren=0;
    for (ctBranch *c = b->children.head; c; c=c->nextChild) ++nchildren;
    (*writer)(file,"(%u %u %u  ",b->extremum,b->saddle,nchildren);
    for (ctBranch *c = b->children.head; c; c=c->nextChild) 
        print_branch(c,(d+1),file,writer);
    (*writer)(file,")");
}

void
print_saddles
(   TrilinearGraph *g, 
    size_t nverts, 
    size_t nsaddles, 
    size_t order[], 
    void *file,
    void (*writer)(void*,const char*,...) )
{
    SaddleInfo info;
    (*writer)(file,"saddles %u\n", nsaddles);
    for (size_t i=1; i < nverts; ++i ) { /* can't have saddle as first vertex */
        if ( tl_get_saddle_info(g, order[i], &info ) ) {
            (*writer)(file,"%u %u %d %u %lf %lf %lf %lf\n", 
                order[i], order[i-1], info.type, info.where, 
                info.location[0], info.location[1], info.location[2],
                info.value );
        }
    }
}






/* 
 * binary output 
 * 
 * the format is identical to the ascii output, except that integers and floats
 * are stored directly, as 32bit quantities.
 *
 */

void
print_branch_binary
(   ctBranch *b, 
    int d, 
    void* file, 
    void (*writer)(void*,void*,size_t) )
{
    size_t nchildren=0;
    for (ctBranch *c = b->children.head; c; c=c->nextChild) ++nchildren;
    int32_t out[3];
    out[0] = b->extremum;
    out[1] = b->saddle;
    out[2] = nchildren;
    (*writer)(file,out,sizeof(int32_t)*3);
    for (ctBranch *c = b->children.head; c; c=c->nextChild) {
        print_branch_binary(c,(d+1),file,writer);
    }
}

void
print_saddles_binary
(   TrilinearGraph *g, 
    size_t nverts, 
    size_t nsaddles, 
    size_t order[], 
    void* file, 
    void (*writer)(void*,void*,size_t))
{
    SaddleInfo info;
    
    {   int32_t nsaddles_out = nsaddles;
        (*writer)(file,&nsaddles_out,sizeof(int32_t));
    }

    for (size_t i=1; i < nverts; ++i ) { /* can't have saddle as first vertex */
        if ( tl_get_saddle_info(g, order[i], &info ) ) {
            int32_t ints[4] = { order[i]
                              , order[i-1]
                              , info.type
                              , info.where };
            float floats[4] = { info.location[0]
                              , info.location[1]
                              , info.location[2] 
                              , info.value };
            (*writer)(file,ints,sizeof(int32_t)*4);
            (*writer)(file,floats,sizeof(int32_t)*4);
        }
    }
}


static
void
print_usage_and_exit()
{
printf( "\n\
tltree : compute the contour tree of a 3D image with trilinear interpolation\n\
\n\
usage: tltree [options] ncols nrows nstacks infile [outfile]\n\
\n\
    The first three arguments give the size of the image.  The input file\n\
    should contain the image, stored as a sequence of bytes, one byte per\n\
    voxel, in normal image order (columns, then rows, then 'stacks' or images.)\
\n\n\
    Options:\n\
        -z : gzipped output\n\n\
        -b : binary output (default is ascii)\n\n"
);
    exit(EXIT_FAILURE);
}


//need to slightly re-arrange fwrite arguments to match gzwrite
int
fwrite_ ( FILE *f, void *d, size_t s )
{ return fwrite(d,s,1,f); }



int
main (int argc__, char *argv__[]) 
{
    bool binary=false;
    bool zipped=false;
   
    int argc = argc__;
    char **argv = argv__;
    {   int opt;
        while ((opt=getopt(argc__,argv__,"zb"))!=-1) {
            switch(opt) {
            case 'z':
                zipped=true;
                --argc;
                ++argv;
                break;
            case 'b':
                binary=true;
                --argc;
                ++argv;
                break;
            default :
                print_usage_and_exit();
            }
        }
    }

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
    uint32_t sizeu[3] = {sizei[0],sizei[1],sizei[2]};
   
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


    void *out = 0;
    if ( argc > 5 ) {
        if (zipped) {
            if (binary) 
                out = gzopen(argv[5],"wb");
            else 
                out = gzopen(argv[5],"w");
        } else {
            if (binary)
                out = fopen(argv[5],"wb");
            else 
                out = fopen(argv[5],"w");
        }

        if (!out) {
            fprintf(stderr,"Couldn't open file %s for writing.\n",argv[4]);
            exit(EXIT_FAILURE);
        }
    } else {
        if (binary || zipped) {
            fprintf(stderr,"Surely you don't mean to print binary/compressed output to stdout.\n");
            exit(EXIT_FAILURE);
        }
        out = stdout; 
    }

    { //compute and output the contour tree
        size_t num_verts, *sorted_verts;

        TrilinearGraph* graph = 
            tl_create_trilinear_graph( sizeu, image, &num_verts, &sorted_verts);
        
        WorkingData wd = { image, graph }; 

        ctContext *ct = ct_init( num_verts, sorted_verts, value, neighbors, (void*)(&wd) ); 

        //ctArc *orig = 
           ct_sweepAndMerge(ct);

        //test ct_copyTree and ct_deleteTree
        //ctArc *copy = ct_copyTree( orig,0,ct );
        //ct_deleteTree(copy,ct);
        
        ctBranch *root = ct_decompose(ct); 
        
        assert(root);
        if (!binary) {
            void *writer; 
            //types of gzprintf and fprintf really are compatible
            //the compiler just doesn't realize it.
            if (zipped) writer = gzprintf;
            else writer = fprintf;
            
            print_header_comment(out,writer);
            print_branch(root,0,out,writer); 
            (*((void (*)(void*,const char*,...))writer))(out,"\n");
            print_saddles(graph, num_verts,
                num_verts - sizeu[0]*sizeu[1]*sizeu[2], 
                sorted_verts, out,writer);
            
        } else {
            void *writer;
            if (zipped) writer = gzwrite;
            else writer = fwrite_;
            
            print_branch_binary(root,0,out,writer); 
            print_saddles_binary(graph, num_verts,
                num_verts - sizeu[0]*sizeu[1]*sizeu[2], 
                sorted_verts, out,writer);
        }

        ctBranch_delete(root,ct);    
        tl_cleanup(graph); 
        ct_cleanup(ct);
    } 
    
    if ( zipped ) gzclose(out);
    else fclose(out);
}


