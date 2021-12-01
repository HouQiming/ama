#include "cutlass/cutlass.h"
#include "cutlass/gemm/device/gemm.h"
#include "cutlass/gemm/kernel/default_gemm.h"
#include "cutlass/gemm/device/default_gemm_configuration.h"

using OperatorClass = cutlass::arch::OpClassSimt;
using ArchTag = cutlass::arch::Sm86;
using ThreadblockShape = cutlass::gemm::device::DefaultGemmConfiguration<
	OperatorClass, ArchTag, float, float, float, float>::ThreadblockShape;
/// Warp-level tile size (concept: GemmShape)
using WarpShape = cutlass::gemm::device::DefaultGemmConfiguration<
	OperatorClass, ArchTag, float, float, float, float>::WarpShape;
/// Instruction-level tile size (concept: GemmShape)
using InstructionShape = cutlass::gemm::device::DefaultGemmConfiguration<
	OperatorClass, ArchTag, float, float, float, float>::InstructionShape;
/// Epilogue output operator
using EpilogueOutputOp = cutlass::gemm::device::DefaultGemmConfiguration<
	OperatorClass, ArchTag, float, float, float, float>::EpilogueOutputOp;
using ThreadblockSwizzle = cutlass::gemm::threadblock::GemmIdentityThreadblockSwizzle<>;
static const int kStages = cutlass::gemm::device::DefaultGemmConfiguration<OperatorClass, ArchTag, float, float, float, float>::kStages;
/// Access granularity of A matrix in units of elements
static const int kAlignmentA = cutlass::gemm::device::DefaultGemmConfiguration<OperatorClass, ArchTag, float, float, float, float>::kAlignmentA;
/// Access granularity of B matrix in units of elements
static const int kAlignmentB = cutlass::gemm::device::DefaultGemmConfiguration<OperatorClass, ArchTag, float, float, float, float>::kAlignmentB;
static const bool kSplitKSerial = false;
template<typename LayoutA, typename LayoutB, typename LayoutC>
using CutlassGemmKernel = typename cutlass::gemm::kernel::DefaultGemm<
  float,
  LayoutA,
  kAlignmentA,
  float,
  LayoutB,
  kAlignmentB,
  float,
  LayoutC,
  float,
  OperatorClass,
  ArchTag,
  ThreadblockShape,
  WarpShape,
  InstructionShape,
  EpilogueOutputOp,
  ThreadblockSwizzle,
  kStages,
  kSplitKSerial,
  cutlass::gemm::device::DefaultGemmConfiguration<OperatorClass, ArchTag, float, float, float, float>::Operator
>::GemmKernel;

template<typename T>
void check(T result, char const *const func, const char *const file, int const line) {
	if (result) {
		fprintf(stderr, "CUDA error at %s:%d code=%d \"%s\" \n",
				file, line, static_cast<unsigned int>(result), func);
		// Make sure we call CUDA Device Reset before exiting
		abort();
	}
}
#define checkCudaErrors(val)           check((val), #val, __FILE__, __LINE__)

template<typename LayoutA, typename LayoutB, typename LayoutC>
__device__ void run(
	cutlass::gemm::GemmCoord problem_size,
	cutlass::TensorRef<float const, LayoutA> ref_A,
	cutlass::TensorRef<float const, LayoutB> ref_B,
	cutlass::TensorRef<float const, LayoutC> ref_C,
	cutlass::TensorRef<float, LayoutC> ref_D,
	typename EpilogueOutputOp::Params epilogue = EpilogueOutputOp::Params()
	//int split_k_slices=1
) {
	using TGemmKernel = CutlassGemmKernel<LayoutA, LayoutB, LayoutC>;
	ThreadblockSwizzle threadblock_swizzle;
	cutlass::gemm::GemmCoord grid_shape = threadblock_swizzle.get_tiled_shape(
		problem_size, 
		{ThreadblockShape::kM, ThreadblockShape::kN, ThreadblockShape::kK},
		1//split_k_slices
	);
	
	// Initialize the Params structure
	typename TGemmKernel::Params params{
		problem_size,
		grid_shape,
		ref_A.non_const_ref(),
		ref_B.non_const_ref(),
		ref_C.non_const_ref(),
		ref_D,
		epilogue,
		NULL //workspace
	};
	dim3 grid = threadblock_swizzle.get_grid_shape(params.grid_tiled_shape);
	dim3 block(TGemmKernel::kThreadCount, 1, 1);
	int smem_size = int(sizeof(typename TGemmKernel::SharedStorage));
	cutlass::Kernel<TGemmKernel><<<grid, block, smem_size>>>(params);
}

__global__ void gpu_master(float* a, float* b, float* c, int n) {
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	a += i * n * n;
	b += i * n * n;
	c += i * n * n;
	run<cutlass::layout::RowMajor, cutlass::layout::RowMajor, cutlass::layout::RowMajor>(
		{n,n,n},
		{a, n},
		{b, n},
		{c, n},
		{c, n},
		{1.f, 0.f}
	);
}

int main() {
	float* a = NULL;
	float* b = NULL;
	float* c = NULL;
	int n = 128;
	int m = 7 * 32;
	cudaMallocHost((void**) & a, m * n * n * sizeof(float));
	cudaMallocHost((void**) & b, m * n * n * sizeof(float));
	cudaMallocHost((void**) & c, m * n * n * sizeof(float));
	cudaMemset(a, 0x3f, m * n * n * sizeof(float));
	cudaMemset(b, 0x3f, m * n * n * sizeof(float));
	cudaMemset(c, 0x3f, m * n * n * sizeof(float));
	gpu_master<<<7, 32>>>(a, b, c, n);
	checkCudaErrors(cudaPeekAtLastError());
	float* cc = (float*)malloc(m * n * n * sizeof(float));
	cudaMemcpy(cc, c, m * n * n * sizeof(float), cudaMemcpyDeviceToHost);
	printf("%f %f %f\n", cc[0], cc[n * n + 1], cc[n * n * (m - 1) + n * n - 7]);
	return 0;
}
