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
#include <immintrin.h>

typedef
struct bsr_matrix
{
    int nrows;
    int ncols;
    int nnz;
    int b;

    int *rowptr;
    int *colidx;
    double *dbl;
    float *flt;

} bsr_matrix;

void bsr_vmspmv3(bsr_matrix *A, const double *x, double *y)
{
    int nrows = A->nrows;
    int *rowptr=A->rowptr;
    int *colidx=A->colidx;
    const float *data=A->flt;

    const int b=3;

    __m256d mm_zeros =_mm256_setzero_pd();
    for(int i=0;i<nrows;i++)
    {
        __m256d vA[3];
        for(int k=0;k<3;k++) vA[k] = mm_zeros;
        for(int k=rowptr[i];k<rowptr[i+1];k++)
        {
            const float *AA=data+9*k;

            int j = colidx[k];
            __m256d vx = _mm256_loadu_pd(x+b*j);

            vA[0] += _mm256_cvtps_pd(_mm_loadu_ps(AA+0))*_mm256_permute4x64_pd(vx,0b00000000); //0b01010101
            vA[1] += _mm256_cvtps_pd(_mm_loadu_ps(AA+3))*_mm256_permute4x64_pd(vx,0b01010101); //0b01010101
            vA[2] += _mm256_cvtps_pd(_mm_loadu_ps(AA+6))*_mm256_permute4x64_pd(vx,0b10101010); //0b01010101
        }

        // sum over columns
        __m256d vy, vz;
        vz = vA[0] + vA[1] + vA[2];

        double *y_i = y+b*i;
        vy = _mm256_loadu_pd(y_i);       // optional blend to keep
        vz =_mm256_blend_pd(vy,vz,0x7);  // 4th element unchanged
        _mm256_storeu_pd(y_i,vz);
    }
}
int main(){}
"
      HAVE_AVX2_EXTENSION)
  endif()
  cmake_pop_check_state()
endmacro()
