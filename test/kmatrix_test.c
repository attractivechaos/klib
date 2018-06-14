#include "kmatrix.h"

#include <stdio.h>
#include <math.h>

#define PRINT_RESULT() do { printf( "[%s]: %s\n\n", __func__, km_equal( result, comp ) ? "passed !" : "failed" ); \
                            printf( "result =\n" ); if( result != NULL ) km_print( result ); \
                            printf( "expected =\n" ); km_print( comp ); } while( 0 )
                     
void km_test_resize()
{
  double input_data[ 6 ] = { 5, 6, 8, 7, 2, 1 };
  kmatrix_t input = km_create( input_data, 2, 3 );
  
  double comp_data[ 12 ] = { 5, 6, 8, 0, 7, 2, 1, 0, 0, 0, 0, 0 };
  kmatrix_t comp = km_create( comp_data, 3, 4 );
  
  kmatrix_t result = km_resize( input, 3, 4 );
  
  PRINT_RESULT();
  
  km_free( comp );
  km_free( result );
}

void km_test_sum()
{
  double input_data_1[ 6 ] = { 5, 6, 8, 7, 2, 1 };
  kmatrix_t input_1 = km_create( input_data_1, 3, 2 );
  
  double input_data_2[ 6 ] = { 2, 5, 1, 8, 3, 4 };
  kmatrix_t input_2 = km_create( input_data_2, 3, 2 );
  
  double comp_data[ 6 ] = { 9, 16, 10, 23, 8, 9 };
  kmatrix_t comp = km_create( comp_data, 3, 2 );
  
  kmatrix_t result = km_create( NULL, 3, 2 );
  
  result = km_sum( input_1, 1.0, input_2, 2.0, result );
  
  PRINT_RESULT();
  
  km_free( input_1 );
  km_free( input_2 );
  km_free( comp );
  km_free( result );
}

void km_test_dot()
{
  double input_data_1[ 6 ] = { 5, 6, 8, 7, 2, 1 };
  kmatrix_t input_1 = km_create( input_data_1, 3, 2 );
  
  double input_data_2[ 6 ] = { 2, 5, 1, 8, 3, 4 };
  kmatrix_t input_2 = km_create( input_data_2, 3, 2 );
  
  double comp_data[ 4 ] = { 24, 97, 22, 90 };
  kmatrix_t comp = km_create( comp_data, 2, 2 );
  
  kmatrix_t result = km_create( NULL, 2, 2 );
  
  result = km_dot( input_1, KMATRIX_TRANSPOSE, input_2, KMATRIX_KEEP, result );
  
  PRINT_RESULT();
  
  km_free( input_1 );
  km_free( input_2 );
  km_free( comp );
  km_free( result );
}

void km_test_trans()
{
  double input_data[ 6 ] = { 2, 5, 1, 8, 3, 4 };
  kmatrix_t input = km_create( input_data, 3, 2 );
  
  double comp_data[ 6 ] = { 2, 1, 3, 5, 8, 4 };
  kmatrix_t comp = km_create( comp_data, 2, 3 );
  
  kmatrix_t result = km_create( NULL, 2, 3 );
                                
  result = km_trans( input, result );
  
  PRINT_RESULT();
  
  km_free( input );
  km_free( comp );
  km_free( result );
}

void km_test_inv()
{
  double input_data[ 9 ] = { 3, 7, 8, 9, 0, 2, 3, 7, 2 };
  kmatrix_t input = km_create( input_data, 3, 3 );
  
  double comp_data[ 9 ] = { -0.0370, 0.1111, 0.0370, -0.0317, -0.0476, 0.1746, 0.1667, 0, -0.1667 };
  kmatrix_t comp = km_create( comp_data, 3, 3 );
  
  kmatrix_t result = km_create( NULL, 3, 3 );
  
  result = km_inv( input, result );
  
  PRINT_RESULT();

  km_free( input );
  km_free( comp );
  km_free( result );
}

void km_test_det()
{
  double input_data[ 9 ] = { 3, 7, 8, 9, 0, 2, 3, 7, 2 };
  kmatrix_t input = km_create( input_data, 3, 3 );
  
  double comp = 378.00000000000011;
  
  double det = km_det( input );
  
  printf( "[km_test_det]: %s\n\n", ( fabs( det - comp ) < 0.0001  ) ? "passed !" : "failed" );
  printf( "result = %g\n", det );
  printf( "expected = %g\n", comp );

  km_free( input );
}

int main( int arg, char* argv[] )
{  
  km_test_resize();
  km_test_sum();
  km_test_dot();
  km_test_trans();
  km_test_inv();
  km_test_det();
  
  return 0;
}
