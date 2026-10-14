//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Assembly-only R10 phase/R11 module index. BX retained native representation
// owner, SI original file bytes. Native nonvolatile registers are clobbered;
// the actual outer entry must save/restore them and its original OS stack.
// AX1 is representation only, never creator/provider/entry/read authority.
TEXT ·serviceLassoInterpretCreatorPE<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE nativepe_before
 CMPQ R10, $1
 JNE nativepe_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), DX
 LEAQ ·serviceLassoCreatorFileReadsAfterOriginal(SB), R8
 LEAQ ·serviceLassoCreatorMetadataAfterOriginal(SB), R9
 LEAQ ·serviceLassoCreatorNativePEAfterOriginal(SB), BX
 JMP nativepe_row
nativepe_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), DX
 LEAQ ·serviceLassoCreatorFileReadsBeforeOriginal(SB), R8
 LEAQ ·serviceLassoCreatorMetadataBeforeOriginal(SB), R9
 LEAQ ·serviceLassoCreatorNativePEBeforeOriginal(SB), BX
nativepe_row:
 CMPQ serviceLassoCreatorModuleStorage_Returned(DX), $1
 JNE nativepe_refused
 CMPQ serviceLassoCreatorModuleStorage_Failure(DX), $0
 JNE nativepe_refused
 MOVQ serviceLassoCreatorModuleStorage_Count(DX), CX
 MOVL $const_serviceLassoCreatorModuleCapacity, AX
 CMPQ CX, AX
 JA nativepe_refused
 CMPQ R11, CX
 JAE nativepe_refused
 MOVQ serviceLassoCreatorModuleStorage_Process(DX), BP
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorFileReadExtent, AX
 JO nativepe_refused
 ADDQ AX, R8
 JC nativepe_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorMetadataExtent, AX
 JO nativepe_refused
 ADDQ AX, R9
 JC nativepe_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorNativePEExtent, AX
 JO nativepe_refused
 ADDQ AX, BX
 JC nativepe_refused
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorNativePEExtent, AX
nativepe_fresh:
 CMPQ 0(DX), $0
 JNE nativepe_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE nativepe_fresh
 MOVQ $1, serviceLassoCreatorNativePE_Attempted(BX)
 MOVQ R10, serviceLassoCreatorNativePE_Phase(BX)
 MOVQ R11, serviceLassoCreatorNativePE_ModuleIndex(BX)
 MOVQ R8, serviceLassoCreatorNativePE_Read(BX)
 MOVQ R9, serviceLassoCreatorNativePE_Metadata(BX)
 CMPQ serviceLassoCreatorFileReadStorage_Attempted(R8), $1
 JNE nativepe_failed
 CMPQ serviceLassoCreatorFileReadStorage_Returned(R8), $1
 JNE nativepe_failed
 CMPQ serviceLassoCreatorFileReadStorage_Failure(R8), $0
 JNE nativepe_failed
 CMPQ serviceLassoCreatorFileReadStorage_Metadata(R8), R9
 JNE nativepe_failed
 CMPQ serviceLassoCreatorMetadataStorage_Attempted(R9), $1
 JNE nativepe_failed
 CMPQ serviceLassoCreatorMetadataStorage_Returned(R9), $1
 JNE nativepe_failed
 CMPQ serviceLassoCreatorMetadataStorage_Failure(R9), $0
 JNE nativepe_failed
 MOVQ serviceLassoCreatorMetadataStorage_File(R9), AX
 CMPQ AX, serviceLassoCreatorFileReadStorage_File(R8)
 JNE nativepe_failed
 MOVQ serviceLassoCreatorMetadataStorage_Process(R9), AX
 CMPQ AX, BP
 JNE nativepe_failed
 CMPQ AX, serviceLassoCreatorFileReadStorage_Process(R8)
 JNE nativepe_failed
 MOVQ serviceLassoCreatorMetadataStorage_Address(R9), AX
 CMPQ AX, serviceLassoCreatorFileReadStorage_Address(R8)
 JNE nativepe_failed
 MOVQ serviceLassoCreatorFileReadStorage_Extent(R8), CX
 MOVQ CX, serviceLassoCreatorNativePE_Extent(BX)
 CMPQ CX, $0
 JL nativepe_failed
 CMPQ CX, serviceLassoCreatorFileReadStorage_Completed(R8)
 JNE nativepe_failed
 CMPQ CX, serviceLassoCreatorMetadataStorage_Standard+serviceLassoFileStandardInformation_EndOfFile(R9)
 JNE nativepe_failed
 CMPQ CX, $64
 JB nativepe_failed
 MOVQ serviceLassoCreatorFileReadStorage_ArenaOffset(R8), AX
 MOVQ AX, DX
 ADDQ CX, DX
 JC nativepe_failed
 MOVQ $const_serviceLassoCreatorFileBytesCapacity, R9
 CMPQ DX, R9
 JA nativepe_failed
 LEAQ ·serviceLassoCreatorFileBytes(SB), SI
 ADDQ AX, SI
 JC nativepe_failed
 MOVQ SI, serviceLassoCreatorNativePE_Bytes(BX)
 MOVQ SI, DX
 ADDQ CX, DX
 JC nativepe_failed
 CMPW 0(SI), $0x5a4d
 JNE nativepe_failed
 MOVL 60(SI), AX
 TESTL AX, AX
 JS nativepe_failed
 CMPQ AX, $64
 JB nativepe_failed
 MOVQ AX, DX
 ADDQ $24, DX
 CMPQ DX, CX
 JA nativepe_failed
 LEAQ (SI)(AX*1), R8
 CMPL 0(R8), $0x4550
 JNE nativepe_failed
 MOVWQZX 4(R8), R9
 MOVQ R9, serviceLassoCreatorNativePE_Machine(BX)
 MOVWQZX 6(R8), R9
 MOVQ R9, serviceLassoCreatorNativePE_Sections(BX)
 TESTQ R9, R9
 JE nativepe_failed
 MOVWQZX 22(R8), R9
 MOVQ R9, serviceLassoCreatorNativePE_Characteristics(BX)
 MOVWQZX 20(R8), R9
 MOVQ R9, serviceLassoCreatorNativePE_OptionalBytes(BX)
 CMPQ R9, $112
 JB nativepe_failed
 MOVQ DX, serviceLassoCreatorNativePE_Optional(BX)
 ADDQ R9, DX
 JC nativepe_failed
 CMPQ DX, CX
 JA nativepe_failed
 MOVQ DX, serviceLassoCreatorNativePE_SectionTable(BX)
 ADDQ $24, R8
 CMPW 0(R8), $0x20b
 JNE nativepe_failed
 MOVL 16(R8), AX
 MOVQ AX, serviceLassoCreatorNativePE_Entry(BX)
 MOVQ 24(R8), AX
 MOVQ AX, serviceLassoCreatorNativePE_ImageBase(BX)
 MOVL 32(R8), AX
 MOVQ AX, serviceLassoCreatorNativePE_SectionAlignment(BX)
 TESTQ AX, AX
 JE nativepe_failed
 LEAQ -1(AX), DX
 TESTQ AX, DX
 JNE nativepe_failed
 MOVL 36(R8), R10
 MOVQ R10, serviceLassoCreatorNativePE_FileAlignment(BX)
 TESTQ R10, R10
 JE nativepe_failed
 LEAQ -1(R10), DX
 TESTQ R10, DX
 JNE nativepe_failed
 CMPQ AX, R10
 JB nativepe_failed
 CMPQ AX, $4096
 JAE nativepe_page_alignment
 CMPQ AX, R10
 JNE nativepe_failed
 JMP nativepe_alignment_done
nativepe_page_alignment:
 CMPQ R10, $512
 JB nativepe_failed
 CMPQ R10, $65536
 JA nativepe_failed
nativepe_alignment_done:
 MOVL 56(R8), R11
 MOVQ R11, serviceLassoCreatorNativePE_ImageBytes(BX)
 TESTQ R11, R11
 JE nativepe_failed
 MOVQ serviceLassoCreatorNativePE_ImageBase(BX), DX
 ADDQ R11, DX
 JC nativepe_failed
 LEAQ -1(AX), DX
 TESTQ R11, DX
 JNE nativepe_failed
 MOVQ serviceLassoCreatorNativePE_Entry(BX), DX
 CMPQ DX, R11
 JAE nativepe_failed
 MOVL 60(R8), DX
 MOVQ DX, serviceLassoCreatorNativePE_HeaderBytes(BX)
 TESTQ DX, DX
 JE nativepe_failed
 CMPQ DX, CX
 JA nativepe_failed
 CMPQ DX, R11
 JA nativepe_failed
 LEAQ -1(R10), AX
 TESTQ DX, AX
 JNE nativepe_failed
 MOVQ serviceLassoCreatorNativePE_SectionTable(BX), AX
 MOVQ serviceLassoCreatorNativePE_Sections(BX), R10
 IMULQ $40, R10
 ADDQ R10, AX
 JC nativepe_failed
 CMPQ AX, DX
 JA nativepe_failed
 MOVL 108(R8), AX
 MOVQ AX, serviceLassoCreatorNativePE_Directories(BX)
 MOVQ serviceLassoCreatorNativePE_OptionalBytes(BX), DX
 SUBQ $112, DX
 SHRQ $3, DX
 CMPQ AX, DX
 JA nativepe_failed
 // Complete section and directory geometry is independently consumed below.
 CALL ·serviceLassoNativePESections<ABI0>(SB)
 CMPQ AX, $1
 JNE nativepe_failed
 CALL ·serviceLassoNativePEDirectories<ABI0>(SB)
 CMPQ AX, $1
 JNE nativepe_failed
 MOVQ $1, serviceLassoCreatorNativePE_Returned(BX)
 MOVQ $1, AX
 RET
nativepe_failed:
 MOVQ $1, serviceLassoCreatorNativePE_Failure(BX)
 MOVQ $1, serviceLassoCreatorNativePE_Returned(BX)
nativepe_refused:
 XORQ AX, AX
 RET

// BX/SI preserved. Scratch registers and OS-native nonvolatile registers are
// not assumed transparent across this private assembly-only convention.
TEXT ·serviceLassoNativePEDirectories<ABI0>(SB),NOSPLIT|NOFRAME,$0
 XORQ CX, CX
 MOVQ serviceLassoCreatorNativePE_Optional(BX), DX
 LEAQ 112(SI)(DX*1), R8
nativepe_directory_loop:
 CMPQ CX, serviceLassoCreatorNativePE_Directories(BX)
 JAE nativepe_directory_good
 MOVL 0(R8), AX
 MOVL 4(R8), DX
 TESTQ DX, DX
 JE nativepe_directory_next
 TESTQ AX, AX
 JE nativepe_directory_bad
 ADDQ AX, DX
 JC nativepe_directory_bad
 MOVQ serviceLassoCreatorNativePE_ImageBytes(BX), R9
 CMPQ CX, $4
 JNE nativepe_directory_limit
 MOVQ serviceLassoCreatorNativePE_Extent(BX), R9
nativepe_directory_limit:
 CMPQ DX, R9
 JA nativepe_directory_bad
nativepe_directory_next:
 ADDQ $8, R8
 INCQ CX
 JMP nativepe_directory_loop
nativepe_directory_good:
 MOVQ $1, AX
 RET
nativepe_directory_bad:
 XORQ AX, AX
 RET

TEXT ·serviceLassoNativePESections<ABI0>(SB),NOSPLIT|NOFRAME,$0
 MOVQ serviceLassoCreatorNativePE_SectionAlignment(BX), DX
 DECQ DX
 MOVQ serviceLassoCreatorNativePE_HeaderBytes(BX), BP
 ADDQ DX, BP
 JC nativepe_section_bad
 NOTQ DX
 ANDQ DX, BP
 CMPQ BP, serviceLassoCreatorNativePE_ImageBytes(BX)
 JA nativepe_section_bad
 XORQ DI, DI
nativepe_section_loop:
 CMPQ DI, serviceLassoCreatorNativePE_Sections(BX)
 JAE nativepe_section_good
 MOVQ DI, R8
 IMULQ $40, R8
 ADDQ serviceLassoCreatorNativePE_SectionTable(BX), R8
 ADDQ SI, R8
 MOVL 12(R8), R12
 MOVL 20(R8), R14
 MOVL 16(R8), R15
 MOVL 8(R8), R13
 CMPQ R13, R15
 JAE nativepe_section_max
 MOVQ R15, R13
nativepe_section_max:
 MOVQ serviceLassoCreatorNativePE_SectionAlignment(BX), DX
 DECQ DX
 TESTQ R12, DX
 JNE nativepe_section_bad
 ADDQ DX, R13
 JC nativepe_section_bad
 NOTQ DX
 ANDQ DX, R13
 MOVQ R12, R11
 ADDQ R13, R11
 JC nativepe_section_bad
 CMPQ R11, serviceLassoCreatorNativePE_ImageBytes(BX)
 JA nativepe_section_bad
 TESTQ R13, R13
 JE nativepe_section_raw
 CMPQ R12, BP
 JB nativepe_section_bad
nativepe_section_raw:
 TESTQ R15, R15
 JE nativepe_section_previous
 CMPQ R14, serviceLassoCreatorNativePE_HeaderBytes(BX)
 JB nativepe_section_bad
 MOVQ serviceLassoCreatorNativePE_FileAlignment(BX), DX
 DECQ DX
 TESTQ R14, DX
 JNE nativepe_section_bad
 TESTQ R15, DX
 JNE nativepe_section_bad
 MOVQ R14, AX
 ADDQ R15, AX
 JC nativepe_section_bad
 CMPQ AX, serviceLassoCreatorNativePE_Extent(BX)
 JA nativepe_section_bad
nativepe_section_previous:
 XORQ CX, CX
nativepe_previous_loop:
 CMPQ CX, DI
 JAE nativepe_section_next
 MOVQ CX, DX
 IMULQ $40, DX
 ADDQ serviceLassoCreatorNativePE_SectionTable(BX), DX
 ADDQ SI, DX
 MOVL 8(DX), R10
 MOVL 16(DX), R9
 CMPQ R10, R9
 JAE nativepe_previous_max
 MOVQ R9, R10
nativepe_previous_max:
 MOVQ serviceLassoCreatorNativePE_SectionAlignment(BX), R9
 DECQ R9
 ADDQ R9, R10
 JC nativepe_section_bad
 NOTQ R9
 ANDQ R9, R10
 TESTQ R13, R13
 JE nativepe_previous_raw
 TESTQ R10, R10
 JE nativepe_previous_raw
 MOVL 12(DX), AX
 ADDQ AX, R10
 JC nativepe_section_bad
 CMPQ R12, R10
 JAE nativepe_previous_raw
 CMPQ AX, R11
 JB nativepe_section_bad
nativepe_previous_raw:
 TESTQ R15, R15
 JE nativepe_previous_next
 MOVL 16(DX), R9
 TESTQ R9, R9
 JE nativepe_previous_next
 MOVL 20(DX), AX
 ADDQ AX, R9
 JC nativepe_section_bad
 CMPQ R14, R9
 JAE nativepe_previous_next
 MOVQ R14, R10
 ADDQ R15, R10
 JC nativepe_section_bad
 CMPQ AX, R10
 JB nativepe_section_bad
nativepe_previous_next:
 INCQ CX
 JMP nativepe_previous_loop
nativepe_section_next:
 INCQ DI
 JMP nativepe_section_loop
nativepe_section_good:
 MOVQ $1, AX
 RET
nativepe_section_bad:
 XORQ AX, AX
 RET

// BX exact completed native PE; AX RVA/CX size. Returns AX kind0 refusal,
// kind1 SAME original file address DX, kind2 virtualzero DX0. Preserves BX,
// clobbers DX/SI/DI/R8/R9/R10/R11. No byte copying, native calls or Go G/TLS.
TEXT ·serviceLassoNativePERVA<ABI0>(SB),NOSPLIT|NOFRAME,$0
 XORQ DX, DX
 TESTQ BX, BX
 JE nativepe_rva_bad
 CMPQ serviceLassoCreatorNativePE_Attempted(BX), $1
 JNE nativepe_rva_bad
 CMPQ serviceLassoCreatorNativePE_Returned(BX), $1
 JNE nativepe_rva_bad
 CMPQ serviceLassoCreatorNativePE_Failure(BX), $0
 JNE nativepe_rva_bad
 TESTQ CX, CX
 JE nativepe_rva_bad
 MOVQ AX, R8
 ADDQ CX, R8
 JC nativepe_rva_bad
 CMPQ R8, serviceLassoCreatorNativePE_ImageBytes(BX)
 JA nativepe_rva_bad
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), SI
 CMPQ AX, serviceLassoCreatorNativePE_HeaderBytes(BX)
 JAE nativepe_rva_sections
 CMPQ R8, serviceLassoCreatorNativePE_HeaderBytes(BX)
 JA nativepe_rva_bad
 JMP nativepe_rva_file
nativepe_rva_sections:
 XORQ DI, DI
nativepe_rva_section_loop:
 CMPQ DI, serviceLassoCreatorNativePE_Sections(BX)
 JAE nativepe_rva_bad
 MOVQ DI, R9
 IMULQ $40, R9
 ADDQ serviceLassoCreatorNativePE_SectionTable(BX), R9
 ADDQ SI, R9
 MOVL 12(R9), R10
 CMPQ AX, R10
 JB nativepe_rva_next
 MOVQ AX, R11
 SUBQ R10, R11
 MOVL 16(R9), R10
 CMPQ R11, R10
 JAE nativepe_rva_virtual
 MOVQ R11, R8
 ADDQ CX, R8
 JC nativepe_rva_bad
 CMPQ R8, R10
 JA nativepe_rva_bad
 MOVL 20(R9), AX
 ADDQ R11, AX
 JC nativepe_rva_bad
 JMP nativepe_rva_file
nativepe_rva_virtual:
 MOVL 8(R9), R10
 CMPQ R11, R10
 JAE nativepe_rva_next
 ADDQ CX, R11
 JC nativepe_rva_bad
 CMPQ R11, R10
 JA nativepe_rva_bad
 XORQ DX, DX
 MOVQ $2, AX
 RET
nativepe_rva_next:
 INCQ DI
 JMP nativepe_rva_section_loop
nativepe_rva_file:
 MOVQ AX, R8
 ADDQ CX, R8
 JC nativepe_rva_bad
 CMPQ R8, serviceLassoCreatorNativePE_Extent(BX)
 JA nativepe_rva_bad
 MOVQ SI, DX
 ADDQ AX, DX
 JC nativepe_rva_bad
 MOVQ DX, R8
 ADDQ CX, R8
 JC nativepe_rva_bad
 MOVQ $1, AX
 RET
nativepe_rva_bad:
 XORQ DX, DX
 XORQ AX, AX
 RET

// BX SAME native PE/AX nameRVA -> AX1 DX originalpointer/CX byte count before
// checked NUL, otherwise AX0/DX0/CX0. Preserves BX/BP/R12-R15, clobbers others.
// Names are original locators only, never provider/source authentication.
TEXT ·serviceLassoNativePEName<ABI0>(SB),NOSPLIT|NOFRAME,$0
 TESTQ BX, BX
 JE nativepe_name_bad
 CMPQ serviceLassoCreatorNativePE_Attempted(BX), $1
 JNE nativepe_name_bad
 CMPQ serviceLassoCreatorNativePE_Returned(BX), $1
 JNE nativepe_name_bad
 CMPQ serviceLassoCreatorNativePE_Failure(BX), $0
 JNE nativepe_name_bad
 TESTQ AX, AX
 JE nativepe_name_bad
 MOVQ serviceLassoCreatorNativePE_Bytes(BX), SI
 CMPQ AX, serviceLassoCreatorNativePE_HeaderBytes(BX)
 JAE nativepe_name_sections
 MOVQ serviceLassoCreatorNativePE_HeaderBytes(BX), R11
 SUBQ AX, R11
 JMP nativepe_name_file
nativepe_name_sections:
 XORQ DI, DI
nativepe_name_section_loop:
 CMPQ DI, serviceLassoCreatorNativePE_Sections(BX)
 JAE nativepe_name_bad
 MOVQ DI, R8
 IMULQ $40, R8
 ADDQ serviceLassoCreatorNativePE_SectionTable(BX), R8
 ADDQ SI, R8
 MOVL 12(R8), R9
 CMPQ AX, R9
 JB nativepe_name_next
 MOVQ AX, R10
 SUBQ R9, R10
 MOVL 16(R8), R11
 CMPQ R10, R11
 JAE nativepe_name_next
 SUBQ R10, R11
 MOVL 20(R8), AX
 ADDQ R10, AX
 JC nativepe_name_bad
 JMP nativepe_name_file
nativepe_name_next:
 INCQ DI
 JMP nativepe_name_section_loop
nativepe_name_file:
 TESTQ R11, R11
 JE nativepe_name_bad
 MOVQ AX, R8
 ADDQ R11, R8
 JC nativepe_name_bad
 CMPQ R8, serviceLassoCreatorNativePE_Extent(BX)
 JA nativepe_name_bad
 MOVQ SI, DX
 ADDQ AX, DX
 JC nativepe_name_bad
 MOVQ DX, R8
 ADDQ R11, R8
 JC nativepe_name_bad
 XORQ CX, CX
nativepe_name_scan:
 CMPQ CX, R11
 JAE nativepe_name_bad
 CMPB (DX)(CX*1), $0
 JE nativepe_name_end
 INCQ CX
 JMP nativepe_name_scan
nativepe_name_end:
 TESTQ CX, CX
 JE nativepe_name_bad
 MOVQ $1, AX
 RET
nativepe_name_bad:
 XORQ CX, CX
 XORQ DX, DX
 XORQ AX, AX
 RET
