#ifndef MESH_H
#define MESH_H

#include <vector>

#include "Global.h"

struct Mesh 
{
    unsigned char * data;
    uint width, height;
    
    Mesh(unsigned char *d, uint w, uint h) : data(d),width(w),height(h) {}
    
    void getNeighbors6(size_t i, std::vector<size_t> & n) { get6Neighbors(i%width,i/width,n); }
    void getNeighbors48(size_t i, std::vector<size_t> & n);
    void createGraph(std::vector<size_t> & order);
    
    void get4Neighbors( uint x, uint y, std::vector< size_t > & neighbors);
    void get8Neighbors( uint x, uint y, std::vector< size_t > & neighbors);
    void get6Neighbors( uint x, uint y, std::vector< size_t > & neighbors);

    bool less(uint a, uint b) { return (data[a]==data[b])? a<b : data[a]<data[b]; }
	
};

#endif
