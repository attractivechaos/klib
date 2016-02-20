#ifndef MATRICES_H
#define MATRICES_H

#include <stdlib.h>
#include <stdint.h>

#define KMATRIX_SIZE_MAX 50 

#define KMATRIX_IDENTITY 1
#define KMATRIX_ZERO 0

#define KMATRIX_TRANSPOSE 1
#define KMATRIX_KEEP 0

struct _kmatrix_data_t;

typedef struct _kmatrix_data_t kmatrix_data_t;

typedef kmatrix_data_t* kmatrix_t;


kmatrix_t km_create( double*, size_t, size_t );
kmatrix_t km_create_square( size_t, uint8_t ); 
void km_free( kmatrix_t );     
double km_get( kmatrix_t, size_t, size_t );            
double* km_to_array( kmatrix_t );                             
void km_set( kmatrix_t, size_t, size_t, double );      
kmatrix_t km_clear( kmatrix_t );                                 
kmatrix_t km_resize( kmatrix_t, size_t, size_t );                
kmatrix_t km_scale( kmatrix_t, double, kmatrix_t );                 
kmatrix_t km_sum( kmatrix_t, double, kmatrix_t, double, kmatrix_t );       
kmatrix_t km_dot( kmatrix_t, uint8_t, kmatrix_t, uint8_t, kmatrix_t );       
double km_det( kmatrix_t );                           
kmatrix_t km_trans( kmatrix_t, kmatrix_t );                     
kmatrix_t km_inv( kmatrix_t, kmatrix_t );                       
void km_print( kmatrix_t );                                     


#endif // MATRICES_H
