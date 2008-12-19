#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef NULL
#define NULL 0
#endif

#define EVEN_TET_PARITY 0
#define ODD_TET_PARITY 1

#define NOTHING 0xffffffff

typedef unsigned int uint;



//typedef float DataType; //data is stored like this
//#define ABS(x) fabs(x)

typedef unsigned char DataType;
//typedef unsigned short DataType;
#define ABS(x) abs(x)

#define MAKEINT(x) x
//define MAKEINT(x) (int(x*0xffff))

inline bool compareLess( unsigned char a, unsigned char b ) { return a < b; }
//inline bool compareLess( unsigned short int a, unsigned short int b ) { return a < b; }
//inline bool compareLess( float a, float b ) { return a < b; }
//inline bool compareLess( double a, double b ) { return a < b; }

inline bool compareEqual( unsigned char a, unsigned char b ) { return a == b; }
//inline bool compareEqual( unsigned short int a, unsigned short int b ) { return a == b; }
//inline bool compareEqual( float a, float b ) { return a == b; }
//inline bool compareEqual( double a, double b ) { return a == b; }

//#define INTEGRITY_CHECKS


#endif


