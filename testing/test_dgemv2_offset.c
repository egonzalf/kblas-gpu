#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <cublas_v2.h>
#include "kblas.h"
#include "testing_utils.h"

#define FMULS_GEMV(n) ((n) * (n) + 2. * (n))
#define FADDS_GEMV(n) ((n) * (n)           )

#define PRECISION_d

#if defined(PRECISION_z) || defined(PRECISION_c)
#define FLOPS(n) ( 6. * FMULS_GEMV(n) + 2. * FADDS_GEMV(n))
#else
#define FLOPS(n) (      FMULS_GEMV(n) +      FADDS_GEMV(n))
#endif

int main(int argc, char** argv)
{
	if(argc < 7)
	{
		printf("USAGE: %s <device-id> <no-trans'n' or trans't' or conj-trans'c'> <matrix-dim> <start-offset> <stop-offset> <step-offset>\n", argv[0]); 
		printf("==> <device-id>: GPU device id to use \n");
		printf("==> <no-trans'n' or trans't' or conj-trans'c'>: Process the matrix in non-transposed,transposed, or conjugate transposed configuration \n");
		printf("==> <matrix-dim>: The dimension of the matrix\n");
		printf("==> <start-offset> <stop-offset> <step-offset>: Offset range. For every <offset> in the offset range, test is performed on a submatrix whose dimension is <matrix-dim>-<offset>\n");
		exit(-1);
	}
	
	int dev = atoi(argv[1]);
	char trans = *argv[2];
	int dim = atoi(argv[3]);
	int istart = atoi(argv[4]);
	int istop = atoi(argv[5]);
	int istep = atoi(argv[6]);
	
	const int nruns = NRUNS;
	
	cudaError_t ed = cudaSetDevice(dev);
	if(ed != cudaSuccess){printf("Error setting device : %s \n", cudaGetErrorString(ed) ); exit(-1);}
	
	cublasHandle_t cublas_handle;
	cublasAtomicsMode_t mode = CUBLAS_ATOMICS_ALLOWED;
	cublasCreate(&cublas_handle);
	cublasSetAtomicsMode(cublas_handle, mode);
	
	struct cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, dev);
	
	if(istop >= dim){printf("Error: maximum offset value causes zero or negative submatrix dimension\n"); exit(-1);}
	
    int M = dim;
    int N = M;
    int LDA = M;
    int LDA_ = ((M+31)/32)*32;

	int incx = 1;
	int incy = 1;
	int vecsize_x = N * abs(incx);
	int vecsize_y = M * abs(incy);
	
	cublasOperation_t trans_;
	if(trans == 'N' || trans == 'n')
		trans_ = CUBLAS_OP_N;
	else if (trans == 'T' || trans == 't')
		trans_ = CUBLAS_OP_T;
	else if (trans == 'C' || trans == 'c')
		trans_ = CUBLAS_OP_C;
		
	double alpha = 2.3, beta = -0.6;
	
	cudaError_t err;
	cudaEvent_t start, stop; 
	
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	
    // point to host memory
    double* A = NULL;
    double* x = NULL;
    double* ycuda = NULL;
    double* ykblas = NULL;
	
    // point to device memory
    double* dA = NULL;
    double* dx = NULL;
    double* dy = NULL;

    if(trans == 'N' || trans == 'n')printf("non-transposed test .. \n");
	else if (trans == 'T' || trans == 't') printf("transposed test .. \n");
	else if (trans == 'C' || trans == 'c') printf("Conjugate transposed test .. \n");
	else { printf("transpose configuration is not properly specified\n"); exit(-1);}
	printf("Allocating Matrices\n");
    A = (double*)malloc(N*LDA*sizeof(double));
    x = (double*)malloc(vecsize_x*sizeof(double));
    ycuda = (double*)malloc(vecsize_y*sizeof(double));
    ykblas = (double*)malloc(vecsize_y*sizeof(double));
    
    err = cudaMalloc((void**)&dA, N*LDA_*sizeof(double));
    if(err != cudaSuccess){printf("ERROR: %s \n", cudaGetErrorString(err)); exit(1);}
    err = cudaMalloc((void**)&dx, vecsize_x*sizeof(double));
    if(err != cudaSuccess){printf("ERROR: %s \n", cudaGetErrorString(err)); exit(1);}
    err = cudaMalloc((void**)&dy, vecsize_y*sizeof(double));
	if(err != cudaSuccess){printf("ERROR: %s \n", cudaGetErrorString(err)); exit(1);}
	
    // Initialize matrix and vector
    printf("Initializing on cpu .. \n");
    int i, j, m;
    for(i = 0; i < M; i++)
    		for(j = 0; j < N; j++)
    			A[j*LDA+i] = kblas_drand();
      
    for(i = 0; i < vecsize_x; i++)
      x[i] = kblas_drand();
    
    cublasSetMatrix(dim, dim, sizeof(double), A, LDA, dA, LDA_);
	cudaMemcpy(dx, x, vecsize_x*sizeof(double), cudaMemcpyHostToDevice);

	printf("------------------- Testing DGEMV ----------------\n");
    printf("  Matrix        CUBLAS       KBLAS          Max.  \n");
    printf(" Dimension     (Gflop/s)   (Gflop/s)       Error  \n");
    printf("-----------   ----------   ----------   ----------\n");
    
    int r;
    for(m = istart; m <= istop; m += istep)
    {
    	float elapsedTime; 
    	int offset = m;
    	int dim_ = dim-offset;
      	float flops = FLOPS( (float)dim_ ) / 1e6;
		
		for(i = 0; i < vecsize_y; i++)
    	{
    		// init y for now until beta is supported in gemv2
      		ycuda[i] = kblas_drand();
      		ykblas[i] = ycuda[i];
    	}
        
        // handle the offset
        double* dA_ = dA + offset * LDA_ + offset;
        double* dx_ = dx + offset * incx;
        double* dy_ = dy + offset * incy;
        int vecsize_y_ = vecsize_y - offset;
        int vecsize_x_ = vecsize_x - offset;
        
      	// --- cuda test
      	elapsedTime = 0;
      	for(r = 0; r < nruns; r++)
      	{
      		cudaMemcpy(dy_, ycuda, vecsize_y_ * sizeof(double), cudaMemcpyHostToDevice);
      		
      		cudaEventRecord(start, 0);
      		cublasDgemv(cublas_handle, trans_, dim_, dim_, &alpha, dA_, LDA_, dx_, incx, &beta, dy_, incy);
      		cudaEventRecord(stop, 0);
      		cudaEventSynchronize(stop);
      		float time  = 0;
      		cudaEventElapsedTime(&time, start, stop);
      		elapsedTime += time;
      	}
      	elapsedTime /= nruns;
      	float cuda_perf = flops / elapsedTime;
      
      	cudaMemcpy(ycuda, dy_, vecsize_y_ * sizeof(double), cudaMemcpyDeviceToHost);
      	// end of cuda test
      	  	
      	// ---- kblas
      	elapsedTime = 0;
      	for(r = 0; r < nruns; r++)
      	{
      		cudaMemcpy(dy_, ykblas, vecsize_y_ * sizeof(double), cudaMemcpyHostToDevice);
      		
      		cudaEventRecord(start, 0);
      		kblas_dgemv2_offset( trans, dim, dim, alpha, dA, LDA_, dx, incx, beta, dy, incy, offset, offset);
      		cudaEventRecord(stop, 0);
      		cudaEventSynchronize(stop);
      		
      		float time  = 0;	
      		cudaEventElapsedTime(&time, start, stop);
      		elapsedTime += time;
      	}
      	elapsedTime /= nruns;
      	float kblas_perf = flops / elapsedTime;

      	cudaMemcpy(ykblas, dy_, vecsize_y_ * sizeof(double), cudaMemcpyDeviceToHost);
      
      	// testing error -- specify ref. vector and result vector
      	double* yref = ycuda; 
      	double* yres = ykblas;
      	
      	double error = dget_max_error(yref, yres, dim_, incy);
      
      	//for(i = 0; i < m; i++) printf("[%d]:   %-8.2f   %-8.2f\n", i, ycuda[i], ykblas[i]);
      		
      	//printf("-----------   ----------   ----------   ----------   ----------   ----------\n");
    	printf("%-11d   %-10.2f   %-10.2f   %-10e;\n", dim_, cuda_perf, kblas_perf, error);
    	
    }
	
	cudaEventDestroy(start);
	cudaEventDestroy(stop);
	
    if(dA)cudaFree(dA);
    if(dx)cudaFree(dx);
    if(dy)cudaFree(dy);
    
    if(A)free(A);
    if(x)free(x);
    if(ycuda)free(ycuda);
	if(ykblas)free(ykblas);

	cublasDestroy(cublas_handle);
    return EXIT_SUCCESS;
}

