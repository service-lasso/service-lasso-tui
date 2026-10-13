//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Exact128B five-argument RPM occurrence. Count is native SIZE_T64, unlike
// DWORD ReadFile. Existing NativeCall reads prefix/args only; no400B access.
// The complete fresh per-region caller must dominate this parameterized seam.
TEXT ·serviceLassoCreatorMemoryNativeCall<ABI0>(SB),NOSPLIT|NOFRAME,$0
 TESTQ AX, AX
 JE creator_memory_native_refused
 CMPQ StdCallInfo_N(AX), $5
 JNE creator_memory_native_refused
 CMPQ serviceLassoCreatorMemoryCall_Count(AX), $0
 JNE creator_memory_native_refused
 JMP ·serviceLassoNativeCall<ABI0>(SB)
creator_memory_native_refused:
 XORQ AX, AX
 RET

// Phase R10/module index R11. Serialized issuing primary owns both cursors.
// Whole valid-input provider, D/reserve and outer failure custody are caller
// prerequisites. This walk never authenticates loaded bytes or justifies gaps.
TEXT ·serviceLassoObserveCreatorMemory<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE creator_memory_before
 CMPQ R10, $1
 JNE creator_memory_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), DX
 LEAQ ·serviceLassoCreatorExtentsAfterOriginal(SB), R8
 LEAQ ·serviceLassoCreatorWalksAfterOriginal(SB), BX
 JMP creator_memory_row
creator_memory_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), DX
 LEAQ ·serviceLassoCreatorExtentsBeforeOriginal(SB), R8
 LEAQ ·serviceLassoCreatorWalksBeforeOriginal(SB), BX
creator_memory_row:
 CMPQ serviceLassoCreatorModuleStorage_Returned(DX), $1
 JNE creator_memory_refused
 CMPQ serviceLassoCreatorModuleStorage_Failure(DX), $0
 JNE creator_memory_refused
 MOVQ serviceLassoCreatorModuleStorage_Count(DX), CX
 MOVL $const_serviceLassoCreatorModuleCapacity, AX
 CMPQ CX, AX
 JA creator_memory_refused
 CMPQ R11, CX
 JAE creator_memory_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorExtentBytes, AX
 JO creator_memory_refused
 ADDQ AX, R8
 JC creator_memory_refused
 CMPQ serviceLassoCreatorExtentStorage_Returned(R8), $1
 JNE creator_memory_refused
 CMPQ serviceLassoCreatorExtentStorage_Failure(R8), $0
 JNE creator_memory_refused
 MOVQ serviceLassoCreatorModuleStorage_Process(DX), CX
 CMPQ CX, serviceLassoCreatorExtentStorage_Process(R8)
 JNE creator_memory_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorWalkExtent, AX
 JO creator_memory_refused
 ADDQ AX, BX
 JC creator_memory_refused
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorWalkExtent, AX
creator_memory_walk_fresh:
 CMPQ 0(DX), $0
 JNE creator_memory_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE creator_memory_walk_fresh
 MOVQ $1, serviceLassoCreatorWalkStorage_Attempted(BX)
 MOVQ R8, serviceLassoCreatorWalkStorage_ExtentOwner(BX)
 MOVQ R10, serviceLassoCreatorWalkStorage_Phase(BX)
 MOVQ R11, serviceLassoCreatorWalkStorage_ModuleIndex(BX)
 MOVQ serviceLassoCreatorExtentStorage_Process(R8), CX
 MOVQ CX, serviceLassoCreatorWalkStorage_Process(BX)
 MOVQ serviceLassoCreatorExtentStorage_Address(R8), CX
 MOVQ CX, serviceLassoCreatorWalkStorage_Base(BX)
 MOVQ CX, serviceLassoCreatorWalkStorage_AddressCursor(BX)
 MOVQ serviceLassoCreatorExtentStorage_ImageEnd(R8), DX
 MOVQ DX, serviceLassoCreatorWalkStorage_ImageEnd(BX)
 CMPQ CX, DX
 JAE creator_memory_failed
 MOVQ ·serviceLassoCreatorRegionUsed(SB), CX
 MOVQ CX, serviceLassoCreatorWalkStorage_FirstRegion(BX)
creator_memory_next:
 MOVQ ·serviceLassoCreatorRegionUsed(SB), CX
 MOVQ $const_serviceLassoCreatorRegionCapacity, DX
 CMPQ CX, DX
 JAE creator_memory_failed
 MOVQ CX, AX
 MOVQ $const_serviceLassoCreatorRegionQueryExtent, DX
 MULQ DX
 TESTQ DX, DX
 JNE creator_memory_failed
 LEAQ ·serviceLassoCreatorRegionQueries(SB), R8
 ADDQ AX, R8
 JC creator_memory_failed
 MOVQ R8, DX
 MOVQ $const_serviceLassoCreatorRegionQueryExtent, AX
creator_memory_query_fresh:
 CMPQ 0(DX), $0
 JNE creator_memory_failed
 ADDQ $8, DX
 SUBQ $8, AX
 JNE creator_memory_query_fresh
 MOVQ CX, AX
 MOVQ $const_serviceLassoCreatorMemoryCallExtent, DX
 MULQ DX
 TESTQ DX, DX
 JNE creator_memory_failed
 LEAQ ·serviceLassoCreatorRegionReads(SB), R9
 ADDQ AX, R9
 JC creator_memory_failed
 MOVQ R9, DX
 MOVQ $const_serviceLassoCreatorMemoryCallExtent, AX
creator_memory_read_fresh:
 CMPQ 0(DX), $0
 JNE creator_memory_failed
 ADDQ $8, DX
 SUBQ $8, AX
 JNE creator_memory_read_fresh
 MOVQ CX, AX
 MOVQ $const_serviceLassoCreatorRegionExtent, DX
 MULQ DX
 TESTQ DX, DX
 JNE creator_memory_failed
 LEAQ ·serviceLassoCreatorRegionOriginals(SB), R10
 ADDQ AX, R10
 JC creator_memory_failed
 MOVQ R10, DX
 MOVQ $const_serviceLassoCreatorRegionExtent, AX
creator_memory_region_fresh:
 CMPQ 0(DX), $0
 JNE creator_memory_failed
 ADDQ $8, DX
 SUBQ $8, AX
 JNE creator_memory_region_fresh
 MOVQ $1, serviceLassoCreatorRegionStorage_Attempted(R10)
 MOVQ CX, serviceLassoCreatorRegionStorage_QueryIndex(R10)
 MOVQ CX, serviceLassoCreatorRegionStorage_ReadIndex(R10)
 INCQ ·serviceLassoCreatorRegionUsed(SB)
 INCQ serviceLassoCreatorWalkStorage_Used(BX)
 MOVQ serviceLassoCreatorWalkStorage_Process(BX), DX
 MOVQ DX, serviceLassoCreatorRegionStorage_Process(R10)
 MOVQ DX, serviceLassoCreatorRegionQuery_Process(R8)
 MOVQ serviceLassoCreatorWalkStorage_AddressCursor(BX), DX
 MOVQ DX, serviceLassoCreatorRegionStorage_Address(R10)
 MOVQ DX, serviceLassoCreatorRegionQuery_Address(R8)
 MOVQ serviceLassoCreatorWalkStorage_Base(BX), DX
 MOVQ DX, serviceLassoCreatorRegionStorage_ModuleBase(R10)
 MOVQ DX, serviceLassoCreatorRegionQuery_ModuleBase(R8)
 MOVQ serviceLassoCreatorWalkStorage_ImageEnd(BX), DX
 MOVQ DX, serviceLassoCreatorRegionStorage_ImageEnd(R10)
 MOVQ DX, serviceLassoCreatorRegionQuery_ImageEnd(R8)
 MOVQ $1, serviceLassoCreatorRegionQuery_Attempted(R8)
 LEAQ serviceLassoCreatorRegionQuery_Call(R8), AX
 MOVQ serviceLassoCreatorWalkStorage_Process(BX), DX
 MOVQ DX, serviceLassoNativeCallStorage_Arguments(AX)
 MOVQ serviceLassoCreatorWalkStorage_AddressCursor(BX), DX
 MOVQ DX, serviceLassoNativeCallStorage_Arguments+8(AX)
 LEAQ serviceLassoCreatorRegionQuery_Information(R8), DX
 MOVQ DX, serviceLassoNativeCallStorage_Arguments+16(AX)
 MOVQ $48, serviceLassoNativeCallStorage_Arguments+24(AX)
 MOVQ ·serviceLassoVirtualQueryEx(SB), DX
 MOVQ DX, StdCallInfo_Fn(AX)
 MOVQ $4, StdCallInfo_N(AX)
 CALL ·serviceLassoNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_memory_query_call_failed
 CALL ·serviceLassoCreatorMemoryCurrentRows<ABI0>(SB)
 CMPQ serviceLassoCreatorRegionQuery_Call+StdCallInfo_R1(R8), $48
 JNE creator_memory_query_failed
 LEAQ serviceLassoCreatorRegionQuery_Information(R8), DX
 MOVQ serviceLassoMemoryBasicInformation_AllocationBase(DX), CX
 CMPQ CX, serviceLassoCreatorWalkStorage_Base(BX)
 JNE creator_memory_query_failed
 MOVQ serviceLassoMemoryBasicInformation_BaseAddress(DX), CX
 CMPQ CX, serviceLassoCreatorWalkStorage_AddressCursor(BX)
 JA creator_memory_query_failed
 MOVQ serviceLassoMemoryBasicInformation_RegionSize(DX), AX
 TESTQ AX, AX
 JE creator_memory_query_failed
 ADDQ CX, AX
 JC creator_memory_query_failed
 MOVQ AX, serviceLassoCreatorRegionQuery_RegionEnd(R8)
 CMPQ AX, serviceLassoCreatorWalkStorage_AddressCursor(BX)
 JBE creator_memory_query_failed
 MOVQ $1, serviceLassoCreatorRegionQuery_Returned(R8)
 CMPQ AX, serviceLassoCreatorWalkStorage_ImageEnd(BX)
 JBE creator_memory_clipped
 MOVQ serviceLassoCreatorWalkStorage_ImageEnd(BX), AX
creator_memory_clipped:
 SUBQ serviceLassoCreatorWalkStorage_AddressCursor(BX), AX
 MOVQ AX, serviceLassoCreatorRegionStorage_Extent(R10)
 CMPL serviceLassoMemoryBasicInformation_State(DX), $0x2000
 JE creator_memory_reserved
 CMPL serviceLassoMemoryBasicInformation_State(DX), $0x1000
 JNE creator_memory_region_failed
 CMPL serviceLassoMemoryBasicInformation_Type(DX), $0x1000000
 JNE creator_memory_region_failed
 MOVL serviceLassoMemoryBasicInformation_Protect(DX), CX
 TESTL $0x100, CX
 JNE creator_memory_unreadable
 ANDL $0xff, CX
 CMPL CX, $2
 JE creator_memory_copy
 CMPL CX, $4
 JE creator_memory_copy
 CMPL CX, $8
 JE creator_memory_copy
 CMPL CX, $0x20
 JE creator_memory_copy
 CMPL CX, $0x40
 JE creator_memory_copy
 CMPL CX, $0x80
 JE creator_memory_copy
 JMP creator_memory_unreadable
creator_memory_reserved:
 MOVQ $2, serviceLassoCreatorRegionStorage_Disposition(R10)
 JMP creator_memory_advance
creator_memory_unreadable:
 MOVQ $3, serviceLassoCreatorRegionStorage_Disposition(R10)
 JMP creator_memory_advance
creator_memory_copy:
 MOVQ ·serviceLassoCreatorMemoryArenaUsed(SB), CX
 MOVQ CX, serviceLassoCreatorRegionStorage_ArenaOffset(R10)
 MOVQ CX, R11
 ADDQ serviceLassoCreatorRegionStorage_Extent(R10), R11
 JC creator_memory_region_failed
 MOVQ $const_serviceLassoCreatorMemoryBytesCapacity, DX
 CMPQ R11, DX
 JA creator_memory_region_failed
 LEAQ ·serviceLassoCreatorMemoryBytes(SB), DX
 ADDQ CX, DX
 JC creator_memory_region_failed
 MOVQ DX, AX
 MOVQ serviceLassoCreatorRegionStorage_Extent(R10), CX
creator_memory_bytes_fresh:
 CMPB 0(AX), $0
 JNE creator_memory_region_failed
 ADDQ $1, AX
 SUBQ $1, CX
 JNE creator_memory_bytes_fresh
 MOVQ R11, ·serviceLassoCreatorMemoryArenaUsed(SB)
 MOVQ $1, serviceLassoCreatorRegionStorage_Disposition(R10)
 MOVQ R9, AX
 MOVQ serviceLassoCreatorWalkStorage_Process(BX), CX
 MOVQ CX, serviceLassoCreatorMemoryCall_Arguments(AX)
 MOVQ serviceLassoCreatorRegionStorage_Address(R10), CX
 MOVQ CX, serviceLassoCreatorMemoryCall_Arguments+8(AX)
 MOVQ CX, serviceLassoCreatorMemoryCall_Offset(AX)
 MOVQ DX, serviceLassoCreatorMemoryCall_Arguments+16(AX)
 MOVQ serviceLassoCreatorRegionStorage_Extent(R10), CX
 MOVQ CX, serviceLassoCreatorMemoryCall_Arguments+24(AX)
 MOVQ CX, serviceLassoCreatorMemoryCall_Requested(AX)
 LEAQ serviceLassoCreatorMemoryCall_Count(AX), CX
 MOVQ CX, serviceLassoCreatorMemoryCall_Arguments+32(AX)
 MOVQ ·serviceLassoReadProcessMemory(SB), CX
 MOVQ CX, StdCallInfo_Fn(AX)
 MOVQ $5, StdCallInfo_N(AX)
 CALL ·serviceLassoCreatorMemoryNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_memory_region_failed
 CALL ·serviceLassoCreatorMemoryCurrentRows<ABI0>(SB)
 MOVQ StdCallInfo_R1(R9), AX
 CALL ·serviceLassoNativeBOOL<ABI0>(SB)
 TESTQ AX, AX
 JE creator_memory_region_failed
 MOVQ serviceLassoCreatorMemoryCall_Count(R9), CX
 CMPQ CX, serviceLassoCreatorRegionStorage_Extent(R10)
 JNE creator_memory_region_failed
creator_memory_advance:
 MOVQ $1, serviceLassoCreatorRegionStorage_Returned(R10)
 MOVQ serviceLassoCreatorRegionStorage_Extent(R10), CX
 ADDQ CX, serviceLassoCreatorWalkStorage_AddressCursor(BX)
 JC creator_memory_region_failed
 MOVQ serviceLassoCreatorWalkStorage_ImageEnd(BX), CX
 CMPQ serviceLassoCreatorWalkStorage_AddressCursor(BX), CX
 JA creator_memory_region_failed
 JB creator_memory_next
 MOVQ $1, serviceLassoCreatorWalkStorage_Returned(BX)
 MOVQ $1, AX
 RET
creator_memory_query_call_failed:
 CALL ·serviceLassoCreatorMemoryCurrentRows<ABI0>(SB)
creator_memory_query_failed:
 MOVQ $1, serviceLassoCreatorRegionQuery_Failure(R8)
 MOVQ $1, serviceLassoCreatorRegionQuery_Returned(R8)
creator_memory_region_failed:
 CALL ·serviceLassoCreatorMemoryCurrentRows<ABI0>(SB)
 MOVQ $1, serviceLassoCreatorRegionStorage_Failure(R10)
 MOVQ $1, serviceLassoCreatorRegionStorage_Returned(R10)
creator_memory_failed:
 MOVQ $1, serviceLassoCreatorWalkStorage_Failure(BX)
 MOVQ $1, serviceLassoCreatorWalkStorage_Returned(BX)
creator_memory_refused:
 XORQ AX, AX
 RET

// Only called after this walk has reserved at least one complete fresh row.
// BX is the actual source-owned walk; recompute volatile row addresses rather
// than assuming NativeCall/StdCall leaves scratch registers unchanged.
TEXT ·serviceLassoCreatorMemoryCurrentRows<ABI0>(SB),NOSPLIT|NOFRAME,$0
 MOVQ serviceLassoCreatorWalkStorage_FirstRegion(BX), CX
 ADDQ serviceLassoCreatorWalkStorage_Used(BX), CX
 DECQ CX
 MOVQ CX, AX
 SHLQ $9, AX
 LEAQ ·serviceLassoCreatorRegionQueries(SB), R8
 ADDQ AX, R8
 MOVQ CX, AX
 SHLQ $7, AX
 LEAQ ·serviceLassoCreatorRegionReads(SB), R9
 ADDQ AX, R9
 IMULQ $const_serviceLassoCreatorRegionExtent, CX
 LEAQ ·serviceLassoCreatorRegionOriginals(SB), R10
 ADDQ CX, R10
 RET
