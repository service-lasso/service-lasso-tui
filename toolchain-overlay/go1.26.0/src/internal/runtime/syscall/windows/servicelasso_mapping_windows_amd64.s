//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Acquisition/representation only. MEM_IMAGE/protection/range never establish
// creator, loaded bytes, root writer, no-COW or authority to read any wire data.
// Outer actual entry preserves BX/SI/DI/native state and charges all borrowers.
TEXT ·serviceLassoObserveRootMapping<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoRootMappingOriginal(SB), BX
	// Observe the entire source-owned fixed occurrence before any mutation;
	// a foreign/partial nonzero native output cannot be overwritten as fresh.
	MOVQ BX, DX
	MOVQ $const_serviceLassoRootMappingExtent, R8
mapping_fresh:
	CMPQ 0(DX), $0
	JNE mapping_refused
	ADDQ $8, DX
	SUBQ $8, R8
	JNE mapping_fresh
	MOVQ $1, serviceLassoRootMappingStorage_Attempted(BX)
	LEAQ ·serviceLassoNativeRoot(SB), CX
	MOVQ CX, serviceLassoRootMappingStorage_Address(BX)
	LEAQ serviceLassoRootMappingStorage_Call(BX), AX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	LEAQ serviceLassoRootMappingStorage_Information(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ $48, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVQ ·serviceLassoVirtualQuery(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $3, StdCallInfo_N(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE mapping_failed
	// VirtualQuery returns SIZE_T, not BOOL/DWORD: retain/test all64 bits.
	CMPQ serviceLassoRootMappingStorage_Call+StdCallInfo_R1(BX), $48
	JNE mapping_failed
	LEAQ serviceLassoRootMappingStorage_Information(BX), DX
	CMPL serviceLassoMemoryBasicInformation_State(DX), $0x1000
	JNE mapping_failed
	CMPL serviceLassoMemoryBasicInformation_Type(DX), $0x1000000
	JNE mapping_failed
	MOVL serviceLassoMemoryBasicInformation_Protect(DX), CX
	CMPL CX, $4
	JE mapping_protection
	CMPL CX, $8
	JNE mapping_failed
mapping_protection:
	MOVQ serviceLassoMemoryBasicInformation_AllocationBase(DX), CX
	TESTQ CX, CX
	JE mapping_failed
	CMPQ CX, serviceLassoRootMappingStorage_Address(BX)
	JA mapping_failed
	MOVQ serviceLassoMemoryBasicInformation_BaseAddress(DX), R8
	CMPQ CX, R8
	JA mapping_failed
	CMPQ R8, serviceLassoRootMappingStorage_Address(BX)
	JA mapping_failed
	MOVQ serviceLassoMemoryBasicInformation_RegionSize(DX), R9
	TESTQ R9, R9
	JE mapping_failed
	ADDQ R8, R9
	JC mapping_failed
	MOVQ R9, serviceLassoRootMappingStorage_RegionEnd(BX)
	MOVQ serviceLassoRootMappingStorage_Address(BX), CX
	ADDQ $const_serviceLassoNativeRootExtent, CX
	JC mapping_failed
	CMPQ CX, R9
	JA mapping_failed
	MOVQ $1, serviceLassoRootMappingStorage_Returned(BX)
	MOVQ $1, AX
	RET
mapping_failed:
	MOVQ $1, serviceLassoRootMappingStorage_Failure(BX)
	MOVQ $1, serviceLassoRootMappingStorage_Returned(BX)
mapping_refused:
	XORQ AX, AX
	RET
