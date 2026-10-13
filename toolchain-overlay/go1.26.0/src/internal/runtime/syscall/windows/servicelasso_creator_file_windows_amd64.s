//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Same actual snapshot phase R10 and row R11. Existing-file open only; no
// image/file equality, provider authority, containing reserve or read grant.
TEXT ·serviceLassoOpenCreatorImageFile<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE creator_file_before
 CMPQ R10, $1
 JNE creator_file_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), DX
 LEAQ ·serviceLassoCreatorNamesAfterOriginal(SB), R8
 LEAQ ·serviceLassoCreatorNamesAfter(SB), R9
 LEAQ ·serviceLassoCreatorPathsAfter(SB), R10
 LEAQ ·serviceLassoCreatorFilesAfterOriginal(SB), BX
 JMP creator_file_row
creator_file_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), DX
 LEAQ ·serviceLassoCreatorNamesBeforeOriginal(SB), R8
 LEAQ ·serviceLassoCreatorNamesBefore(SB), R9
 LEAQ ·serviceLassoCreatorPathsBefore(SB), R10
 LEAQ ·serviceLassoCreatorFilesBeforeOriginal(SB), BX
creator_file_row:
 CMPQ serviceLassoCreatorModuleStorage_Returned(DX), $1
 JNE creator_file_refused
 CMPQ serviceLassoCreatorModuleStorage_Failure(DX), $0
 JNE creator_file_refused
 MOVQ serviceLassoCreatorModuleStorage_Count(DX), CX
 MOVL $const_serviceLassoCreatorModuleCapacity, AX
 CMPQ CX, AX
 JA creator_file_refused
 CMPQ R11, CX
 JAE creator_file_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorNameExtent, AX
 JO creator_file_refused
 ADDQ AX, R8
 JC creator_file_refused
 CMPQ serviceLassoCreatorNameStorage_Returned(R8), $1
 JNE creator_file_refused
 CMPQ serviceLassoCreatorNameStorage_Failure(R8), $0
 JNE creator_file_refused
 MOVQ serviceLassoCreatorModuleStorage_Process(DX), CX
 CMPQ CX, serviceLassoCreatorNameStorage_Process(R8)
 JNE creator_file_refused
 MOVL $const_serviceLassoCreatorMappedNameUnits, AX
 SHLQ $1, AX
 IMULQ R11, AX
 JO creator_file_refused
 ADDQ AX, R9
 JC creator_file_refused
 CMPQ R9, serviceLassoCreatorNameStorage_Buffer(R8)
 JNE creator_file_refused
 MOVL serviceLassoCreatorNameStorage_Count(R8), CX
 TESTQ CX, CX
 JE creator_file_refused
 MOVL $const_serviceLassoCreatorMappedNameUnits, AX
 CMPQ CX, AX
 JAE creator_file_refused
 CMPW 0(R9), $92
 JNE creator_file_refused
 CMPW (R9)(CX*2), $0
 JNE creator_file_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorFileExtent, AX
 JO creator_file_refused
 ADDQ AX, BX
 JC creator_file_refused
 MOVQ $const_serviceLassoCreatorPathUnits, AX
 SHLQ $1, AX
 IMULQ R11, AX
 JO creator_file_refused
 ADDQ AX, R10
 JC creator_file_refused
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorFileExtent, AX
creator_file_owner_fresh:
 CMPQ 0(DX), $0
 JNE creator_file_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE creator_file_owner_fresh
 MOVQ R10, DX
 MOVQ $const_serviceLassoCreatorPathUnits, AX
creator_file_path_fresh:
 CMPW 0(DX), $0
 JNE creator_file_refused
 ADDQ $2, DX
 SUBQ $1, AX
 JNE creator_file_path_fresh
 MOVQ $1, serviceLassoCreatorFileStorage_Attempted(BX)
 MOVQ R9, serviceLassoCreatorFileStorage_NameBuffer(BX)
 MOVQ R10, serviceLassoCreatorFileStorage_PathBuffer(BX)
 MOVL CX, serviceLassoCreatorFileStorage_NameCount(BX)
 MOVQ serviceLassoCreatorNameStorage_Process(R8), DX
 MOVQ DX, serviceLassoCreatorFileStorage_Process(BX)
 MOVQ serviceLassoCreatorNameStorage_Address(R8), DX
 MOVQ DX, serviceLassoCreatorFileStorage_Address(BX)
 ADDQ $const_serviceLassoGlobalRootPrefixUnits+1, CX
 JC creator_file_failed
 MOVQ $const_serviceLassoCreatorPathUnits, DX
 CMPQ CX, DX
 JA creator_file_failed
 MOVQ CX, serviceLassoCreatorFileStorage_PathUnits(BX)
 // Exact UTF16 prefix, 14 units: two slashes, question mark, slash, GLOBALROOT.
 MOVW $92, 0(R10)
 MOVW $92, 2(R10)
 MOVW $63, 4(R10)
 MOVW $92, 6(R10)
 MOVW $71, 8(R10)
 MOVW $76, 10(R10)
 MOVW $79, 12(R10)
 MOVW $66, 14(R10)
 MOVW $65, 16(R10)
 MOVW $76, 18(R10)
 MOVW $82, 20(R10)
 MOVW $79, 22(R10)
 MOVW $79, 24(R10)
 MOVW $84, 26(R10)
 ADDQ $28, R10
 MOVL serviceLassoCreatorFileStorage_NameCount(BX), CX
 ADDQ $1, CX
creator_file_copy:
 MOVW 0(R9), DX
 MOVW DX, 0(R10)
 ADDQ $2, R9
 ADDQ $2, R10
 SUBQ $1, CX
 JNE creator_file_copy
 LEAQ serviceLassoCreatorFileStorage_Call(BX), AX
 MOVQ serviceLassoCreatorFileStorage_PathBuffer(BX), DX
 MOVQ DX, serviceLassoNativeCallStorage_Arguments(AX)
 // Explicit read-only rights; sharing read only; NULL means noninheriting.
 MOVQ $0x120081, serviceLassoNativeCallStorage_Arguments+8(AX)
 MOVQ $1, serviceLassoNativeCallStorage_Arguments+16(AX)
 MOVQ $0, serviceLassoNativeCallStorage_Arguments+24(AX)
 MOVQ $3, serviceLassoNativeCallStorage_Arguments+32(AX)
 MOVQ $128, serviceLassoNativeCallStorage_Arguments+40(AX)
 MOVQ $0, serviceLassoNativeCallStorage_Arguments+48(AX)
 MOVQ ·serviceLassoCreateImageFile(SB), DX
 MOVQ DX, StdCallInfo_Fn(AX)
 MOVQ $7, StdCallInfo_N(AX)
 CALL ·serviceLassoNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_file_failed
 MOVQ serviceLassoCreatorFileStorage_Call+StdCallInfo_R1(BX), CX
 MOVQ CX, serviceLassoCreatorFileStorage_File(BX)
 CMPQ CX, $-1
 JE creator_file_failed
 TESTQ CX, CX
 JE creator_file_failed
 MOVQ $1, serviceLassoCreatorFileStorage_Returned(BX)
 MOVQ $1, AX
 RET
creator_file_failed:
 MOVQ $1, serviceLassoCreatorFileStorage_Failure(BX)
 MOVQ $1, serviceLassoCreatorFileStorage_Returned(BX)
creator_file_refused:
 XORQ AX, AX
 RET
