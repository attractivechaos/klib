#include "kmatrix.h"

#include <stdio.h>

#define PRINT_RESULT() do { printf( "[%s]: %s\n\n", __func__, km_equal( result, comp ) ? "passed !" : "failed" ); \
                            printf( "result =\n" ); km_print( result ); \
                            printf( "expected =\n" ); km_print( comp ); } while( 0 )
                     
void km_test_reshape()
{
  double data[ 8 ] = { 5, 6, 8, 7, 2, 1, 0, 0 };
  
  kmatrix_t input = km_create( 2, 3, data );
  kmatrix_t comp = km_create( 4, 2, data );
  
  kmatrix_t result = km_reshape( input, 4, 2, NULL );
  
  PRINT_RESULT();
  
  km_free( input );
  km_free( comp );
  km_free( result );
}

void km_test_sum()
{
  double input_data_1[ 6 ] = { 5, 6, 8, 7, 2, 1 };
  double input_data_2[ 6 ] = { 2, 5, 1, 8, 3, 4 };
  
  kmatrix_t input_1 = km_create( 3, 2, input_data_1 );
  kmatrix_t input_2 = km_create( 3, 2, input_data_2 );
  
  double comp_data[ 6 ] = { 9, 16, 10, 23, 8, 9 };
  kmatrix_t comp = km_create( 3, 2, comp_data );
  
  kmatrix_t result = km_sum( input_1, 1.0, input_2, 2.0, NULL );
  
  PRINT_RESULT();
  
  km_free( input_1 );
  km_free( input_2 );
  km_free( comp );
  km_free( result );
}

void km_test_dot()
{
  double input_data_1[ 6 ] = { 5, 6, 8, 7, 2, 1 };
  double input_data_2[ 6 ] = { 2, 5, 1, 8, 3, 4 };
  
  kmatrix_t input_1 = km_create( 3, 2, input_data_1 );
  kmatrix_t input_2 = km_create( 3, 2, input_data_2 );
  
  double comp_data[ 4 ] = { 24, 97, 22, 90 };
  kmatrix_t comp = km_create( 2, 2, comp_data );
  
  kmatrix_t result = km_dot( input_1, KMATRIX_TRANSPOSE, input_2, KMATRIX_KEEP, NULL );
  
  PRINT_RESULT();
  
  km_free( input_1 );
  km_free( input_2 );
  km_free( comp );
  km_free( result );
}

void km_test_trans()
{
  double input_data[ 6 ] = { 2, 5, 1, 8, 3, 4 };
  kmatrix_t input = km_create( 3, 2, input_data );
  
  double comp_data[ 6 ] = { 2, 1, 3, 5, 8, 4 };
  kmatrix_t comp = km_create( 2, 3, comp_data );
  
  kmatrix_t result = km_trans( input, NULL );
  
  PRINT_RESULT();
  
  km_free( input );
  km_free( comp );
  km_free( result );
}

void km_test_inv()
{
  double input_data[ 9 ] = { 3, 7, 8, 9, 0, 2, 3, 7, 2 };
  kmatrix_t input = km_create( 3, 3, input_data );
  
  double comp_data[ 9 ] = { -0.0370, 0.1111, 0.0370, -0.0317, -0.0476, 0.1746, 0.1667, 0, -0.1667 };
  kmatrix_t comp = km_create( 3, 3, comp_data );
  
  kmatrix_t result = km_inv( input, NULL );
  
  PRINT_RESULT();
  
  km_free( input );
  km_free( comp );
  km_free( result );
}


int main( int arg, char* argv[] )
{
//   double matrix_data_1[ 9 ] = { 3, 7, 8, 9, 0, 2, 3, 7, 2 };
//   double matrix_data_2[ 9 ] = { 0, 7, 8, 3, 5, 1, 7, 2, 1 };
//   
//   kmatrix_t matrix_1 = km_create( 3, 3, matrix_data_1 );
//   kmatrix_t matrix_2 = km_create( 3, 3, NULL );
//   km_fill( matrix_2, matrix_data_2 );  
//   
//   kmatrix_t result = km_dot( matrix_1, KMATRIX_KEEP, matrix_2, KMATRIX_KEEP, NULL );
//   km_print( result );
//   
//   result = km_dot( matrix_1, KMATRIX_TRANSPOSE, matrix_2, KMATRIX_TRANSPOSE, result );
//   km_print( result );
//   
//   printf( "determinants: %g and %g\n", km_det( matrix_1 ), km_det( matrix_2 ) );
//   
//   km_free( matrix_1 );
//   km_free( matrix_2 );
//   km_free( result );
  
  km_test_reshape();
  km_test_sum();
  km_test_dot();
  km_test_trans();
  km_test_inv();
  
  return 0;
}
