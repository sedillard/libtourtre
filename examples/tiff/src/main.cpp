#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>


extern "C" 
{
#include <tiff.h>
#include <tiffio.h>
#include <tourtre.h>
}

#include "Mesh.h"


using std::cin;
using std::cout;
using std::clog;
using std::cerr;
using std::endl;



double value ( size_t v, void * d ) {
    Mesh * mesh = reinterpret_cast<Mesh*>(d);
    return mesh->data[v];
}


size_t neighbors48 ( size_t v, size_t * nbrs, void * d ) {
    Mesh * mesh = static_cast<Mesh*>(d);
    static std::vector<size_t> nbrsBuf;
    
    nbrsBuf.clear();
    mesh->getNeighbors48(v,nbrsBuf);
    
    for (uint i = 0; i < nbrsBuf.size(); i++) {
            nbrs[i] = nbrsBuf[i]; 
    }
    return nbrsBuf.size();
}

size_t neighbors6 ( size_t v, size_t * nbrs, void * d ) {
    Mesh * mesh = static_cast<Mesh*>(d);
    static std::vector<size_t> nbrsBuf;
    
    nbrsBuf.clear();
    mesh->getNeighbors6(v,nbrsBuf);
    
    for (uint i = 0; i < nbrsBuf.size(); i++) {
            nbrs[i] = nbrsBuf[i]; 
    }
    return nbrsBuf.size();
}

void outputTree( std::ofstream & out, ctBranch * b, unsigned char * data, int thresh ) {
    out << "(" << b->extremum << ' ' << b->saddle ;
    
    for ( ctBranch * c = b->children.head; c != NULL; c = c->nextChild ){
      if ( abs(int(data[c->extremum])-int(data[c->saddle])) > thresh ) {
        out << " ";
        outputTree( out, c, data, thresh );
      }
    }
    
    out << ")";
} 


void loadTiff (const char * filename, unsigned char **pixels, int *width, int *height)
{
    TIFF* tif = TIFFOpen(filename, "r");

    if (!tif) {
            cerr << "ERR: Couldn't open " << filename << endl;
            exit(EXIT_FAILURE);
    }

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, height);
    int numPixels = (*width) * (*height);

    *pixels = new unsigned char[numPixels];
    uint32* raster = new uint32[numPixels];

    if (!raster || !pixels) {
            cerr << "ERR: Couldn't allocate enough memory" << endl;
            exit(EXIT_FAILURE);
    }

    unsigned char minVal = 255, maxVal = 0;
    if (TIFFReadRGBAImage(tif, *width, *height, raster, 0)) {
            for (int i = 0; i < numPixels; i++) {
                unsigned char b = TIFFGetR( raster[i] ) ;
                (*pixels)[i] = b;
                minVal = std::min(b,minVal);
                maxVal = std::max(b,maxVal);
            }
    } else {
            cerr << "ERR: Couldn't read " << filename << endl;
            exit(EXIT_FAILURE);
    }

    cout << "min value is " << (int)minVal << " and max is " << (int)maxVal << endl;

    delete[] raster;
    TIFFClose(tif);
}


int main( int argc, char ** argv ) 
{
    int errflg = 0;
    int c;
    
    //command line parameters
    char filename[1024] = "";
    char outfile[1024] = "";
    char switches[256] = "i:o:xp:";
    bool use48 = false;
    int thresh = 0;

    while ( ( c = getopt( argc, argv, switches ) ) != EOF ) {
      switch ( c ) {
        case 'i': 
          strcpy(filename,optarg);
          break;
        case 'o': 
          strcpy(outfile,optarg);
          break;
        case 'x': 
          use48 = true;
          break;
        case 'p':
          thresh = atoi(optarg);
          break;
        case '?':
          errflg++;
      }
    }
            
    if ( errflg || filename[0] == '\0') {
      clog << "usage: " << argv[0] << " <flags> " << endl << endl;
      
      clog << "flags" << endl;
      clog << "\t -i < filename >  :  filename" << endl;
      clog << "\t -o < filename >  :  filename" << endl;
      clog << "\t -p < threshold > : pruning threshold" << endl;
      clog << "\t -x : use 4/8/checkerboard connectivity" << endl;
      clog << "\t\tdefault is 6 connectivity" << endl;
      clog << endl;
      exit(EXIT_FAILURE);
    }



    //Load data
    unsigned char * data;
    int w,h;
    loadTiff( filename, &data,&w,&h );

    //Create mesh
    Mesh mesh(data,w,h);
    std::vector<size_t> totalOrder;
    mesh.createGraph( totalOrder ); //this just sorts the vertices according to data.less()
    
    //init countour tree library
    ctContext * ctx = 
      ct_init(
        w*h, //numVertices
        &(totalOrder.front()), //don't modify totalOrder after this!
        &value,
        use48 ? &neighbors48 : &neighbors6,
        &mesh //data for callbacks. The global functions less, value and 
              //neighbors are just wrappers which call mesh->getNeighbors, etc
    );
    
    //create contour tree
    ct_sweepAndMerge( ctx );
    ctBranch * root = ct_decompose( ctx );
    
    //create branch decomposition
    ctBranch ** map = ct_branchMap(ctx);
    ct_cleanup( ctx );
    
    //output tree
    std::ofstream out(outfile,std::ios::out);
    if (out) {
            outputTree( out, root, data, thresh);
    } else {
            cerr << "ERR: couldn't open output file " << outfile << endl;
    }
	
}



	
