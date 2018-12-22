#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include "device_launch_parameters.h"

#include "matrix.cuh"

__global__ void rayTracingGPU(uchar4 * ptr, float * modelView, float * projection, int * viewPort)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x;
	int y = threadIdx.y + blockIdx.y * blockDim.y;
	int offset = x + y * blockDim.x*gridDim.x; //index offset in pbo

	float objectCoord[3];

	UnProject(x, y, 0, modelView, projection, viewPort, objectCoord);

	ptr[offset].x = 0;
	ptr[offset].y = 255;
	ptr[offset].z = 0;
	ptr[offset].w = 255;
}

__host__ void lunchRayTracer(uchar4 *ptr, float * modelView, float * projection, int * viewPort)
{
	dim3 grids(640 / 16, 480 / 16);
	dim3 threads(16, 16);
	rayTracingGPU << <grids, threads >> > (ptr, modelView, projection, viewPort);
}

