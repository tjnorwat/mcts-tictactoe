#include <stdio.h>

// Kernel function to print "Hello, World!" from the GPU
__global__ void helloKernel() {
    printf("Hello, World! from GPU thread %d\n", threadIdx.x);
}

int main() {
    // Print "Hello, World!" from the CPU
    printf("Hello, World! from CPU\n");
    
    // Launch the kernel with 5 threads
    helloKernel<<<1, 5>>>();
    
    // Wait for the GPU to finish
    cudaDeviceSynchronize();
    
    return 0;
}
