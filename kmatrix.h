#ifndef KMATRIX_H
#define KMATRIX_H

#include <stdint.h>
#include <stdbool.h>

#define KMATRIX_SIZE_MAX 50 

#define KMATRIX_IDENTITY 'I'
#define KMATRIX_ZERO '0'

#define KMATRIX_TRANSPOSE 'T'
#define KMATRIX_KEEP 'N'


// Matrix data opaque type. Protect real data
struct _kmatrix_data_t;
typedef struct _kmatrix_data_t kmatrix_data_t;
typedef kmatrix_data_t* kmatrix_t;


// Create matrix with "n_rows" rows and "n_cols" columns
// Filled (row by row) with values from "data" (NULL for filling with zeros)
kmatrix_t km_create( double* data, size_t n_rows, size_t n_cols );

// Create square matrix of size "size". Diagonal filled with "d_value"
kmatrix_t km_create_square( size_t size, char type ); 

// Deallocate memory of matrix "mat"
void km_free( kmatrix_t mat );

// Copy content from matrix "src" to (preallocated) matrix "dst"
kmatrix_t km_copy( kmatrix_t src, kmatrix_t dst );

// Clear content of matrix "mat" and return it
kmatrix_t km_clear( kmatrix_t mat );

// Verify it matrices "mat_1" and "mat_2" have the same values, inside a little tolerance
bool km_equal( kmatrix_t mat_1, kmatrix_t mat_2 );

// Get columns number for matrix "mat"
size_t km_width( kmatrix_t mat );

// Get rows number for matrix "mat"
size_t km_height( kmatrix_t mat );

// Return pointer to matrix "mat" values array
// Copied to buffer "buf" before if not NULL
double* km_get_data( kmatrix_t mat, double* buf );

// Copies data from buffer "buf" (if not NULL) to internal values array of matrix "mat"
void km_set_data( kmatrix_t mat, double* data );

// Get value at row "row" and column "col" of matrix "mat"
double km_get_at( kmatrix_t mat, size_t row, size_t col );           

// Set value at row "row" and column "col" of matrix "mat" to "val"                             
void km_set_at( kmatrix_t mat, size_t row, size_t col, double val ); 

// Resize matrix "mat" to "n_rows" rows and "n_cols" columns
// Save result on "rslt" and return it                                
kmatrix_t km_resize( kmatrix_t mat, size_t n_rows, size_t n_cols, kmatrix_t rslt );  

// Multiply matrix "mat" by scalar "factor"
// Save result on "rslt" and return it 
kmatrix_t km_scale( kmatrix_t mat, double factor, kmatrix_t rslt );

// Sum matrices "mat_1" and "mat_2" 
// Parcels weighted by wgt_1 and wgt_2
// Save result on "rslt" and return it 
kmatrix_t km_sum( kmatrix_t mat_1, double wgt_1, kmatrix_t mat_2, double wgt_2, kmatrix_t rslt ); 

// Dot product of matrices "mat_1" and "mat_2" 
// Set trs_{1,2} to transpose or not the previous matrix
// Save result on "rslt" and return it 
kmatrix_t km_dot( kmatrix_t mat_1, char trs_1, kmatrix_t mat_2, char trs_2, kmatrix_t rslt ); 

// Calculate determinant of matrix "mat"
double km_det( kmatrix_t mat );       

// Transpose matrix "mat" 
// Save result on "rslt" and return it 
kmatrix_t km_trans( kmatrix_t mat, kmatrix_t rslt );                     

// Invert matrix "mat" 
// Save result on matrix "rslt" and return it 
kmatrix_t km_inv( kmatrix_t mat, kmatrix_t rslt );                       

// Formatted print of matrix "mat"
void km_print( kmatrix_t mat );                                     


#endif // KMATRIX_H
