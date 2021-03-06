/*
    -- KBLAS (version 1.0) --
       Ahmad Abdelfattah, Center of Extreme Computing
	   Hatem Ltaief, Supercomputing Laboratory
	   David Keyes, Center of Extreme Computing
	   King Abdullah University of Science and Technology (KAUST)
       June 2013
	   KBLAS is a subset of BLAS routines highly optimized for NVIDIA GPUs 
*/
/**
	-- Center of Extreme Computing and Supercomputing Laboratory
	-- Division of Applied Mathematics and Computational Science
	-- King Abdullah University of Science and Technology
	-- (C) Copyright 2013

	Redistribution  and  use  in  source and binary forms, with or without
	modification,  are  permitted  provided  that the following conditions
	are met:

	*	Redistributions  of  source  code  must  retain  the above copyright
		notice,  this  list  of  conditions  and  the  following  disclaimer.
	* 	Redistributions  in  binary  form must reproduce the above copyright
		notice,  this list of conditions and the following disclaimer in the
		documentation  and/or other materials provided with the distribution.
	* 	Neither  the  name of the University of Tennessee, Knoxville nor the
		names of its contributors may be used to endorse or promote products
		derived from this software without specific prior written permission.

	THIS  SOFTWARE  IS  PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	''AS IS''  AND  ANY  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A  PARTICULAR  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT NOT
	LIMITED  TO,  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA,  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY  OF  LIABILITY,  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF  THIS  SOFTWARE,  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cublas.h>
#include "gemv2_core.cuh"

#if(SM >= 30)

#define zgemvn_nb       (32)
#define zgemvn_ntcol    (4)
#define zgemvn_ept      (2)
#define zgemvn_width    (zgemvn_ntcol*zgemvn_ept)
#define zgemvn_by       (4)

#define zgemvt_nb       (32)
#define zgemvt_ntcol    (4)
#define zgemvt_ept      (2)
#define zgemvt_width    (zgemvt_ntcol*zgemvt_ept)
#define zgemvt_by       (4)

#else

#define zgemvn_nb               (64)
#define zgemvn_ntcol    		(8)
#define zgemvn_ept              (2)
#define zgemvn_width    (zgemvn_ntcol*zgemvn_ept)
#define zgemvn_by               (1)

#define zgemvt_nb               (64)
#define zgemvt_ntcol    		(8)
#define zgemvt_ept              (2)
#define zgemvt_width    (zgemvt_ntcol*zgemvt_ept)
#define zgemvt_by               (1)
#endif


extern "C"
int kblas_zscal_async(int n, cuDoubleComplex alpha, cuDoubleComplex *x, int incx, cudaStream_t stream);

  
int kblas_zgemv2_driver(	char trans, int rows, int cols,
						cuDoubleComplex alpha, cuDoubleComplex *dA, int lda, 
						cuDoubleComplex *dX, int incx, 
						cuDoubleComplex  beta, cuDoubleComplex *dY, int incy,
						cudaStream_t stream)
{	
	if(trans == 'n' || trans == 'N')
	{
		// scaling with beta
		kblas_zscal_async(rows, beta, dY, incy, stream);
		
		int mod_r = rows % zgemvn_nb;
		int mod_c = cols % zgemvn_width;	
		
		int blocks = rows/zgemvn_nb;
		if(mod_r != 0) blocks += 1;
		
		const int thread_x = zgemvn_nb;
		const int thread_y = zgemvn_ntcol; 
		const int ept = zgemvn_ept;
		
		int threshold = mod_c / ept; 
		int ept_ = mod_c % ept;
		dim3 dimBlock(thread_x, thread_y);
		dim3 dimGrid(blocks, zgemvn_by);
		switch(ept_)
		{
			case 0: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 0><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			case 1: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 1><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			case 2: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 2><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			case 3: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 3><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			case 4: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 4><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			case 5: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 5><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			case 6: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 6><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			case 7: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 7><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			case 8: gemvn<cuDoubleComplex, zgemvn_nb, zgemvn_ntcol, ept, zgemvn_width, 8><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold); break;
			default: printf("irregular part %d is not supported, please extend the case statement of zgemv\n", ept_); exit(1);
		}
	}	// end of non-transpose case
	else if(trans == 't' || trans == 'T' || trans == 'c' || trans == 'C')
	{
		// scaling with beta
		kblas_zscal_async(cols, beta, dY, incy, stream);
		
		int mod_r = rows % zgemvt_nb;
		int mod_c = cols % zgemvt_width;
		
		int blocks = cols/zgemvt_width;
		if(mod_c != 0) blocks += 1;
		
		const int thread_x = zgemvt_nb;
		const int thread_y = zgemvt_ntcol;
		const int ept = zgemvt_ept;
		
		int threshold = mod_c / ept;
		int ept_ = mod_c % ept;
		
		dim3 dimBlock(thread_x, thread_y);
		dim3 dimGrid(blocks, zgemvt_by);
		
		int conj;
		if(trans == 'c' || trans == 'C')conj = 1;
		else conj = 0;
		//printf("modr = %d, modc = %d, threshold = %d, ept_ = %d \n", mod_r, mod_c, threshold, ept_);
		switch(ept_)
		{
			case 0: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 0><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			case 1: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 1><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			case 2: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 2><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			case 3: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 3><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			case 4: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 4><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			case 5: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 5><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			case 6: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 6><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			case 7: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 7><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			case 8: gemvt<cuDoubleComplex, zgemvt_nb, zgemvt_ntcol, ept, zgemvt_width, 8><<<dimGrid, dimBlock, 0, stream>>>(rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, mod_r, mod_c, threshold, conj); break;
			default: printf("irregular part %d is not supported, please extend the case statement of zgemv\n", ept_); exit(1);
		}
	}
	else
	{	
		printf("ZGEMV error: Unrecognized transpose mode %c \n", trans);
		return -1;
	}
	
	return 0;
}

extern "C"
int kblas_zgemv2(char trans, int rows, int cols,
				cuDoubleComplex alpha, cuDoubleComplex *dA, int lda, 
				cuDoubleComplex *dX, int incx, 
				cuDoubleComplex  beta, cuDoubleComplex *dY, int incy)
{
	return kblas_zgemv2_driver(	trans, rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, 0);
}

extern "C"
int kblas_zgemv2_async(	char trans, int rows, int cols,
						cuDoubleComplex alpha, cuDoubleComplex *dA, int lda, 
						cuDoubleComplex *dX, int incx, 
						cuDoubleComplex  beta, cuDoubleComplex *dY, int incy,
						cudaStream_t stream)
{
	return kblas_zgemv2_driver(	trans, rows, cols, alpha, dA, lda, dX, incx, beta, dY, incy, stream);
}
