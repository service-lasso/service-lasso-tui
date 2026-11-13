//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// BX native PE/DI independently preowned output/R12 descriptor/R13 thunk.
// Private assembly-only convention preserves BX, clobbers other registers.
// Source caller must finance/enroll actual distinct output and all noalias
// relations. No raw pointer/name/output flag establishes provider authority.
TEXT ·serviceLassoNativePEImport<ABI0>(SB),NOSPLIT|NOFRAME,$0
 TESTQ BX, BX
 JE nativeimport_refused
 CMPQ serviceLassoCreatorNativePE_Attempted(BX), $1
 JNE nativeimport_refused
 CMPQ serviceLassoCreatorNativePE_Returned(BX), $1
 JNE nativeimport_refused
 CMPQ serviceLassoCreatorNativePE_Failure(BX), $0
 JNE nativeimport_refused
 TESTQ DI, DI
 JE nativeimport_refused
 TESTQ $7, DI
 JNE nativeimport_refused
 MOVQ DI, BP
 MOVQ DI, R15
 ADDQ $const_serviceLassoCreatorNativeImportExtent, R15
 JC nativeimport_refused
 XORQ R10, R10
nativeimport_exclusion:
 MOVQ BX, R8
 MOVQ $const_serviceLassoCreatorNativePEExtent, R9
 CMPQ R10, $0
 JE nativeimport_check_span
 MOVQ serviceLassoCreatorNativePE_Read(BX), R8
 MOVQ $const_serviceLassoCreatorFileReadExtent, R9
 CMPQ R10, $1
 JE nativeimport_check_span
 MOVQ serviceLassoCreatorNativePE_Metadata(BX), R8
 MOVQ $const_serviceLassoCreatorMetadataExtent, R9
 CMPQ R10, $2
 JE nativeimport_check_span
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), R8
 MOVQ serviceLassoCreatorNativePE_Extent(BX), R9
 CMPQ R10, $3
 JE nativeimport_check_span
 LEAQ ·serviceLassoCreatorFileReadCalls(SB), R8
 MOVQ $const_serviceLassoCreatorFileBytesCapacity, R9
 SHLQ $7, R9 // generator checks full128*capacity native extent
nativeimport_check_span:
 TESTQ R8, R8
 JE nativeimport_refused
 MOVQ R8, DX
 ADDQ R9, DX
 JC nativeimport_refused
 CMPQ BP, DX
 JAE nativeimport_next_span
 CMPQ R8, R15
 JB nativeimport_refused
nativeimport_next_span:
 INCQ R10
 CMPQ R10, $5
 JB nativeimport_exclusion
 MOVQ BP, DX
 MOVQ $const_serviceLassoCreatorNativeImportExtent, AX
nativeimport_fresh:
 CMPQ 0(DX), $0
 JNE nativeimport_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE nativeimport_fresh
 MOVQ $1, serviceLassoCreatorNativeImport_Attempted(BP)
 MOVQ BX, serviceLassoCreatorNativeImport_PE(BP)
 MOVQ R12, serviceLassoCreatorNativeImport_DescriptorIndex(BP)
 MOVQ R13, serviceLassoCreatorNativeImport_ThunkIndex(BP)
 CMPQ serviceLassoCreatorNativePE_Directories(BX), $1
 JBE nativeimport_failed
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), SI
 MOVQ serviceLassoCreatorNativePE_Optional(BX), AX
 LEAQ 120(SI)(AX*1), DX
 MOVL 0(DX), R14
 MOVL 4(DX), AX
 CMPQ AX, $20
 JB nativeimport_failed
 SUBQ $20, AX
 XORQ DX, DX
 MOVQ $20, CX
 DIVQ CX
 CMPQ R12, AX
 JA nativeimport_failed
 MOVQ R12, AX
 IMULQ $20, AX
 ADDQ R14, AX
 JC nativeimport_failed
 MOVQ $20, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeimport_failed
 MOVL 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeImport_OriginalFirstThunk(BP)
 MOVL 4(DX), CX
 MOVQ CX, serviceLassoCreatorNativeImport_Timestamp(BP)
 ORQ CX, AX
 MOVL 8(DX), CX
 MOVQ CX, serviceLassoCreatorNativeImport_ForwarderChain(BP)
 ORQ CX, AX
 MOVL 12(DX), CX
 MOVQ CX, serviceLassoCreatorNativeImport_NameRVA(BP)
 ORQ CX, AX
 MOVL 16(DX), CX
 MOVQ CX, serviceLassoCreatorNativeImport_FirstThunk(BP)
 ORQ CX, AX
 JNE nativeimport_module
 MOVQ $1, serviceLassoCreatorNativeImport_DescriptorEnd(BP)
 TESTQ R13, R13
 JNE nativeimport_failed
 JMP nativeimport_good
nativeimport_module:
 TESTQ CX, CX
 JE nativeimport_failed
 MOVQ serviceLassoCreatorNativeImport_NameRVA(BP), AX
 CALL ·serviceLassoNativePEName<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeimport_failed
 MOVQ DX, serviceLassoCreatorNativeImport_Module(BP)
 MOVQ CX, serviceLassoCreatorNativeImport_ModuleBytes(BP)
 MOVQ serviceLassoCreatorNativeImport_OriginalFirstThunk(BP), R14
 TESTQ R14, R14
 JNE nativeimport_source
 MOVQ serviceLassoCreatorNativeImport_FirstThunk(BP), R14
 MOVQ $1, serviceLassoCreatorNativeImport_UsesFirstThunk(BP)
nativeimport_source:
 MOVQ serviceLassoCreatorNativePE_ImageBytes(BX), AX
 CMPQ R14, AX
 JAE nativeimport_failed
 SUBQ R14, AX
 SHRQ $3, AX
 CMPQ R13, AX
 JA nativeimport_failed
 MOVQ R13, AX
 SHLQ $3, AX
 ADDQ R14, AX
 JC nativeimport_failed
 MOVQ AX, serviceLassoCreatorNativeImport_ThunkRVA(BP)
 MOVQ $8, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeimport_failed
 MOVQ 0(DX), R14
 MOVQ R14, serviceLassoCreatorNativeImport_RawThunk(BP)
 // Even a null source thunk has a corresponding loaded IAT slot. Retain
 // that locator before terminal success; the owning comparison consumes it.
 MOVQ serviceLassoCreatorNativeImport_FirstThunk(BP), CX
 MOVQ serviceLassoCreatorNativePE_ImageBytes(BX), AX
 CMPQ CX, AX
 JAE nativeimport_failed
 SUBQ CX, AX
 SHRQ $3, AX
 CMPQ R13, AX
 JA nativeimport_failed
 MOVQ R13, AX
 SHLQ $3, AX
 ADDQ CX, AX
 JC nativeimport_failed
 MOVQ AX, serviceLassoCreatorNativeImport_IATRVA(BP)
 ADDQ $8, AX
 JC nativeimport_failed
 CMPQ AX, serviceLassoCreatorNativePE_ImageBytes(BX)
 JA nativeimport_failed
 TESTQ R14, R14
 JNE nativeimport_nonterminal
 MOVQ $1, serviceLassoCreatorNativeImport_ThunkEnd(BP)
 JMP nativeimport_good
nativeimport_nonterminal:
 TESTQ R14, R14
 JS nativeimport_ordinal
 MOVL $0xffffffff, AX
 CMPQ R14, AX
 JA nativeimport_failed
 MOVQ R14, AX
 MOVQ $2, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeimport_failed
 MOVWQZX 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeImport_Hint(BP)
 MOVQ R14, AX
 ADDQ $2, AX
 CALL ·serviceLassoNativePEName<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeimport_failed
 MOVQ DX, serviceLassoCreatorNativeImport_Name(BP)
 MOVQ CX, serviceLassoCreatorNativeImport_NameBytes(BP)
 JMP nativeimport_good
nativeimport_ordinal:
 MOVQ $0x7fffffffffff0000, AX
 TESTQ R14, AX
 JNE nativeimport_failed
 MOVQ R14, AX
 ANDQ $65535, AX
 MOVQ AX, serviceLassoCreatorNativeImport_Ordinal(BP)
nativeimport_good:
 MOVQ $1, serviceLassoCreatorNativeImport_Returned(BP)
 MOVQ $1, AX
 RET
nativeimport_failed:
 MOVQ $1, serviceLassoCreatorNativeImport_Failure(BP)
 MOVQ $1, serviceLassoCreatorNativeImport_Returned(BP)
nativeimport_refused:
 XORQ AX, AX
 RET
