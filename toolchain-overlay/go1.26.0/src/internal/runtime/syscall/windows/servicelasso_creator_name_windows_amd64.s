//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Same phase0/1 and actual completed snapshot row index R11 as mapping.
// The readonly caller retains module/mapping/provider/writer lifetimes; no
// path, native result or occurrence flag supplies image authentication.
TEXT ·serviceLassoObserveCreatorMappedName<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE creator_name_before
 CMPQ R10, $1
 JNE creator_name_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), DX
 LEAQ ·serviceLassoCreatorMappingsAfter(SB), R8
 LEAQ ·serviceLassoCreatorNamesAfter(SB), R9
 LEAQ ·serviceLassoCreatorNamesAfterOriginal(SB), BX
 JMP creator_name_row
creator_name_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), DX
 LEAQ ·serviceLassoCreatorMappingsBefore(SB), R8
 LEAQ ·serviceLassoCreatorNamesBefore(SB), R9
 LEAQ ·serviceLassoCreatorNamesBeforeOriginal(SB), BX
creator_name_row:
 CMPQ serviceLassoCreatorModuleStorage_Returned(DX), $1
 JNE creator_name_refused
 CMPQ serviceLassoCreatorModuleStorage_Failure(DX), $0
 JNE creator_name_refused
 MOVQ serviceLassoCreatorModuleStorage_Count(DX), CX
 MOVL $const_serviceLassoCreatorModuleCapacity, AX
 CMPQ CX, AX
 JA creator_name_refused
 CMPQ R11, CX
 JAE creator_name_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorImageMappingExtent, AX
 JO creator_name_refused
 ADDQ AX, R8
 JC creator_name_refused
 CMPQ serviceLassoCreatorImageMappingStorage_Returned(R8), $1
 JNE creator_name_refused
 CMPQ serviceLassoCreatorImageMappingStorage_Failure(R8), $0
 JNE creator_name_refused
 MOVQ serviceLassoCreatorModuleStorage_Process(DX), CX
 CMPQ CX, serviceLassoCreatorImageMappingStorage_Process(R8)
 JNE creator_name_refused
 MOVQ R8, R10
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorNameExtent, AX
 JO creator_name_refused
 ADDQ AX, BX
 JC creator_name_refused
 MOVL $const_serviceLassoCreatorMappedNameUnits, AX
 SHLQ $1, AX
 IMULQ R11, AX
 JO creator_name_refused
 ADDQ AX, R9
 JC creator_name_refused
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorNameExtent, R8
creator_name_owner_fresh:
 CMPQ 0(DX), $0
 JNE creator_name_refused
 ADDQ $8, DX
 SUBQ $8, R8
 JNE creator_name_owner_fresh
 MOVQ R9, DX
 MOVL $const_serviceLassoCreatorMappedNameUnits, R8
creator_name_buffer_fresh:
 CMPW 0(DX), $0
 JNE creator_name_refused
 ADDQ $2, DX
 SUBQ $1, R8
 JNE creator_name_buffer_fresh
 MOVQ $1, serviceLassoCreatorNameStorage_Attempted(BX)
 MOVQ R9, serviceLassoCreatorNameStorage_Buffer(BX)
 MOVL $const_serviceLassoCreatorMappedNameUnits, CX
 MOVL CX, serviceLassoCreatorNameStorage_Capacity(BX)
 MOVQ serviceLassoCreatorImageMappingStorage_Process(R10), CX
 MOVQ CX, serviceLassoCreatorNameStorage_Process(BX)
 MOVQ serviceLassoCreatorImageMappingStorage_Address(R10), DX
 MOVQ DX, serviceLassoCreatorNameStorage_Address(BX)
 LEAQ serviceLassoCreatorNameStorage_Call(BX), AX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
 MOVQ DX, serviceLassoNativeCallStorage_Arguments+8(AX)
 MOVQ R9, serviceLassoNativeCallStorage_Arguments+16(AX)
 MOVL serviceLassoCreatorNameStorage_Capacity(BX), CX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments+24(AX)
 MOVQ ·serviceLassoGetMappedFileName(SB), CX
 MOVQ CX, StdCallInfo_Fn(AX)
 MOVQ $4, StdCallInfo_N(AX)
 CALL ·serviceLassoNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_name_failed
 // DWORD result excludes terminator; equal capacity is truncation, not name.
 MOVL serviceLassoCreatorNameStorage_Call+StdCallInfo_R1(BX), CX
 MOVL CX, serviceLassoCreatorNameStorage_Count(BX)
 TESTQ CX, CX
 JE creator_name_failed
 MOVL serviceLassoCreatorNameStorage_Capacity(BX), DX
 CMPQ CX, DX
 JAE creator_name_failed
 MOVQ serviceLassoCreatorNameStorage_Buffer(BX), DX
 CMPW (DX)(CX*2), $0
 JNE creator_name_failed
creator_name_prefix:
 CMPW 0(DX), $0
 JE creator_name_failed
 ADDQ $2, DX
 SUBQ $1, CX
 JNE creator_name_prefix
 MOVQ $1, serviceLassoCreatorNameStorage_Returned(BX)
 MOVQ $1, AX
 RET
creator_name_failed:
 MOVQ $1, serviceLassoCreatorNameStorage_Failure(BX)
 MOVQ $1, serviceLassoCreatorNameStorage_Returned(BX)
creator_name_refused:
 XORQ AX, AX
 RET
