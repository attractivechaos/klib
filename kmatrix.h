#ifndef KMATRIX_H
#define KMATRIX_H

#include <stdlib.h>
#include <stdbool.h>

#define KMATRIX_SIZE_MAX 50 

#define KMATRIX_IDENTITY 'I'
#define KMATRIX_ZERO '0'

#define KMATRIX_TRANSPOSE 'T'
#define KMATRIX_KEEP 'N'


/** Matrix internal data structure */
struct _kmatrix_data_t;
typedef struct _kmatrix_data_t kmatrix_data_t;

/** Matrix data opaque type. Protects real data */
typedef kmatrix_data_t* kmatrix_t;

/**
 * Create matrix with specified values and dimensions
 * 
 * @param data array with values in row-major order to fill matrix data (NULL for filling with zeros)
 * @param n_rows number of rows
 * @param n_cols number of columns
 * 
 * @return reference/pointer to allocated and filled matrix. NULL if dimensions are bigger than KMATRIX_SIZE_MAX
 */
kmatrix_t km_create( double* data, size_t n_rows, size_t n_cols );

/**
 * Create square matrix of specified size and type
 * 
 * @param size size/order of the square matrix (equal number of rows and cells)
 * @param type defines if internal data is filled as zero (KMATRIX_ZERO) or identity (KMATRIX_IDENTITY) matrix
 * 
 * @return reference/pointer to allocated and filled matrix. NULL if size is bigger than KMATRIX_SIZE_MAX
 */
kmatrix_t km_create_square( size_t size, char type ); 

/**
 * Destroy/Deallocate memory of matrix
 * 
 * @param mat matrix to be destroyed/deallocated
 */
void km_free( kmatrix_t mat );

/**
 * Copy content from one matrix to another, previously allocated
 * 
 * @param src matrix from which data will be copied (source)
 * @param dst matrix to which data will be copied (destination)
 * 
 * @return reference/pointer to destination matrix. NULL on error
 */
kmatrix_t km_copy( kmatrix_t src, kmatrix_t dst );

/**
 * Set all elements of given matrix to zero
 * 
 * @param mat matrix to be cleared/zeroed
 * 
 * @return reference/pointer to cleared matrix
 */
kmatrix_t km_clear( kmatrix_t mat );

/**
 * Compare 2 matrices values for equality (considering a certain tolerance, because of floating-point precision limit)
 * 
 * @param mat_1 first matrix
 * @param mat_2 second matrix (should have same dimensions of first one) 
 * 
 * @return true if all corresponding values are equal (inside tolerance), false otherwise (or in case of errors)
 */
bool km_equal( kmatrix_t mat_1, kmatrix_t mat_2 );

/**
 * Get columns number for give matrix
 * 
 * @param mat matrix provided
 * 
 * @return number of columns for the matrix (0 on error)
 */
size_t km_width( kmatrix_t mat );

/**
 * Get rows number for give matrix
 * 
 * @param mat matrix provided
 * 
 * @return number of rows for the matrix (0 on error)
 */
size_t km_height( kmatrix_t mat );

/**
 * Get array of given matrix values in row-major order
 * 
 * @param mat given matrix
 * @param buf allocated array/buffer to store matrix values data (in row-major order)
 * 
 * @return pointer to filled buffer. NULL on error
 */
double* km_get_data( kmatrix_t mat, double* buf );

/**
 * Set given matrix values from row-major order data array
 * 
 * @param mat given matrix
 * @param data row-major order data array for filling the matrix
 */
void km_set_data( kmatrix_t mat, double* data );

/**
 * Get value of given matrix element at specified position
 * 
 * @param mat given matrix
 * @param row row position of accessed element
 * @param col column position of accessed element
 * 
 * @return value of specified element. 0 on error
 */
double km_get_at( kmatrix_t mat, size_t row, size_t col );           

/**
 * Set value of given matrix element at specified position
 * 
 * @param mat given matrix
 * @param row row position of accessed element
 * @param col column position of accessed element
 * @param val new value of accessed element
 */                            
void km_set_at( kmatrix_t mat, size_t row, size_t col, double val ); 

/**
 * Resize/reallocate given matrix to specified dimensions. Fill new space with zeros when growing
 * 
 * @param mat matrix to be resized
 * @param n_rows new number of rows
 * @param n_cols new number of columns
 * 
 * @return reference/pointer to resized/reallocated matrix. NULL on errors
 */                              
kmatrix_t km_resize( kmatrix_t mat, size_t n_rows, size_t n_cols );  

/**
 * Multiply all given matrix elements by a specified value
 * 
 * @param mat matrix to be scaled
 * @param factor scalar value that multiplies the matrix
 * @param rslt preallocated matrix to store the scaling result (can be the same as the input one)
 * 
 * @return reference/pointer to scaled matrix (result). NULL on errors
 */  
kmatrix_t km_scale( kmatrix_t mat, double factor, kmatrix_t rslt );

/**
 * Perform element-wise weighted sum of 2 matrices
 * 
 * @param mat_1 first matrix
 * @param wgt_1 weight of first matrix on sum
 * @param mat_2 second matrix (should have same dimensions of first one) 
 * @param wgt_2 weight of second matrix on sum
 * @param rslt preallocated matrix to store the sum result
 * 
 * @return reference/pointer to sum result matrix. NULL on errors
 */  
kmatrix_t km_sum( kmatrix_t mat_1, double wgt_1, kmatrix_t mat_2, double wgt_2, kmatrix_t rslt ); 

/**
 * Perform dot product/multiplication of 2 matrices
 * 
 * @param mat_1 first matrix (nxk dimensions after transpose/keep)
 * @param trs_1 defines if first matrix is transposed (KMATRIX_TRANSPOSE) or not (KMATRIX_KEEP) before operation 
 * @param mat_2 second matrix (kxm dimensions after transpose/keep) 
 * @param trs_2 defines if second matrix is transposed (KMATRIX_TRANSPOSE) or not (KMATRIX_KEEP) before operation 
 * @param rslt preallocated matrix to store the dot product/multiplication result (nxm dimensions)
 * 
 * @return reference/pointer to dot product/multiplication result matrix. NULL on errors
 */ 
kmatrix_t km_dot( kmatrix_t mat_1, char trs_1, kmatrix_t mat_2, char trs_2, kmatrix_t rslt ); 

/**
 * Calculate determinant of given matrix
 * 
 * @param mat matrix to be used on determinant calculation
 * 
 * @return determinant value (0 on errors)
 */
double km_det( kmatrix_t mat );       

/**
 * Transpose given matrix
 * 
 * @param mat matrix to be trasnposed (nxm dimensions)
 * @param rslt preallocated matrix (n*m data size) to store the transposition result (can be the same as the input one)
 * 
 * @return reference/pointer to transposed matrix (result). NULL on errors
 */ 
kmatrix_t km_trans( kmatrix_t mat, kmatrix_t rslt );                     

/**
 * Calculate inverse of given square matrix
 * 
 * @param mat matrix to be inverted (should be square)
 * @param rslt preallocated matrix to store the inversion result (can be the same as the input one)
 * 
 * @return reference/pointer to inverted matrix (result). NULL on errors
 */ 
kmatrix_t km_inv( kmatrix_t mat, kmatrix_t rslt );                       

/**
 * Print given matrix element values in a formatted way
 * 
 * @param mat matrix to be displayed
 */
void km_print( kmatrix_t mat );                                     


#endif // KMATRIX_H
