//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// BX SAME native PE, DI fresh preowned output, R12 block offset/R13 entry
// index/R14 actual loaded base. Preserves BX, clobbers other native registers.
// Source caller must enroll/finance this exact output occurrence beforehand.
// AX1 is readonly one-row interpretation, never whole traversal or authority.
TEXT ·serviceLassoNativePERelocation<ABI0>(SB),NOSPLIT|NOFRAME,$0
 TESTQ BX, BX
 JE nativereloc_refused
 CMPQ serviceLassoCreatorNativePE_Attempted(BX), $1
 JNE nativereloc_refused
 CMPQ serviceLassoCreatorNativePE_Returned(BX), $1
 JNE nativereloc_refused
 CMPQ serviceLassoCreatorNativePE_Failure(BX), $0
 JNE nativereloc_refused
 TESTQ DI, DI
 JE nativereloc_refused
 TESTQ $7, DI
 JNE nativereloc_refused
 MOVQ DI, BP
 MOVQ DI, R15
 ADDQ $const_serviceLassoCreatorNativeRelocationExtent, R15
 JC nativereloc_refused
 XORQ R10, R10
nativereloc_exclusion:
 MOVQ BX, R8
 MOVQ $const_serviceLassoCreatorNativePEExtent, R9
 CMPQ R10, $0
 JE nativereloc_check_span
 MOVQ serviceLassoCreatorNativePE_Read(BX), R8
 MOVQ $const_serviceLassoCreatorFileReadExtent, R9
 CMPQ R10, $1
 JE nativereloc_check_span
 MOVQ serviceLassoCreatorNativePE_Metadata(BX), R8
 MOVQ $const_serviceLassoCreatorMetadataExtent, R9
 CMPQ R10, $2
 JE nativereloc_check_span
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), R8
 MOVQ serviceLassoCreatorNativePE_Extent(BX), R9
 CMPQ R10, $3
 JE nativereloc_check_span
 LEAQ ·serviceLassoCreatorFileReadCalls(SB), R8
 MOVQ $const_serviceLassoCreatorFileBytesCapacity, R9
 // Generator independently checks the full128*capacity native extent.
 SHLQ $7, R9
nativereloc_check_span:
 TESTQ R8, R8
 JE nativereloc_refused
 MOVQ R8, DX
 ADDQ R9, DX
 JC nativereloc_refused
 CMPQ BP, DX
 JAE nativereloc_next_span
 CMPQ R8, R15
 JB nativereloc_refused
nativereloc_next_span:
 INCQ R10
 CMPQ R10, $5
 JB nativereloc_exclusion
 MOVQ BP, DX
 MOVQ $const_serviceLassoCreatorNativeRelocationExtent, AX
nativereloc_fresh:
 CMPQ 0(DX), $0
 JNE nativereloc_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE nativereloc_fresh
 MOVQ $1, serviceLassoCreatorNativeRelocation_Attempted(BP)
 MOVQ BX, serviceLassoCreatorNativeRelocation_PE(BP)
 MOVQ R14, serviceLassoCreatorNativeRelocation_LoadedBase(BP)
 MOVQ R12, serviceLassoCreatorNativeRelocation_BlockOffset(BP)
 MOVQ R13, serviceLassoCreatorNativeRelocation_EntryIndex(BP)
 CMPQ serviceLassoCreatorNativePE_Directories(BX), $5
 JBE nativereloc_failed
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), SI
 MOVQ serviceLassoCreatorNativePE_Optional(BX), AX
 LEAQ 152(SI)(AX*1), DX
 MOVL 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeRelocation_DirectoryRVA(BP)
 MOVL 4(DX), CX
 MOVQ CX, serviceLassoCreatorNativeRelocation_DirectoryBytes(BP)
 CMPQ R12, CX
 JA nativereloc_failed
 SUBQ R12, CX
 CMPQ CX, $8
 JB nativereloc_failed
 ADDQ R12, AX
 JC nativereloc_failed
 MOVQ $8, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativereloc_failed
 MOVL 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeRelocation_PageRVA(BP)
 TESTQ $4095, AX
 JNE nativereloc_failed
 MOVL 4(DX), CX
 MOVQ CX, serviceLassoCreatorNativeRelocation_BlockBytes(BP)
 CMPQ CX, $8
 JB nativereloc_failed
 TESTQ $1, CX
 JNE nativereloc_failed
 MOVQ R12, AX
 ADDQ CX, AX
 JC nativereloc_failed
 CMPQ AX, serviceLassoCreatorNativeRelocation_DirectoryBytes(BP)
 JA nativereloc_failed
 MOVQ AX, serviceLassoCreatorNativeRelocation_NextBlock(BP)
 SUBQ $8, CX
 SHRQ $1, CX
 MOVQ CX, serviceLassoCreatorNativeRelocation_EntryCount(BP)
 TESTQ CX, CX
 JNE nativereloc_entry
 TESTQ R13, R13
 JNE nativereloc_failed
 JMP nativereloc_good
nativereloc_entry:
 CMPQ R13, CX
 JAE nativereloc_failed
 MOVQ R13, AX
 SHLQ $1, AX
 ADDQ $8, AX
 ADDQ R12, AX
 JC nativereloc_failed
 ADDQ serviceLassoCreatorNativeRelocation_DirectoryRVA(BP), AX
 JC nativereloc_failed
 MOVQ $2, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativereloc_failed
 MOVWQZX 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeRelocation_RawEntry(BP)
 MOVQ AX, CX
 SHRQ $12, CX
 MOVQ CX, serviceLassoCreatorNativeRelocation_Type(BP)
 ANDQ $4095, AX
 ADDQ serviceLassoCreatorNativeRelocation_PageRVA(BP), AX
 MOVQ AX, serviceLassoCreatorNativeRelocation_TargetRVA(BP)
 TESTQ CX, CX
 JE nativereloc_good
 CMPQ CX, $10
 JNE nativereloc_failed
 CMPQ serviceLassoCreatorNativePE_Machine(BX), $0x8664
 JNE nativereloc_failed
 MOVQ $8, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 MOVQ AX, serviceLassoCreatorNativeRelocation_OriginalKind(BP)
 CMPQ AX, $1
 JE nativereloc_raw
 CMPQ AX, $2
 JNE nativereloc_failed
 XORQ AX, AX
 JMP nativereloc_project
nativereloc_raw:
 MOVQ 0(DX), AX
nativereloc_project:
 MOVQ AX, serviceLassoCreatorNativeRelocation_Original(BP)
 MOVQ R14, CX
 SUBQ serviceLassoCreatorNativePE_ImageBase(BX), CX
 ADDQ CX, AX // deliberate uint64 modular projection, not checked address
 MOVQ AX, serviceLassoCreatorNativeRelocation_Expected(BP)
nativereloc_good:
 MOVQ $1, serviceLassoCreatorNativeRelocation_Returned(BP)
 MOVQ $1, AX
 RET
nativereloc_failed:
 MOVQ $1, serviceLassoCreatorNativeRelocation_Failure(BP)
 MOVQ $1, serviceLassoCreatorNativeRelocation_Returned(BP)
nativereloc_refused:
 XORQ AX, AX
 RET
