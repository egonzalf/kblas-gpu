#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <cublas.h>
#include "kblas.h"
#include "testing_utils.h"

#define FMULS_SCAL(n) (n)
#define FADDS_SCAL(n) (0)

#define PRECISION_c

#if defined(PRECISION_z) || defined(PRECISION_c)
#define FLOPS(n) ( 6. * FMULS_SCAL(n) + 2. * FADDS_SCAL(n))
#else
#define FLOPS(n) (      FMULS_SCAL(n) +      FADDS_SCAL(n))
#endif

float get_magnitude(cuFloatComplex a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}

float get_max_error(int n, cuFloatComplex* ref, int inc_ref, cuFloatComplex *res, int inc_res)
{
	int i, j;
	float max_err = -1.0;
	float err = -1.0;
	inc_ref = abs(inc_ref);
	inc_res = abs(inc_res);
	for(i = 0; i < n; i++)
	{
		cuFloatComplex rf = ref[i * inc_ref];
		cuFloatComplex rs = res[i * inc_res];
		err = get_magnitude( make_cuFloatComplex(rf.x-rs.x, rf.y-rs.y) );
		if(get_magnitude(rf) > 0)err /= get_magnitude(rf);
		if(err > max_err)max_err = err;
	}
	return max_err;
}

int main(int argc, char** argv)
{
	if(argc < 5)
	{
		printf("USAGE: %s <device-id> <start-dim> <stop-dim> <step-dim>\n", argv[0]); 
		printf("==> <device-id>: GPU device id to use \n");
		printf("==> <start-dim> <stop-dim> <step-dim>: test for dimensions in the range start-dim : stop-dim with step step-dim \n");
		exit(-1);
	}
	
	int dev = atoi(argv[1]);
	int istart = atoi(argv[2]);
	int istop = atoi(argv[3]);
	int istep = atoi(argv[4]);
	
	const int nruns = NRUNS;
	
	cudaError_t ed = cudaSetDevice(dev);
	if(ed != cudaSuccess){printf("Error setting device : %s \n", cudaGetErrorString(ed) ); exit(-1);}
	
	struct cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, dev);
	
    int N = istop;
    int incx = 1;
	
	int vecsize = N * abs(incx);
	
	cudaError_t err;
	cudaEvent_t start, stop; 
	
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	
    // point to host memory
    cuFloatComplex* x = NULL;
    cuFloatComplex* xcublas = NULL;
    cuFloatComplex* xkblas = NULL;
    
    // point to device memory
    cuFloatComplex* dx = NULL;
    
	printf("Allocating vectors\n");
    x 		= (cuFloatComplex*)malloc(vecsize*sizeof(cuFloatComplex));
    xcublas = (cuFloatComplex*)malloc(vecsize*sizeof(cuFloatComplex));
    xkblas 	= (cuFloatComplex*)malloc(vecsize*sizeof(cuFloatComplex));
    
    cudaMalloc((void**)&dx, vecsize*sizeof(cuFloatComplex));
    
    // Initialize vectors
    printf("Initializing ... \n");
    int i, j, m, r;
    for(i = 0; i < N; i++)
      x[i] = kblas_crand();;

    cuFloatComplex alpha; 
    alpha  = kblas_crand();
    
    printf("--------------- Testing CSCAL ----------------\n");
    printf("  Matrix       CUBLAS      KBLAS        Max.  \n");
    printf(" Dimension    (Gflop/s)   (Gflop/s)    Error  \n");
    printf("-----------   ---------   ---------   --------\n");
    
    for(m = istart; m <= istop; m += istep)
    {
    	float elapsedTime; 
    	
      	float flops = FLOPS( (float)m ) / 1e6;
		
		// cublas test
		elapsedTime = 0.0;
      	for(r = 0; r < nruns; r++)
      	{
      		cudaMemcpy(dx, x, m * sizeof(cuFloatComplex), cudaMemcpyHostToDevice);
			cudaEventRecord(start, 0);
      		
      		cublasCscal(m, alpha, dx, incx);
      		
      		cudaEventRecord(stop, 0);
      		cudaEventSynchronize(stop);
      		
      		float time = 0;
      		cudaEventElapsedTime(&time, start, stop);
      		elapsedTime += time;
      	}
      	elapsedTime /= nruns;
      	float cublas_perf = flops / elapsedTime;
      
      	cudaMemcpy(xcublas, dx, vecsize * sizeof(cuFloatComplex), cudaMemcpyDeviceToHost);
      	// end of cuda test
      	  	
      	// ---- kblas
      	elapsedTime = 0.0;
      	for(r = 0; r < nruns; r++)
      	{
      		cudaMemcpy(dx, x, m * sizeof(cuFloatComplex), cudaMemcpyHostToDevice);
      		cudaEventRecord(start, 0);
      		kblas_cscal(m, alpha, dx, incx);
      		cudaEventRecord(stop, 0);
      		cudaEventSynchronize(stop);
      		float time = 0;
      		cudaEventElapsedTime(&time, start, stop);
      		elapsedTime += time;
      	}
      	elapsedTime /= nruns;
      	float kblas_perf = flops / elapsedTime;

      	cudaMemcpy(xkblas, dx, vecsize * sizeof(cuFloatComplex), cudaMemcpyDeviceToHost);
      	
      	// testing error -- specify ref. vector and result vector
      	cuFloatComplex* yref = xcublas; 
      	cuFloatComplex* yres = xkblas;
      	
      	float error = get_max_error(m, yref, incx, yres, incx);
      
      	printf("%-11d   %-9.2f   %-9.2f   %-10e;\n", m, cublas_perf, kblas_perf, error);
    }
	
	cudaEventDestroy(start);
	cudaEventDestroy(stop);
	
    if(dx)cudaFree(dx);
    
    if(x)free(x);
    if(xcublas)free(xcublas);
	if(xkblas)free(xkblas);
	
	return EXIT_SUCCESS;
}

