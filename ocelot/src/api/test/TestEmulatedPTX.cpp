#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>

#include <ocelot/ir/Module.h>

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

.visible .entry insert_high_word(
	.param .u64 insert_high_word_param_0
)
{
	.reg .b32 %r<2>;
	.reg .b64 %rd<8>;

	ld.param.u64 %rd1, [insert_high_word_param_0];
	.loc 1 7 3
	.loc 1 4 58, function_name $L__info_string0, inlined_at 1 7 3
	.loc 1 4 58, function_name .debug_str + 0, inlined_at 1 7 3
	cvta.to.global.u64 %rd2, %rd1;
	mov.u32 %r1, %tid.x;
	mul.wide.u32 %rd3, %r1, 8;
	add.s64 %rd4, %rd2, %rd3;
	ld.global.u64 %rd5, [%rd4];
	bfi.b64 %rd7, 1, %rd5, 32, 32;
	st.global.u64 [%rd4], %rd7;
	ret;
}

	.file 1 "insert_high_word.cu"
	.section .debug_str
	{
	.b8 0
$L__info_string0:
	.b8 105,110,115
	.b8 101,114,116,0
	}
)ptx";

	{
		std::stringstream source(ptx);
		ir::Module module((void*)ptx, source);
		const ir::PTXInstruction* bfi = nullptr;
		for(const ir::PTXStatement& statement : module.statements())
		{
			if(statement.directive == ir::PTXStatement::Instr && statement.instruction.opcode == ir::PTXInstruction::Bfi)
			{
				bfi = &statement.instruction;
				break;
			}
		}
		if(bfi == nullptr || bfi->pq.type != ir::PTXOperand::b64 || bfi->a.type != ir::PTXOperand::b64 ||
			bfi->b.type != ir::PTXOperand::u32 || bfi->c.type != ir::PTXOperand::u32)
		{
			std::cerr << "bfi.b64 operands were not normalized to b64, b64, u32, u32\n";
			return 1;
		}
	}

	std::array<std::uint64_t, 4> data{{0, 2, 4, 6}};
	void* arguments[] = {data.data()};
	ptx_run(ptx, 1, arguments, 4, 1, 1, 1, 1, 1, 0);

	const std::array<std::uint64_t, 4> expected{{0x100000000, 0x100000002, 0x100000004, 0x100000006}};
	if(data == expected) return 0;

	std::cerr << "PTX 8.7 sm_50 emulation returned";
	for(std::uint64_t value : data) std::cerr << ' ' << value;
	std::cerr << "; expected high 32-bit words set to 1\n";
	return 1;
}
