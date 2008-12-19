#ifndef MESH_H
#define MESH_H

#include <vector>

#include "Global.h"
#include "Data.h"

//abstract mesh class
class Mesh 
{
	public:
	
	Data & data;
	Mesh(Data & d) : data(d) {}
	
	void getNeighbors(size_t i, std::vector<size_t> & n);
	void createGraph(std::vector<size_t> & order);
	uint numVerts();
	
	

	void find6Neighbors( uint x, uint y, uint z, std::vector< size_t > & neighbors);
	void find18Neighbors( uint x, uint y, uint z, std::vector< size_t > & neighbors);
	
};

#endif
