#include <array>
#include <iostream>

extern "C" void ptx_run(const char* source, int n_args, void* args[],
	int block_x, int block_y, int block_z,
	int grid_x, int grid_y, int grid_z, int shared_mem_size);

int main()
{
	// Exercise PTX 8.7 sm_50 execution with NVRTC 12.8-style metadata and PTX-legal debug-string variants.
	const char* ptx = R"ptx(
.version 8.7
.target sm_50
.address_size 64

.visible .entry increment(
	.param .u64 increment_param_0
)
{
	.reg .b32 %r<4>;
	.reg .b64 %rd<5>;

	ld.param.u64 %rd1, [increment_param_0];
	.loc 1 7 3
	.loc 1 4 58, function_name $L__info_string0, inlined_at 1 7 3
	.loc 1 4 58, function_name .debug_str + 0, inlined_at 1 7 3
	cvta.to.global.u64 %rd2, %rd1;
	mov.u32 %r1, %tid.x;
	mul.wide.u32 %rd3, %r1, 4;
	add.s64 %rd4, %rd2, %rd3;
	ld.global.u32 %r2, [%rd4];
	add.s32 %r3, %r2, 1;
	st.global.u32 [%rd4], %r3;
	ret;
}

	.file 1 "increment.cu"
	.section .debug_str
	{
	.b8 0
$L__info_string0:
	.b8 105,110,99,114,101
	.b8 109,101,110,116,0
	}
)ptx";

	std::array<unsigned int, 4> data{{1, 2, 3, 4}};
	void* arguments[] = {data.data()};
	ptx_run(ptx, 1, arguments, 4, 1, 1, 1, 1, 1, 0);

	const std::array<unsigned int, 4> expected{{2, 3, 4, 5}};
	if(data == expected) return 0;

	std::cerr << "PTX 8.7 sm_50 emulation returned";
	for(unsigned int value : data) std::cerr << ' ' << value;
	std::cerr << "; expected 2 3 4 5\n";
	return 1;
}
