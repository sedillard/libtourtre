#include "Mesh.h"

#include <iostream>
#include <algorithm>

using std::cout;
using std::endl;
using std::sort;





class AscendingOrder 
{
    Mesh & mesh;
    public:
    AscendingOrder( Mesh & m ) : mesh(m) {}
    bool operator()(const uint & a, const uint & b) const { 
            return mesh.less( a , b );
    }
};


void Mesh::createGraph(std::vector<size_t> & order) 
{
    order.resize( width*height );
    for (uint i = 0; i < order.size(); i++) order[i] = i;
    sort( order.begin() , order.end(), AscendingOrder(*this) );
}

void Mesh::getNeighbors48(size_t i, std::vector<size_t>& n) 
{
    uint x = i%width, y = i/width;
    if ( (x+y)%2 == 1 ) {
      get4Neighbors(x,y,n);
    } else {
      get8Neighbors(x,y,n);
    }
}

void Mesh::get4Neighbors( uint x, uint y, std::vector< size_t > & neighbors )
{
  if ( x-1 < width  ) neighbors.push_back( y*width + x-1 ); 
  if ( x+1 < width  ) neighbors.push_back( y*width + x+1 ); 
  if ( y-1 < height ) neighbors.push_back( (y-1)*width + x ); 
  if ( y+1 < height ) neighbors.push_back( (y+1)*width + x ); 
}

void Mesh::get8Neighbors( uint x, uint y, std::vector< size_t > & neighbors )
{
  get4Neighbors(x,y,neighbors);
  if ( x-1 < width && y-1 < height ) neighbors.push_back( (y-1)*width + x-1 ); 
  if ( x+1 < width && y-1 < height ) neighbors.push_back( (y-1)*width + x+1 ); 
  if ( x-1 < width && y+1 < height ) neighbors.push_back( (y+1)*width + x-1 ); 
  if ( x+1 < width && y+1 < height ) neighbors.push_back( (y+1)*width + x+1 ); 
}

void Mesh::get6Neighbors( uint x, uint y, std::vector< size_t > & neighbors )
{
  get4Neighbors(x,y,neighbors);
  if ( x-1 < width && y-1 < height ) neighbors.push_back( (y-1)*width + x-1 ); 
  if ( x+1 < width && y+1 < height ) neighbors.push_back( (y+1)*width + x+1 ); 
}

