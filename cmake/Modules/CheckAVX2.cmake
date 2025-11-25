# Checks whether AVX is supported.
# If it is supported then HAVE_AVX2_EXTENSION
# will be true and the necessary compile flags
# are stored in AVX2_FLAGS
macro(check_for_avx2)
  include(CheckCSourceRuns)
  include (CMakePushCheckState)
  include (CheckCSourceCompiles)
  cmake_push_check_state ()
  set(CMAKE_REQUIRED_FLAGS -mavx2 ${CMAKE_REQUIRED_FLAGS})
  check_c_source_compiles("
#include <immintrin.h>
int main(){
  __m256d mm_zeros =_mm256_setzero_pd();
}"
    AVX2_FLAGS_WORK)
    
  if(AVX2_FLAGS_WORK)
    set(AVX2_FLAGS -mavx2)
    check_c_source_runs("
#include <stdio.h>
#include <immintrin.h>

// mixed-precision matrix-vector mutiplication for 3x3 matrices using avx2
void vmmv3(float const *A, double const *x, double *y)
{

    //allocate and initialize avx vectors
    __m256d vA[3];
    for(int k=0;k<3;k++) vA[k] = _mm256_setzero_pd();
    __m256d vx = _mm256_loadu_pd(x);

    // scale columns of matrix A with corresponding elements of vector x
    vA[0] += _mm256_cvtps_pd(_mm_loadu_ps(A+0))*_mm256_permute4x64_pd(vx,0b00000000); //0b01010101
    vA[1] += _mm256_cvtps_pd(_mm_loadu_ps(A+3))*_mm256_permute4x64_pd(vx,0b01010101); //0b01010101
    vA[2] += _mm256_cvtps_pd(_mm_loadu_ps(A+6))*_mm256_permute4x64_pd(vx,0b10101010); //0b01010101

    // sum over columns
    __m256d vy, vz;
    vz = vA[0] + vA[1] + vA[2];

    // blend to keep 4th element unchanged
    vy = _mm256_loadu_pd(y);
    vz =_mm256_blend_pd(vy,vz,0x7);

    // store result
    _mm256_storeu_pd(y,vz);
}

int main()
{
    // padded allocation and initialization
    float A[10] = {1,-1,0,-1,2,-1,0,-1,1,1};
    double x[4] = {1,1,1,1};
    double y[4] = {-1,-1,-1,-1};

    // mixed-precision matrix-vector multiplication
    vmmv3(A,x,y);

    // display matrix
    printf("A=\n[\n");
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<3;j++) printf(" %+.2f",A[3*i+j]);
        printf("\n");
    }
    printf("]\n\n");

    // display input vector
    printf("x=\n[\n");
    for(int i=0;i<3;i++) printf(" %+.2f\n",x[i]);
    printf("]\n\n");

    // display outout vector
    printf("y=\n[\n");
    for(int i=0;i<4;i++) printf(" %+.2f\n",y[i]);
    printf("]\n\n");
}
"
      HAVE_AVX2_EXTENSION)
  endif()
  cmake_pop_check_state()
endmacro()
