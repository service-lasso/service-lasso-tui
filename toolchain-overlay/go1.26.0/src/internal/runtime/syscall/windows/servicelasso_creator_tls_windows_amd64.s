//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// BX native PE/DI independently preowned output/R12 callback index.
// Private assembly-only convention preserves BX, clobbers other registers.
// Source caller must finance/enroll actual distinct output and all noalias
// relations. No raw pointer/name/output flag establishes provider authority.
TEXT ·serviceLassoNativePETLS<ABI0>(SB),NOSPLIT|NOFRAME,$0
 TESTQ BX, BX
 JE nativetls_refused
 CMPQ serviceLassoCreatorNativePE_Attempted(BX), $1
 JNE nativetls_refused
 CMPQ serviceLassoCreatorNativePE_Returned(BX), $1
 JNE nativetls_refused
 CMPQ serviceLassoCreatorNativePE_Failure(BX), $0
 JNE nativetls_refused
 TESTQ DI, DI
 JE nativetls_refused
 TESTQ $7, DI
 JNE nativetls_refused
 MOVQ DI, BP
 MOVQ DI, R15
 ADDQ $const_serviceLassoCreatorNativeTLSExtent, R15
 JC nativetls_refused
 XORQ R10, R10
nativetls_exclusion:
 MOVQ BX, R8
 MOVQ $const_serviceLassoCreatorNativePEExtent, R9
 CMPQ R10, $0
 JE nativetls_check_span
 MOVQ serviceLassoCreatorNativePE_Read(BX), R8
 MOVQ $const_serviceLassoCreatorFileReadExtent, R9
 CMPQ R10, $1
 JE nativetls_check_span
 MOVQ serviceLassoCreatorNativePE_Metadata(BX), R8
 MOVQ $const_serviceLassoCreatorMetadataExtent, R9
 CMPQ R10, $2
 JE nativetls_check_span
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), R8
 MOVQ serviceLassoCreatorNativePE_Extent(BX), R9
 CMPQ R10, $3
 JE nativetls_check_span
 LEAQ ·serviceLassoCreatorFileReadCalls(SB), R8
 MOVQ $const_serviceLassoCreatorFileBytesCapacity, R9
 SHLQ $7, R9 // generator checks full128*capacity native extent
nativetls_check_span:
 TESTQ R8, R8
 JE nativetls_refused
 MOVQ R8, DX
 ADDQ R9, DX
 JC nativetls_refused
 CMPQ BP, DX
 JAE nativetls_next_span
 CMPQ R8, R15
 JB nativetls_refused
nativetls_next_span:
 INCQ R10
 CMPQ R10, $5
 JB nativetls_exclusion
 MOVQ BP, DX
 MOVQ $const_serviceLassoCreatorNativeTLSExtent, AX
nativetls_fresh:
 CMPQ 0(DX), $0
 JNE nativetls_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE nativetls_fresh
 MOVQ $1, serviceLassoCreatorNativeTLS_Attempted(BP)
 MOVQ BX, serviceLassoCreatorNativeTLS_PE(BP)
 MOVQ R12, serviceLassoCreatorNativeTLS_CallbackIndex(BP)
 CMPQ serviceLassoCreatorNativePE_Directories(BX), $9
 JBE nativetls_failed
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), SI
 MOVQ serviceLassoCreatorNativePE_Optional(BX), AX
 LEAQ 184(SI)(AX*1), DX
 MOVL 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeTLS_DirectoryRVA(BP)
 MOVL 4(DX), CX
 MOVQ CX, serviceLassoCreatorNativeTLS_DirectoryBytes(BP)
 CMPQ CX, $40
 JB nativetls_failed
 MOVQ $40, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativetls_failed
 MOVQ 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeTLS_RawStartVA(BP)
 MOVQ 8(DX), CX
 MOVQ CX, serviceLassoCreatorNativeTLS_RawEndVA(BP)
 MOVQ 16(DX), R8
 MOVQ R8, serviceLassoCreatorNativeTLS_IndexVA(BP)
 MOVQ 24(DX), R8
 MOVQ R8, serviceLassoCreatorNativeTLS_CallbacksVA(BP)
 MOVL 32(DX), R8
 MOVQ R8, serviceLassoCreatorNativeTLS_ZeroFill(BP)
 MOVL 36(DX), R8
 MOVQ R8, serviceLassoCreatorNativeTLS_Characteristics(BP)
 MOVQ AX, R8
 ORQ CX, R8
 JE nativetls_index
 CMPQ CX, AX
 JB nativetls_failed
 SUBQ AX, CX
 MOVQ CX, serviceLassoCreatorNativeTLS_TemplateBytes(BP)
 CALL ·serviceLassoNativePETLSVA<ABI0>(SB)
 TESTQ AX, AX
 JE nativetls_failed
 MOVQ DX, serviceLassoCreatorNativeTLS_TemplateRVA(BP)
 MOVQ serviceLassoCreatorNativeTLS_TemplateBytes(BP), CX
 TESTQ CX, CX
 JE nativetls_index
 MOVQ DX, AX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativetls_failed
 MOVQ DX, serviceLassoCreatorNativeTLS_Template(BP)
nativetls_index:
 MOVQ serviceLassoCreatorNativeTLS_IndexVA(BP), AX
 TESTQ AX, AX
 JE nativetls_callbacks
 MOVQ $4, CX
 CALL ·serviceLassoNativePETLSVA<ABI0>(SB)
 TESTQ AX, AX
 JE nativetls_failed
 MOVQ DX, serviceLassoCreatorNativeTLS_IndexRVA(BP)
 MOVQ DX, AX
 MOVQ $4, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 TESTQ AX, AX
 JE nativetls_failed
 MOVQ AX, serviceLassoCreatorNativeTLS_IndexKind(BP)
nativetls_callbacks:
 MOVQ serviceLassoCreatorNativeTLS_CallbacksVA(BP), AX
 TESTQ AX, AX
 JNE nativetls_array
 TESTQ R12, R12
 JNE nativetls_failed
 JMP nativetls_end
nativetls_array:
 MOVQ $8, CX
 CALL ·serviceLassoNativePETLSVA<ABI0>(SB)
 TESTQ AX, AX
 JE nativetls_failed
 MOVQ DX, serviceLassoCreatorNativeTLS_CallbackArrayRVA(BP)
 MOVQ serviceLassoCreatorNativePE_ImageBytes(BX), AX
 SUBQ DX, AX
 SHRQ $3, AX
 CMPQ R12, AX
 JA nativetls_failed
 MOVQ R12, AX
 SHLQ $3, AX
 ADDQ DX, AX
 JC nativetls_failed
 MOVQ AX, serviceLassoCreatorNativeTLS_CallbackSlotRVA(BP)
 MOVQ $8, CX
 CALL ·serviceLassoNativePERVA<ABI0>(SB)
 CMPQ AX, $1
 JNE nativetls_failed
 MOVQ 0(DX), AX
 MOVQ AX, serviceLassoCreatorNativeTLS_CallbackVA(BP)
 TESTQ AX, AX
 JE nativetls_end
 MOVQ $1, CX
 CALL ·serviceLassoNativePETLSVA<ABI0>(SB)
 TESTQ AX, AX
 JE nativetls_good // external raw callback retained, never admitted/called
 MOVQ DX, serviceLassoCreatorNativeTLS_CallbackRVA(BP)
 MOVQ $1, serviceLassoCreatorNativeTLS_CallbackInImage(BP)
 JMP nativetls_good
nativetls_end:
 MOVQ $1, serviceLassoCreatorNativeTLS_CallbackEnd(BP)
nativetls_good:
 MOVQ $1, serviceLassoCreatorNativeTLS_Returned(BP)
 MOVQ $1, AX
 RET
nativetls_failed:
 MOVQ $1, serviceLassoCreatorNativeTLS_Failure(BP)
 MOVQ $1, serviceLassoCreatorNativeTLS_Returned(BP)
nativetls_refused:
 XORQ AX, AX
 RET

// Preferred-image VA representation conversion only. BX PE/AX VA/CX size;
// AX1/DX RVA or AX0. Preserves BX/BP/R12-R15; no native or Go runtime calls.
TEXT ·serviceLassoNativePETLSVA<ABI0>(SB),NOSPLIT|NOFRAME,$0
 MOVQ serviceLassoCreatorNativePE_ImageBase(BX), DX
 CMPQ AX, DX
 JB nativetlsva_bad
 SUBQ DX, AX
 MOVQ serviceLassoCreatorNativePE_ImageBytes(BX), DX
 CMPQ AX, DX
 JA nativetlsva_bad
 SUBQ AX, DX
 CMPQ CX, DX
 JA nativetlsva_bad
 MOVQ AX, DX
 MOVQ $1, AX
 RET
nativetlsva_bad:
 XORQ AX, AX
 RET
