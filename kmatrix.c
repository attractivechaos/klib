#include "kmatrix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Fortran77 function declarations

// (BLAS) mat-matrix product
extern void dgemm_( char* tA, char* tB, int* m, int* n, int* k, double* alpha, double* A, int* ldA, double* B, int* ldB, double* beta, double* C, int* ldC );  
// (LAPACK) LU decomoposition of a general mat
extern void dgetrf_( int* M, int *N, double* A, int* ldA, int* IPIV, int* INFO );
// (LAPACK) generate inverse of a mat given its LU decomposition
extern void dgetri_( int* N, double* A, int* ldA, int* IPIV, double* WORK, int* lwork, int* INFO );


struct _kmatrix_data_t
{
  double* data;
  size_t n_rows, n_cols;
};                                  


kmatrix_t km_create( double* data, size_t n_rows, size_t n_cols )
{
  if( n_rows > KMATRIX_SIZE_MAX || n_cols > KMATRIX_SIZE_MAX ) return NULL;

  kmatrix_t new_mat = (kmatrix_t) malloc( sizeof(kmatrix_data_t) );

  new_mat->data = (double*) calloc( n_rows * n_cols, sizeof(double) );

  new_mat->n_rows = n_rows;
  new_mat->n_cols = n_cols;

  if( data == NULL ) km_clear( new_mat );
  else km_set_data( new_mat, data )

  return new_mat;
}

kmatrix_t km_create_square( size_t size, double type )
{
  kmatrix_t new_sq_mat = km_create( NULL, size, size );

  if( type == 'I' )
  {
    for( size_t line = 0; line < size; line++ )
      new_sq_mat->data[ line * size + line ] = 1.0;
  }

  return new_sq_mat;
}

void km_free( kmatrix_t mat )
{
  if( mat == NULL ) return;
  
  free( mat->data );
  
  free( mat );
}

kmatrix_t km_copy( kmatrix_t src, kmatrix_t dst )
{
  if( src == NULL || dst == NULL ) return NULL;

  dst->n_rows = src->n_rows;
  dst->n_cols = src->n_cols;

  memcpy( dst->data, src->data, dst->n_rows * dst->n_cols * sizeof(double) );

  return dst;
}

kmatrix_t km_clear( kmatrix_t mat )
{
  if( mat == NULL ) return NULL;

  memset( mat->data, 0, mat->n_rows * mat->n_cols * sizeof(double) );

  return mat;
}

bool km_equal( kmatrix_t mat_1, kmatrix_t mat_2 )
{
  const double TOLERANCE = 0.0001;
  
  if( mat_1->n_rows != mat_2->n_rows ) return false;
  if( mat_1->n_cols != mat_2->n_cols ) return false;
  
  for( size_t row = 0; row < mat_1->n_rows; row++ )
  {
    for( size_t col = 0; col < mat_1->n_cols; col++ )
    {
      double diff = mat_1->data[ row * mat_1->n_cols + col ] - mat_2->data[ row * mat_2->n_cols + col ];
      
      if( fabs( diff ) > TOLERANCE ) return false;
    }
  }
  
  return true;
}

size_t km_width( kmatrix_t mat )
{
  if( mat == NULL ) return 0;

  return mat->n_cols;
}

size_t km_height( kmatrix_t mat );
{
  if( mat == NULL ) return 0;

  return mat->n_rows;
}

double* km_get_data( kmatrix_t mat, double* buf )
{
  if( mat == NULL ) return NULL;

  if( buf != NULL )
  {
    //memcpy( buf, mat->data, mat->n_rows * mat->n_cols * sizeof(double) );
    for( size_t row = 0; row < mat->n_rows; row++ )
    {
      for( size_t col = 0; col < mat->n_cols; col++ )
        buf[ row * mat->n_cols + col ] = mat->data[ col * mat->n_rows + row ];
    }
    
    return buf;
  }
  
  return mat->data;
}

void km_set_data( kmatrix_t mat, double* data );
{
  if( mat == NULL ) return;

  if( buf == NULL ) return;
  
  //memcpy( mat->data, data, mat->n_rows * mat->n_cols * sizeof(double) );
  for( size_t col = 0; col < mat->n_cols; col++ )
  {
    for( size_t row = 0; row < mat->n_rows; row++ )
      mat->data[ col * mat->n_rows + row ] = data[ row * mat->n_cols + col ];
  }
}

double km_get_at( kmatrix_t mat, size_t row, size_t col )
{
  if( mat == NULL ) return 0.0;

  if( row >= mat->n_rows || col >= mat->n_cols ) return 0.0;

  return mat->data[ row * mat->n_cols + col ];
}

void km_set_at( kmatrix_t mat, size_t row, size_t col, double value )
{
  if( mat == NULL ) return;

  if( row >= mat->n_rows || col >= mat->n_cols ) return;

  mat->data[ row * mat->n_cols + col ] = value;
}

kmatrix_t km_resize( kmatrix_t mat, size_t n_rows, size_t n_cols )
{
  double aux[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];
  
  if( mat == NULL )
    mat = Matrices_Create( NULL, n_rows, n_cols );
  else 
  {
    if( mat->n_rows * mat->n_cols < n_rows * n_cols )
      mat->data = (double*) realloc( mat->data, n_rows * n_cols * sizeof(double) );
  
    memcpy( aux, mat->data, mat->n_rows * mat->n_cols * sizeof(double) );
    
    memset( mat->data, 0, n_rows * n_cols * sizeof(double) );
    
    for( size_t col = 0; col < n_cols; col++ )
    {
      for( size_t row = 0; row < n_rows; row++ )
        mat->data[ col * n_rows + row ] = aux[ col * mat->n_rows + row ];
    }
    
    mat->n_rows = n_rows;
    mat->n_cols = n_cols;
  }

  return mat;
}

kmatrix_t km_scale( kmatrix_t mat, double scalar, kmatrix_t rslt )
{
  if( mat == NULL ) return NULL;
  
  size_t n_vals = rslt->n_rows * rslt->n_cols;
  for( size_t val_idx = 0; val_idx < n_vals; val_idx++ )
    rslt->data[ val_idx ] = scalar * mat->data[ val_idx ];

  rslt->n_rows = mat->n_rows;
  rslt->n_cols = mat->n_cols;
  
  return rslt;
}

kmatrix_t km_sum( kmatrix_t mat_1, double wgt_1, kmatrix_t mat_2, double wgt_2, kmatrix_t rslt )
{
  if( mat_1 == NULL || mat_2 == NULL ) return NULL;

  if( mat_1->n_rows != mat_2->n_rows || mat_1->n_cols != mat_2->n_cols ) return NULL;

  rslt->n_rows = mat_1->n_rows;
  rslt->n_cols = mat_1->n_cols;
  
  size_t n_vals = rslt->n_rows * rslt->n_cols;
  for( size_t val_idx = 0; val_idx < n_vals; val_idx++ )
    rslt->data[ val_idx ] = wgt_1 * mat_1->data[ val_idx ] + wgt_2 * mat_2->data[ val_idx ];

  return rslt;
}

kmatrix_t km_dot( kmatrix_t mat_1, char trs_1, kmatrix_t mat_2, char trs_2, kmatrix_t rslt )
{
  const double alpha = 1.0;
  const double beta = 0.0;
  
  double aux[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];
  
  if( mat_1 == NULL || mat_2 == NULL ) return NULL;
  
  size_t n_mults = ( trs_1 == KMATRIX_TRANSPOSE ) ? mat_1->n_rows : mat_1->n_cols;
   
  if( n_mults != ( ( trs_2 == KMATRIX_TRANSPOSE ) ? mat_2->n_cols : mat_2->n_rows ) ) return NULL;
   
  rslt->n_rows = ( trs_1 == KMATRIX_TRANSPOSE ) ? mat_1->n_cols : mat_1->n_rows;
  rslt->n_cols = ( trs_2 == KMATRIX_TRANSPOSE ) ? mat_2->n_rows : mat_2->n_cols;
  
  int step_1 = ( trs_1 == KMATRIX_TRANSPOSE ) ? n_mults : rslt->n_rows;       // Distance between columns
  int step_2 = ( trs_2 == KMATRIX_TRANSPOSE ) ? rslt->n_cols : n_mults;       // Distance between columns
  
  dgemm_( &trs_1, &trs_2, (int*) &(rslt->n_rows),(int*) &(rslt->n_cols), (int*) &(n_mults), 
          (double*) &alpha, mat_1->data, &step_1, mat_2->data, &step_2, (double*) &beta, aux, (int*) &rslt->n_rows );
  
  memcpy( rslt->data, aux, rslt->n_rows * rslt->n_cols * sizeof(double) );

  return rslt;
}

double km_det( kmatrix_t mat )
{
  double aux[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];
  int pivot[ KMATRIX_SIZE_MAX ];
  int info;
  
  if( mat == NULL ) return 0.0;

  if( mat->n_rows != mat->n_cols ) return 0.0;
  
  memcpy( aux, mat->data, mat->n_rows * mat->n_cols * sizeof(double) );
  
  dgetrf_( (int*) &(mat->n_rows), (int*) &(mat->n_cols), aux, (int*) &(mat->n_rows), pivot, &info );
  
  double det = 1.0;
  for( size_t pivot_idx = 0; pivot_idx < mat->n_rows; pivot_idx++ )
  {
    det *= aux[ pivot_idx * mat->n_rows + pivot_idx ];
    if( pivot[ pivot_idx ] != pivot_idx ) det *= -1.0;
  }

  return det;
}

kmatrix_t km_trans( kmatrix_t mat, kmatrix_t rslt )
{
  double aux[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ] = { 0 };
  
  if( mat == NULL ) return NULL;

  rslt->n_rows = mat->n_cols;
  rslt->n_cols = mat->n_rows;

  for( size_t row = 0; row < rslt->n_rows; row++ )
  {
    for( size_t col = 0; col < rslt->n_cols; col++ )
      aux[ col * mat->n_cols + row ] = mat->data[ row * mat->n_cols + col ];
  }

  memcpy( rslt->data, aux, rslt->n_rows * rslt->n_cols * sizeof(double) );

  return rslt;
}

kmatrix_t km_inv( kmatrix_t mat, kmatrix_t rslt )
{
  double aux[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ] = { 0 };
  int pivot[ KMATRIX_SIZE_MAX ];
  int info;
  
  if( mat == NULL || rslt == NULL ) return NULL;

  if( mat->n_rows != mat->n_cols ) return NULL;

  if( mat != rslt )
  {
    rslt->n_rows = mat->n_rows;
    rslt->n_cols = mat->n_cols;
  
    memcpy( rslt->data, mat->data, mat->n_rows * mat->n_cols * sizeof(double) );
  }
  
  dgetrf_( (int*) &(rslt->n_rows), (int*) &(rslt->n_cols), rslt->data, (int*) &(rslt->n_rows), pivot, &info );
  
  if( info != 0 ) return NULL;
  
  for( size_t pivot_idx = 0; pivot_idx < rslt->n_rows; pivot_idx++ )
  {
    if( aux[ pivot_idx * rslt->n_rows + pivot_idx ] == 0.0 ) return NULL;
  }
  
  int n_works = rslt->n_rows * rslt->n_cols;
  dgetri_( (int*) &(rslt->n_rows), rslt->data, (int*) &(rslt->n_rows), pivot, aux, &n_works, &info );
  
  if( info != 0 ) return NULL;

  return rslt;
}

void km_print( kmatrix_t mat )
{
  if( mat == NULL ) return;
  
  printf( "[%lux%lu] matrix:\n", mat->n_rows, mat->n_cols );
  for( size_t row = 0; row < mat->n_rows; row++ )
  {
    printf( "[" );
    for( size_t col = 0; col < mat->n_cols; col++ )
      printf( " %.6f", mat->data[ row * mat->n_cols + col ] );
    printf( " ]\n" );
  }
  printf( "\n" );
}
