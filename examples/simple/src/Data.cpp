#include "Data.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

#if USE_ZLIB
#include <zlib.h>
#endif


bool Data::less( uint a, uint b ) 
{
	//use overloaded [] operator to access saddles
	if (compareEqual( (*this)[a],(*this)[b])) return a < b;
	else return compareLess((*this)[a],(*this)[b]);
}

bool Data::greater( uint a, uint b ) 
{
	if (compareEqual((*this)[a],(*this)[b])) return a > b;
	else return compareLess((*this)[b],(*this)[a]);
}

#if defined(WIN32)
	#define DIR_SYMBOL "\\"
#else
	#define DIR_SYMBOL "/"
#endif

bool Data::load(const char * file, char * prefix, bool * compressed) {

	char fullPath[1024];
	strcpy(fullPath,file);
	
	//scan for actual filename, rather than full path
	char * filename = fullPath;
	while(true) {
		char * x = strstr(filename,DIR_SYMBOL);
		if (x == '\0') break;
		else {
			filename = x;
			filename++;
		}
	}
	
	char * dimP, * typeP, *gzP;

	dimP = strstr(filename,".");
	*dimP = '\0';
	dimP++;

	typeP = strstr(dimP,".");
	*typeP = '\0';
	typeP++;

	gzP = strstr(typeP,".");
	if (gzP) {
		*gzP = '\0';
		gzP++;
		*compressed = true;
	}
	
	char *xSize, *ySize, *zSize;
	
	xSize = dimP;
	
	ySize = strstr(dimP,"x");
	*ySize = '\0';
	ySize++;

	zSize = strstr(ySize,"x");
	*zSize = '\0';
	zSize++;
	
	size[0] = atoi(xSize);
	size[1] = atoi(ySize);
	size[2] = atoi(zSize);

	std::clog << "Loaing " << filename << " with dimensions " << size[0] << " x " << size[1] << " x " << size[2] << ", of type " << typeP << std::endl;
	strcpy(prefix,fullPath);

	totalSize = size[0] * size[1] * size[2];


	std::ifstream infile;
	gzFile zinfile;

	#if USE_ZLIB
		zinfile = gzopen(file,"rb");
		if (!zinfile) {
			std::cerr << "could not open" << filename << std::endl;
			return false;
		}
	#else
		infile.open(file,std::ios::in);
		if (!infile) {
			std::clog << "could not open " << filename << std::endl;
			return false;
		}
	#endif

	data = new DataType[totalSize];

	if ( strcmp(typeP,"uint8") == 0 ) {

		unsigned char * charData = new unsigned char [totalSize];

		#if USE_ZLIB
		gzread( zinfile , reinterpret_cast<char*>(charData) , totalSize );
		#else
		infile.read( reinterpret_cast<char*>(charData) , totalSize );
		#endif

		for (uint i = 0; i < totalSize; i++) {
			data[i] = static_cast<DataType>( charData[i] );
		}

		delete[] charData;

	} else if ( strcmp(typeP,"uint16") == 0 ) {

		unsigned short int * intData = new unsigned short int [totalSize];

		#if USE_ZLIB
		gzread( zinfile , reinterpret_cast<char*>(intData) , totalSize * 2 );
		#else
		infile.read( reinterpret_cast<char*>(intData) , totalSize * 2 );
		#endif

		for (uint i = 0; i < totalSize; i++) {
			data[i] = static_cast<DataType>( intData[i] );
		}

		delete[] intData;

	} else if ( strcmp(typeP,"float") == 0 ) {

		float * floatData = new float [totalSize];

		#if USE_ZLIB
		gzread( zinfile , reinterpret_cast<char*>(floatData) , totalSize * 4 );
		#else
		infile.read( reinterpret_cast<char*>(floatData) , totalSize * 4 );
		#endif

		for (uint i = 0; i < totalSize; i++) {
			data[i] = static_cast<DataType>( floatData[i] );
		}

		delete[] floatData;

	} else if ( strcmp(typeP,"double") == 0 ) {

		double * doubleData = new double [totalSize];

		#if USE_ZLIB
		gzread( zinfile , reinterpret_cast<char*>(doubleData) , totalSize * 8 );
		#else
		infile.read( reinterpret_cast<char*>(doubleData) , totalSize * 8 );
		#endif

		for (uint i = 0; i < totalSize; i++) {
			data[i] = static_cast<DataType>( doubleData[i] );
		}

		delete[] doubleData;

	} else {

		std::cerr << "unrecognized type : " << typeP << std::endl;
		return false;
	}


	maxValue = minValue = data[0];
	for (uint i = 0; i < totalSize; i++) {
		if (data[i] > maxValue) maxValue = data[i];
		if (data[i] < minValue) minValue = data[i];
	}

	std::clog << "max value was " << (int)maxValue << " and min value was " << (int)minValue << std::endl;

	#if USE_ZLIB
	gzclose( zinfile );
	#else
	infile.close();
	#endif
	
	

	return true;
		
}
