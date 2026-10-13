//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// BX native PE/DI preowned output/R12 selected index/R13 named mode0-or1.
// Private assembly-only convention preserves BX, clobbers other registers.
// Source caller must finance/enroll actual distinct output and all noalias
// relations. No raw pointer/name/output flag establishes provider authority.
TEXT ·serviceLassoNativePEExport<ABI0>(SB),NOSPLIT|NOFRAME,$0
 TESTQ BX, BX
 JE nativeexport_refused
 CMPQ serviceLassoCreatorNativePE_Attempted(BX), $1
 JNE nativeexport_refused
 CMPQ serviceLassoCreatorNativePE_Returned(BX), $1
 JNE nativeexport_refused
 CMPQ serviceLassoCreatorNativePE_Failure(BX), $0
 JNE nativeexport_refused
 TESTQ DI, DI
 JE nativeexport_refused
 TESTQ $7, DI
 JNE nativeexport_refused
 MOVQ DI, BP
 MOVQ DI, R15
 ADDQ $const_serviceLassoCreatorNativeExportExtent, R15
 JC nativeexport_refused
 XORQ R10, R10
nativeexport_exclusion:
 MOVQ BX, R8
 MOVQ $const_serviceLassoCreatorNativePEExtent, R9
 CMPQ R10, $0
 JE nativeexport_check_span
 MOVQ serviceLassoCreatorNativePE_Read(BX), R8
 MOVQ $const_serviceLassoCreatorFileReadExtent, R9
 CMPQ R10, $1
 JE nativeexport_check_span
 MOVQ serviceLassoCreatorNativePE_Metadata(BX), R8
 MOVQ $const_serviceLassoCreatorMetadataExtent, R9
 CMPQ R10, $2
 JE nativeexport_check_span
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), R8
 MOVQ serviceLassoCreatorNativePE_Extent(BX), R9
 CMPQ R10, $3
 JE nativeexport_check_span
 LEAQ ·serviceLassoCreatorFileReadCalls(SB), R8
 MOVQ $const_serviceLassoCreatorFileBytesCapacity, R9
 SHLQ $7, R9 // generator checks full128*capacity native extent
nativeexport_check_span:
 TESTQ R8, R8
 JE nativeexport_refused
 MOVQ R8, DX
 ADDQ R9, DX
 JC nativeexport_refused
 CMPQ BP, DX
 JAE nativeexport_next_span
 CMPQ R8, R15
 JB nativeexport_refused
nativeexport_next_span:
 INCQ R10
 CMPQ R10, $5
 JB nativeexport_exclusion
 MOVQ BP, DX
 MOVQ $const_serviceLassoCreatorNativeExportExtent, AX
nativeexport_fresh:
 CMPQ 0(DX), $0
 JNE nativeexport_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE nativeexport_fresh
 MOVQ $1, serviceLassoCreatorNativeExport_Attempted(BP)
 MOVQ BX, serviceLassoCreatorNativeExport_PE(BP)
 MOVQ R12, serviceLassoCreatorNativeExport_SelectedIndex(BP)
 MOVQ R13, serviceLassoCreatorNativeExport_Named(BP)
 CMPQ R13, $1
 JA nativeexport_failed
 CMPQ serviceLassoCreatorNativePE_Directories(BX), $0
 JE nativeexport_failed
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), SI
 MOVQ serviceLassoCreatorNativePE_Optional(BX), AX
 LEAQ 112(SI)(AX*1), DX
 MOVL 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_DirectoryRVA(BP)
 MOVL 4(DX), CX
 MOVQ CX, serviceLassoCreatorNativeExport_DirectoryBytes(BP)
 CMPQ CX, $40
 JB nativeexport_failed
 MOVQ $40, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeexport_failed
 MOVL 16(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_OrdinalBase(BP)
 MOVL 20(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_FunctionCount(BP)
 MOVL 24(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_NameCount(BP)
 MOVL 28(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_FunctionsRVA(BP)
 MOVL 32(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_NamesRVA(BP)
 MOVL 36(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_OrdinalsRVA(BP)
 // All source counts are DWORD, so these widened table products cannot wrap.
 MOVQ serviceLassoCreatorNativeExport_FunctionCount(BP), CX
 MOVQ serviceLassoCreatorNativeExport_FunctionsRVA(BP), AX
 TESTQ CX, CX
 JE nativeexport_function_span
 TESTQ AX, AX
 JE nativeexport_failed
nativeexport_function_span:
 SHLQ $2, CX
 CALL ·serviceLassoNativePEExportSpan<ABI0>(SB)
 TESTQ AX, AX
 JE nativeexport_failed
 MOVQ serviceLassoCreatorNativeExport_NameCount(BP), CX
 MOVQ serviceLassoCreatorNativeExport_NamesRVA(BP), AX
 TESTQ CX, CX
 JE nativeexport_name_span
 TESTQ AX, AX
 JE nativeexport_failed
nativeexport_name_span:
 SHLQ $2, CX
 CALL ·serviceLassoNativePEExportSpan<ABI0>(SB)
 TESTQ AX, AX
 JE nativeexport_failed
 MOVQ serviceLassoCreatorNativeExport_NameCount(BP), CX
 MOVQ serviceLassoCreatorNativeExport_OrdinalsRVA(BP), AX
 TESTQ CX, CX
 JE nativeexport_ordinal_span
 TESTQ AX, AX
 JE nativeexport_failed
nativeexport_ordinal_span:
 SHLQ $1, CX
 CALL ·serviceLassoNativePEExportSpan<ABI0>(SB)
 TESTQ AX, AX
 JE nativeexport_failed
 TESTQ R13, R13
 JE nativeexport_select_function
 CMPQ R12, serviceLassoCreatorNativeExport_NameCount(BP)
 JAE nativeexport_failed
 MOVQ R12, AX
 SHLQ $2, AX
 ADDQ serviceLassoCreatorNativeExport_NamesRVA(BP), AX
 MOVQ $4, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeexport_failed
 MOVL 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_NameRVA(BP)
 CALL ·serviceLassoNativePEName<ABI0>(SB)
 TESTQ AX, AX
 JE nativeexport_failed
 MOVQ DX, serviceLassoCreatorNativeExport_Name(BP)
 MOVQ CX, serviceLassoCreatorNativeExport_NameBytes(BP)
 MOVQ R12, AX
 SHLQ $1, AX
 ADDQ serviceLassoCreatorNativeExport_OrdinalsRVA(BP), AX
 MOVQ $2, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeexport_failed
 MOVWQZX 0(DX), R14
 JMP nativeexport_function
nativeexport_select_function:
 MOVQ R12, R14
nativeexport_function:
 MOVQ R14, serviceLassoCreatorNativeExport_FunctionIndex(BP)
 CMPQ R14, serviceLassoCreatorNativeExport_FunctionCount(BP)
 JAE nativeexport_failed
 MOVQ R14, AX
 ADDQ serviceLassoCreatorNativeExport_OrdinalBase(BP), AX
 MOVQ AX, serviceLassoCreatorNativeExport_Ordinal(BP)
 MOVQ R14, AX
 SHLQ $2, AX
 ADDQ serviceLassoCreatorNativeExport_FunctionsRVA(BP), AX
 MOVQ $4, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativeexport_failed
 MOVL 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeExport_FunctionRVA(BP)
 TESTQ AX, AX
 JNE nativeexport_nonhole
 MOVQ $1, serviceLassoCreatorNativeExport_Hole(BP)
 JMP nativeexport_good
nativeexport_nonhole:
 MOVQ serviceLassoCreatorNativeExport_DirectoryRVA(BP), CX
 CMPQ AX, CX
 JB nativeexport_direct
 MOVQ AX, R14
 SUBQ CX, R14
 CMPQ R14, serviceLassoCreatorNativeExport_DirectoryBytes(BP)
 JAE nativeexport_direct
 MOVQ $1, serviceLassoCreatorNativeExport_Forwarded(BP)
 CALL ·serviceLassoNativePEName<ABI0>(SB)
 TESTQ AX, AX
 JE nativeexport_failed
 MOVQ DX, serviceLassoCreatorNativeExport_Forwarder(BP)
 MOVQ CX, serviceLassoCreatorNativeExport_ForwarderBytes(BP)
 INCQ CX // NUL also must lie inside original export directory
 MOVQ serviceLassoCreatorNativeExport_DirectoryBytes(BP), AX
 SUBQ R14, AX
 CMPQ CX, AX
 JA nativeexport_failed
 JMP nativeexport_good
nativeexport_direct:
 MOVQ $1, CX
 CALL ·serviceLassoNativePEExportSpan<ABI0>(SB)
 TESTQ AX, AX
 JE nativeexport_failed
nativeexport_good:
 MOVQ $1, serviceLassoCreatorNativeExport_Returned(BP)
 MOVQ $1, AX
 RET
nativeexport_failed:
 MOVQ $1, serviceLassoCreatorNativeExport_Failure(BP)
 MOVQ $1, serviceLassoCreatorNativeExport_Returned(BP)
nativeexport_refused:
 XORQ AX, AX
 RET

// BX PE/AX RVA/CX extent; AX1 only for an image-contained representation.
// Preserves BX/BP/R12-R15, no pointer dereference or native/Go runtime call.
TEXT ·serviceLassoNativePEExportSpan<ABI0>(SB),NOSPLIT|NOFRAME,$0
 MOVQ serviceLassoCreatorNativePE_ImageBytes(BX), DX
 CMPQ AX, DX
 JA nativeexportspan_bad
 SUBQ AX, DX
 CMPQ CX, DX
 JA nativeexportspan_bad
 MOVQ $1, AX
 RET
nativeexportspan_bad:
 XORQ AX, AX
 RET
