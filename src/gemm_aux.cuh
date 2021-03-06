#include <cuda.h>
#include <cuda_runtime.h>

#ifndef _GEMM_AUX_H_
#define _GEMM_AUX_H_

#define MAX_NGPUS	(16)
#define MAX_EVENTS	(100)

#define max(a, b)	( (a) > (b)? (a) : (b) )
#define min(a, b)	( (a) < (b)? (a) : (b) )

// recommend a tile
long recommend_tile(long m, long n, long k, long ngpus, long max_good_tile)
{
	// the goal is to minimize the load imbalance among the gpus
	long ngpus_ = ngpus;
	long tile = (m + ngpus_-1)/ngpus_; 
	while(tile > max_good_tile) 
	{
		ngpus_ += ngpus_;
		tile = (m + ngpus_-1)/ngpus_;
	}
	return tile;
}

void process_error(cublasStatus_t e, const char* txt)
{
	if(e != CUBLAS_STATUS_SUCCESS)printf("%s\n", txt);
}

#endif	// _GEMM_AUX_H_