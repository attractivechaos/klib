#include "kmatrix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

struct _kmatrix_data_t
{
  double* data;
  size_t n_rows, n_cols;
};                                  


kmatrix_t km_clear( kmatrix_t matrix )
{
  if( matrix == NULL ) return NULL;

  memset( matrix->data, 0, matrix->n_rows * matrix->n_cols * sizeof(double) );

  return matrix;
}

kmatrix_t km_create( double* data, size_t n_rows, size_t n_cols )
{
  if( n_rows > KMATRIX_SIZE_MAX || n_cols > KMATRIX_SIZE_MAX ) return NULL;

  kmatrix_t new_matrix_t = (kmatrix_t) malloc( sizeof(kmatrix_data_t) );

  new_matrix_t->data = (double*) calloc( n_rows * n_cols, sizeof(double) );

  new_matrix_t->n_rows = n_rows;
  new_matrix_t->n_cols = n_cols;

  if( data == NULL ) km_clear( new_matrix_t );
  else memcpy( new_matrix_t->data, data, n_rows * n_cols * sizeof(double) );

  return new_matrix_t;
}

kmatrix_t km_create_square( size_t size, uint8_t isIdentity )
{
  kmatrix_t new_sq_matrix = km_create( NULL, size, size );

  if( isIdentity )
  {
    for( size_t line = 0; line < size; line++ )
      new_sq_matrix->data[ line * size + line ] = 1.0;
  }

  return new_sq_matrix;
}

void km_free( kmatrix_t matrix )
{
  if( matrix == NULL ) return;
  
  free( matrix->data );
  
  free( matrix );
}

kmatrix_t km_resize( kmatrix_t matrix, size_t n_rows, size_t n_cols )
{
  if( matrix == NULL )
    matrix = km_create( NULL, n_rows, n_cols );
  else if( matrix->n_rows * matrix->n_cols < n_rows * n_cols )
    matrix->data = (double*) realloc( matrix->data, n_rows * n_cols * sizeof(double) );
  
  matrix->n_rows = n_rows;
  matrix->n_cols = n_cols;

  return matrix;
}

double km_get( kmatrix_t matrix, size_t row, size_t col )
{
  if( matrix == NULL ) return 0.0;

  if( row >= matrix->n_rows || col >= matrix->n_cols ) return 0.0;

  return matrix->data[ row * matrix->n_cols + col ];
}

double* km_to_array( kmatrix_t matrix )
{
  if( matrix == NULL ) return NULL;

  return matrix->data;
}

void km_set( kmatrix_t matrix, size_t row, size_t col, double value )
{
  if( matrix == NULL ) return;

  if( row >= matrix->n_rows || col >= matrix->n_cols ) return;

  matrix->data[ row * matrix->n_cols + col ] = value;
}

kmatrix_t km_scale( kmatrix_t matrix, double scalar, kmatrix_t result )
{
  if( matrix == NULL ) return NULL;
  
  if( (result = km_resize( result, matrix->n_rows, matrix->n_cols )) == NULL ) return NULL;

  for( size_t row = 0; row < matrix->n_rows; row++ )
  {
    for( size_t col = 0; col < matrix->n_cols; col++ )
      result->data[ row * matrix->n_cols + col ] = matrix->data[ row * matrix->n_cols + col ] * scalar;
  }

  return result;
}

kmatrix_t km_sum( kmatrix_t matrix_1, double weight_1, kmatrix_t matrix_2, double weight_2, kmatrix_t result )
{
  static double aux_array[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];

  if( matrix_1 == NULL || matrix_2 == NULL ) return NULL;

  if( matrix_1->n_rows != matrix_2->n_rows || matrix_1->n_cols != matrix_2->n_cols ) return NULL;

  if( (result = km_resize( result, matrix_1->n_rows, matrix_1->n_cols )) == NULL ) return NULL;

  for( size_t row = 0; row < result->n_rows; row++ )
  {
    for( size_t col = 0; col < result->n_cols; col++ )
    {
      double element_1 = matrix_1->data[ row * matrix_1->n_cols + col ];
      double element_2 = matrix_2->data[ row * matrix_2->n_cols + col ];
      aux_array[ row * result->n_cols + col ] = weight_1 * element_1 + weight_2 * element_2;
    }
  }

  memcpy( result->data, aux_array, result->n_rows * result->n_cols * sizeof(double) );

  return result;
}

kmatrix_t km_dot( kmatrix_t matrix_1, uint8_t transpose_1, kmatrix_t matrix_2, uint8_t transpose_2, kmatrix_t result )
{
  static double aux_array[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];

  if( matrix_1 == NULL || matrix_2 == NULL ) return NULL;

  size_t n_ops = transpose_1 ? matrix_1->n_rows : matrix_1->n_cols;
  
  if( n_ops != ( transpose_2 ? matrix_2->n_cols : matrix_2->n_rows ) ) return NULL;
  
  size_t result_n_rows = transpose_1 ? matrix_1->n_cols : matrix_1->n_rows;
  size_t result_n_cols = transpose_2 ? matrix_2->n_rows : matrix_2->n_cols;

  if( (result = km_resize( result, result_n_rows, result_n_cols )) == NULL ) return NULL;

  for( size_t row = 0; row < result->n_rows; row++ )
  {
    for( size_t col = 0; col < result->n_cols; col++ )
    {
      aux_array[ row * result->n_cols + col ] = 0.0;
      for( size_t i = 0; i < n_ops; i++ )
      {
        size_t elementIndex_1 = transpose_1 ? i * matrix_1->n_cols + row : row * matrix_1->n_cols + i;
        size_t elementIndex_2 = transpose_2 ? col * matrix_2->n_cols + i : i * matrix_2->n_cols + col;
        aux_array[ row * result->n_cols + col ] += matrix_1->data[ elementIndex_1 ] * matrix_2->data[ elementIndex_2 ];
      }
    }
  }

  memcpy( result->data, aux_array, result->n_rows * result->n_cols * sizeof(double) );

  return result;
}

void get_cof_array( double* m_array, size_t size, size_t elementRow, size_t elementColumn, double* result )
{
  size_t cof_row = 0, cof_col = 0;
  size_t cof_size = size - 1;
  for( size_t row = 0; row < size; row++ )
  {
    for( size_t col = 0; col < size; col++ )
    {
      if( row != elementRow && col != elementColumn )
      {
        result[ cof_row * cof_size + cof_col ] = m_array[ row * size + col ];

        if( cof_col < cof_size - 1 ) cof_col++;
        else
        {
          cof_col = 0;
          cof_row++;
        }
      }
    }
  }
}

double get_array_det( double* m_array, size_t size )
{
  double aux_array[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];

  if( size == 1 ) return m_array[ 0 ];

  double result = 0.0;
  for( size_t c = 0; c < size; c++ )
  {
    get_cof_array( m_array, size, 0, c, (double*) aux_array );
    result += pow( -1, c ) * m_array[ c ] * get_array_det( (double*) aux_array, size - 1 );
  }

  return result;
}

double get_array_cof( double* m_array, size_t size, size_t row, size_t col )
{
  double aux_array[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];

  get_cof_array( m_array, size, row, col, (double*) aux_array );

  double result = pow( -1, row + col ) * get_array_det( (double*) aux_array, size - 1 );

  return result;
}

double km_det( kmatrix_t matrix )
{
  if( matrix == NULL ) return 0.0;

  if( matrix->n_rows != matrix->n_cols ) return 0.0;

  return get_array_det( matrix->data, matrix->n_rows );
}

kmatrix_t km_trans( kmatrix_t matrix, kmatrix_t result )
{
  static double aux_array[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];

  if( matrix == NULL ) return NULL;

  if( (result = km_resize( result, matrix->n_cols, matrix->n_rows )) == NULL ) return NULL;

  for( size_t row = 0; row < result->n_rows; row++ )
  {
    for( size_t col = 0; col < result->n_cols; col++ )
      aux_array[ row * matrix->n_cols + col ] = matrix->data[ col * matrix->n_cols + row ];
  }

  memcpy( result->data, aux_array, result->n_rows * result->n_cols * sizeof(double) );

  return result;
}

kmatrix_t km_inv( kmatrix_t matrix, kmatrix_t result )
{
  static double aux_array[ KMATRIX_SIZE_MAX * KMATRIX_SIZE_MAX ];

  double determinant = km_det( matrix );

  if( determinant == 0.0 ) return NULL;

  if( (result = km_resize( result, matrix->n_cols, matrix->n_rows )) == NULL ) return NULL;

  for( size_t row = 0; row < result->n_rows; row++ )
  {
    for( size_t col = 0; col < result->n_cols; col++ )
      aux_array[ row * matrix->n_cols + col ] = get_array_cof( matrix->data, result->n_rows, col, row ) / determinant;
  }

  memcpy( result->data, aux_array, result->n_rows * result->n_cols * sizeof(double) );

  return result;
}

void km_print( kmatrix_t matrix )
{
  if( matrix == NULL ) return;

  for( size_t row = 0; row < matrix->n_rows; row++ )
  {
    printf( "[" );
    for( size_t col = 0; col < matrix->n_cols; col++ )
      printf( " %.6f", matrix->data[ row * matrix->n_cols + col ] );
    printf( " ]\n" );
  }
   printf( "\n" );
}
