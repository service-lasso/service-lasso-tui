//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Serialized source-owned reader only. Phase R10/index R11 identify the exact
// completed file metadata occurrence; no buffer/cursor/result finances entry.
TEXT ·serviceLassoReadCreatorImageFile<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE creator_read_before
 CMPQ R10, $1
 JNE creator_read_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), DX
 LEAQ ·serviceLassoCreatorMetadataAfterOriginal(SB), R8
 LEAQ ·serviceLassoCreatorFileReadsAfterOriginal(SB), BX
 JMP creator_read_row
creator_read_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), DX
 LEAQ ·serviceLassoCreatorMetadataBeforeOriginal(SB), R8
 LEAQ ·serviceLassoCreatorFileReadsBeforeOriginal(SB), BX
creator_read_row:
 CMPQ serviceLassoCreatorModuleStorage_Returned(DX), $1
 JNE creator_read_refused
 CMPQ serviceLassoCreatorModuleStorage_Failure(DX), $0
 JNE creator_read_refused
 MOVQ serviceLassoCreatorModuleStorage_Count(DX), CX
 MOVL $const_serviceLassoCreatorModuleCapacity, AX
 CMPQ CX, AX
 JA creator_read_refused
 CMPQ R11, CX
 JAE creator_read_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorMetadataExtent, AX
 JO creator_read_refused
 ADDQ AX, R8
 JC creator_read_refused
 CMPQ serviceLassoCreatorMetadataStorage_Returned(R8), $1
 JNE creator_read_refused
 CMPQ serviceLassoCreatorMetadataStorage_Failure(R8), $0
 JNE creator_read_refused
 MOVQ serviceLassoCreatorModuleStorage_Process(DX), CX
 CMPQ CX, serviceLassoCreatorMetadataStorage_Process(R8)
 JNE creator_read_refused
 MOVQ R8, R10
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorFileReadExtent, AX
 JO creator_read_refused
 ADDQ AX, BX
 JC creator_read_refused
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorFileReadExtent, AX
creator_read_owner_fresh:
 CMPQ 0(DX), $0
 JNE creator_read_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE creator_read_owner_fresh
 MOVQ $1, serviceLassoCreatorFileReadStorage_Attempted(BX)
 MOVQ R10, serviceLassoCreatorFileReadStorage_Metadata(BX)
 MOVQ serviceLassoCreatorMetadataStorage_File(R10), CX
 MOVQ CX, serviceLassoCreatorFileReadStorage_File(BX)
 TESTQ CX, CX
 JE creator_read_failed
 CMPQ CX, $-1
 JE creator_read_failed
 MOVQ serviceLassoCreatorMetadataStorage_Process(R10), CX
 MOVQ CX, serviceLassoCreatorFileReadStorage_Process(BX)
 MOVQ serviceLassoCreatorMetadataStorage_Address(R10), CX
 MOVQ CX, serviceLassoCreatorFileReadStorage_Address(BX)
 MOVQ serviceLassoCreatorMetadataStorage_Standard+serviceLassoFileStandardInformation_EndOfFile(R10), CX
 MOVQ CX, serviceLassoCreatorFileReadStorage_Extent(BX)
 CMPQ CX, $0
 JL creator_read_failed
 MOVQ ·serviceLassoCreatorFileArenaUsed(SB), R8
 MOVQ R8, serviceLassoCreatorFileReadStorage_ArenaOffset(BX)
 MOVQ R8, serviceLassoCreatorFileReadStorage_CallsOffset(BX)
 MOVQ R8, R9
 ADDQ CX, R9
 JC creator_read_failed
 MOVQ $const_serviceLassoCreatorFileBytesCapacity, DX
 CMPQ R9, DX
 JA creator_read_failed
 TESTQ CX, CX
 JE creator_read_allocated
 LEAQ ·serviceLassoCreatorFileBytes(SB), DX
 ADDQ R8, DX
 JC creator_read_failed
 MOVQ CX, R10
creator_read_bytes_fresh:
 CMPB 0(DX), $0
 JNE creator_read_failed
 ADDQ $1, DX
 SUBQ $1, R10
 JNE creator_read_bytes_fresh
 MOVQ R8, AX
 MOVQ $const_serviceLassoBodyReadCallExtent, CX
 MULQ CX
 TESTQ DX, DX
 JNE creator_read_failed
 LEAQ ·serviceLassoCreatorFileReadCalls(SB), R8
 ADDQ AX, R8
 JC creator_read_failed
 MOVQ serviceLassoCreatorFileReadStorage_Extent(BX), AX
 MULQ CX
 TESTQ DX, DX
 JNE creator_read_failed
 MOVQ AX, R10
creator_read_calls_fresh:
 CMPQ 0(R8), $0
 JNE creator_read_failed
 ADDQ $8, R8
 SUBQ $8, R10
 JNE creator_read_calls_fresh
creator_read_allocated:
 // The entire region remains retained on any later failure; no cursor reset.
 MOVQ R9, ·serviceLassoCreatorFileArenaUsed(SB)
 CMPQ serviceLassoCreatorFileReadStorage_Extent(BX), $0
 JE creator_read_complete
creator_read_next:
 MOVQ serviceLassoCreatorFileReadStorage_Used(BX), AX
 CMPQ AX, serviceLassoCreatorFileReadStorage_Extent(BX)
 JAE creator_read_failed
 ADDQ serviceLassoCreatorFileReadStorage_CallsOffset(BX), AX
 JC creator_read_failed
 SHLQ $7, AX
 LEAQ ·serviceLassoCreatorFileReadCalls(SB), DX
 ADDQ DX, AX
 JC creator_read_failed
 MOVQ serviceLassoCreatorFileReadStorage_Completed(BX), DX
 MOVQ DX, serviceLassoBodyReadCall_Offset(AX)
 MOVQ serviceLassoCreatorFileReadStorage_Extent(BX), R8
 SUBQ DX, R8
 MOVL $0xffffffff, CX
 CMPQ R8, CX
 JBE creator_read_request
 MOVQ CX, R8
creator_read_request:
 MOVQ R8, serviceLassoBodyReadCall_Requested(AX)
 MOVQ R8, serviceLassoBodyReadCall_Arguments+16(AX)
 LEAQ ·serviceLassoCreatorFileBytes(SB), CX
 ADDQ serviceLassoCreatorFileReadStorage_ArenaOffset(BX), CX
 JC creator_read_failed
 ADDQ DX, CX
 JC creator_read_failed
 MOVQ CX, serviceLassoBodyReadCall_Arguments+8(AX)
 MOVQ serviceLassoCreatorFileReadStorage_File(BX), CX
 MOVQ CX, serviceLassoBodyReadCall_Arguments(AX)
 LEAQ serviceLassoBodyReadCall_Count(AX), CX
 MOVQ CX, serviceLassoBodyReadCall_Arguments+24(AX)
 MOVQ ·serviceLassoReadFile(SB), CX
 MOVQ CX, StdCallInfo_Fn(AX)
 MOVQ $5, StdCallInfo_N(AX)
 INCQ serviceLassoCreatorFileReadStorage_Used(BX)
 CALL ·serviceLassoBodyReadNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_read_failed
 MOVQ serviceLassoCreatorFileReadStorage_Used(BX), DX
 DECQ DX
 ADDQ serviceLassoCreatorFileReadStorage_CallsOffset(BX), DX
 SHLQ $7, DX
 LEAQ ·serviceLassoCreatorFileReadCalls(SB), CX
 ADDQ CX, DX
 MOVQ StdCallInfo_R1(DX), AX
 CALL ·serviceLassoNativeBOOL<ABI0>(SB)
 TESTQ AX, AX
 JE creator_read_failed
 MOVL serviceLassoBodyReadCall_Count(DX), CX
 TESTQ CX, CX
 JE creator_read_failed
 CMPQ CX, serviceLassoBodyReadCall_Requested(DX)
 JA creator_read_failed
 ADDQ CX, serviceLassoCreatorFileReadStorage_Completed(BX)
 JC creator_read_failed
 MOVQ serviceLassoCreatorFileReadStorage_Extent(BX), CX
 CMPQ serviceLassoCreatorFileReadStorage_Completed(BX), CX
 JA creator_read_failed
 JB creator_read_next
creator_read_complete:
 MOVQ $1, serviceLassoCreatorFileReadStorage_Returned(BX)
 MOVQ $1, AX
 RET
creator_read_failed:
 MOVQ $1, serviceLassoCreatorFileReadStorage_Failure(BX)
 MOVQ $1, serviceLassoCreatorFileReadStorage_Returned(BX)
creator_read_refused:
 XORQ AX, AX
 RET
