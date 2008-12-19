#include "Mesh.h"

#include <iostream>
#include <algorithm>

using std::cout;
using std::endl;
using std::sort;





//functor for sorting
class AscendingOrder 
{
	Data & data;
	public:
	AscendingOrder( Data & d ) : data(d) {}
	bool operator()(const uint & a, const uint & b) const { 
		return data.less( a , b );
	}
};


void Mesh::createGraph(std::vector<size_t> & order) 
{
	order.resize( data.totalSize );
	
	for (uint i = 0; i < order.size(); i++) 
		order[i] = i;
	
	sort( order.begin() , order.end(), AscendingOrder(data) );
}

void Mesh::getNeighbors(size_t i, std::vector<size_t>& n) 
{
	uint x,y,z;
	data.convertIndex( i, x, y, z );
	if ( (x+y+z)%2 == ODD_TET_PARITY ) {
		find6Neighbors(x,y,z,n);
	} else {
		find18Neighbors(x,y,z,n);
	}
}


void Mesh::find6Neighbors( uint x, uint y, uint z, std::vector< size_t > & neighbors) 
{
	uint nx[6],ny[6],nz[6];
	
	for (uint i = 0; i < 6; i++) {
		nx[i] = x;
		ny[i] = y;
		nz[i] = z;
	}
	
	//first 6 neighbors
	nx[0] -= 1;
	ny[1] -= 1;
	nz[2] -= 1;
	nx[3] += 1;
	ny[4] += 1;
	nz[5] += 1;
	
	for (uint i = 0; i < 6; i++) {
		if (nx[i] >= data.size[0]) continue;	
		if (ny[i] >= data.size[1]) continue;	
		if (nz[i] >= data.size[2]) continue;	
	
		neighbors.push_back( data.convertIndex(nx[i],ny[i],nz[i]) );
	}
}

void Mesh::find18Neighbors( uint x, uint y, uint z, std::vector< size_t > & neighbors) 
{
	uint nx[18],ny[18],nz[18];
	
	for (uint i = 0; i < 18; i++) {
		nx[i] = x;
		ny[i] = y;
		nz[i] = z;
	}
	
	//first 6 neighbors
	nx[0] -= 1;
	ny[1] -= 1;
	nz[2] -= 1;
	nx[3] += 1;
	ny[4] += 1;
	nz[5] += 1;
	
	//the rest of the 18
	nx[6] -= 1; ny[6]  -= 1;
	nx[7] += 1; ny[7]  -= 1;
	ny[8] -= 1; nz[8]  -= 1;
	ny[9] += 1; nz[9]  -= 1;
	nz[10]-= 1; nx[10] -= 1;
	nz[11]+= 1; nx[11] -= 1;
		
	nx[12] -= 1; ny[12] += 1;
	nx[13] += 1; ny[13] += 1;
	ny[14] -= 1; nz[14] += 1;
	ny[15] += 1; nz[15] += 1;
	nz[16] -= 1; nx[16] += 1;
	nz[17] += 1; nx[17] += 1;
	
	for (uint i = 0; i < 18; i++) {
		
		
		if (nx[i] >= data.size[0]) continue;	
		if (ny[i] >= data.size[1]) continue;	
		if (nz[i] >= data.size[2]) continue;	
	
		neighbors.push_back( data.convertIndex(nx[i],ny[i],nz[i]) );
	}
}
