#ifndef MATRICES_H
#define MATRICES_H

#include <stdlib.h>
#include <stdbool.h>

#define KMATRIX_SIZE_MAX 50 

// Matrix data opaque type. Protect real data
struct _kmatrix_data_t;

typedef struct _kmatrix_data_t kmatrix_data_t;

typedef kmatrix_data_t* kmatrix_t;


// Create matrix with "n_rows" rows and "n_cols" columns. Filled (row by row) with values from "data" (NULL for zeros)
kmatrix_t km_create( size_t n_rows, size_t n_cols, double* data );

// Create square matrix of size "size". Diagonal filled with "d_value"
kmatrix_t km_create_square( size_t size, double d_value ); 

// Fill matrix "matrix" (row by row) with values from "data" (NULL for zeros)
void km_fill( kmatrix_t matrix, double* data );

// Deallocate memory of "matrix"
void km_free( kmatrix_t matrix );

// Compare "matrix_1" to "matrix_2"
bool km_equal( kmatrix_t matrix_1, kmatrix_t matrix_2 );

// Get value at row "row" and column "col" of "matrix"
double km_get_at( kmatrix_t matrix, size_t row, size_t col );           

// Set value at row "row" and column "col" of "matrix" to "value"                             
void km_set_at( kmatrix_t matrix, size_t row, size_t col, double value ); 

// Reshape matrix "matrix" to "n_rows" rows and "n_cols" columns. Save result on "result" and return it                                
kmatrix_t km_reshape( kmatrix_t matrix, size_t n_rows, size_t n_cols, kmatrix_t result );  

// Multiply matrix "matrix" by scalar "factor". Save result on "result" and return it 
kmatrix_t km_scale( kmatrix_t matrix, double factor, kmatrix_t result );

// Sum matrix_1 and matrix_2 (weighted by weight_1 and weight_2). Save result on "result" and return it 
kmatrix_t km_sum( kmatrix_t matrix_1, double weight_1, kmatrix_t matrix_2, double weight_2, kmatrix_t result ); 


// Dot product of matrix_1 and matrix_2 (trans_{1,2} to transpose matrix before). Save result on "result" and return it 
#define KMATRIX_TRANSPOSE true
#define KMATRIX_KEEP false
kmatrix_t km_dot( kmatrix_t matrix_1, bool trans_1, kmatrix_t matrix_2, bool trans_2, kmatrix_t result ); 

// Determinant of matrix "matrix"
double km_det( kmatrix_t matrix );       

// Transpose matrix "matrix". Save result on "result" and return it 
kmatrix_t km_trans( kmatrix_t matrix, kmatrix_t result );                     

// Invert matrix "matrix". Save result on "result" and return it 
kmatrix_t km_inv( kmatrix_t matrix, kmatrix_t result );                       

// Formatted print of matrix "matrix"
void km_print( kmatrix_t matrix );                                     


#endif // MATRICES_H
