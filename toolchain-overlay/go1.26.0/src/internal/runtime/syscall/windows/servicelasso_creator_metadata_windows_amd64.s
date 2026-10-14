//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Phase R10/index R11 consume the exact acquired file row. Fixed metadata is
// observation only: size/ID/times do not finance reads or authenticate images.
TEXT ·serviceLassoObserveCreatorFileMetadata<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE creator_metadata_before
 CMPQ R10, $1
 JNE creator_metadata_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), DX
 LEAQ ·serviceLassoCreatorFilesAfterOriginal(SB), R8
 LEAQ ·serviceLassoCreatorMetadataAfterOriginal(SB), BX
 JMP creator_metadata_row
creator_metadata_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), DX
 LEAQ ·serviceLassoCreatorFilesBeforeOriginal(SB), R8
 LEAQ ·serviceLassoCreatorMetadataBeforeOriginal(SB), BX
creator_metadata_row:
 CMPQ serviceLassoCreatorModuleStorage_Returned(DX), $1
 JNE creator_metadata_refused
 CMPQ serviceLassoCreatorModuleStorage_Failure(DX), $0
 JNE creator_metadata_refused
 MOVQ serviceLassoCreatorModuleStorage_Count(DX), CX
 MOVL $const_serviceLassoCreatorModuleCapacity, AX
 CMPQ CX, AX
 JA creator_metadata_refused
 CMPQ R11, CX
 JAE creator_metadata_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorFileExtent, AX
 JO creator_metadata_refused
 ADDQ AX, R8
 JC creator_metadata_refused
 CMPQ serviceLassoCreatorFileStorage_Returned(R8), $1
 JNE creator_metadata_refused
 CMPQ serviceLassoCreatorFileStorage_Failure(R8), $0
 JNE creator_metadata_refused
 CMPQ serviceLassoCreatorFileStorage_Call+serviceLassoNativeCallStorage_Returned(R8), $1
 JNE creator_metadata_refused
 MOVQ serviceLassoCreatorFileStorage_File(R8), CX
 TESTQ CX, CX
 JE creator_metadata_refused
 CMPQ CX, $-1
 JE creator_metadata_refused
 CMPQ CX, serviceLassoCreatorFileStorage_Call+StdCallInfo_R1(R8)
 JNE creator_metadata_refused
 MOVQ serviceLassoCreatorModuleStorage_Process(DX), CX
 CMPQ CX, serviceLassoCreatorFileStorage_Process(R8)
 JNE creator_metadata_refused
 MOVQ R8, R10
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorMetadataExtent, AX
 JO creator_metadata_refused
 ADDQ AX, BX
 JC creator_metadata_refused
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorMetadataExtent, AX
creator_metadata_fresh:
 CMPQ 0(DX), $0
 JNE creator_metadata_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE creator_metadata_fresh
 MOVQ $1, serviceLassoCreatorMetadataStorage_Attempted(BX)
 MOVQ serviceLassoCreatorFileStorage_File(R10), CX
 MOVQ CX, serviceLassoCreatorMetadataStorage_File(BX)
 MOVQ serviceLassoCreatorFileStorage_Process(R10), DX
 MOVQ DX, serviceLassoCreatorMetadataStorage_Process(BX)
 MOVQ serviceLassoCreatorFileStorage_Address(R10), DX
 MOVQ DX, serviceLassoCreatorMetadataStorage_Address(BX)
 LEAQ serviceLassoCreatorMetadataStorage_BasicCall(BX), AX
 MOVQ $0, DX
 LEAQ serviceLassoCreatorMetadataStorage_Basic(BX), R8
 MOVQ $40, R9
 CALL ·serviceLassoQueryCreatorFileMetadata<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_metadata_failed
 MOVQ serviceLassoCreatorMetadataStorage_BasicCall+StdCallInfo_R1(BX), AX
 CALL ·serviceLassoNativeBOOL<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_metadata_failed
 LEAQ serviceLassoCreatorMetadataStorage_StandardCall(BX), AX
 MOVQ serviceLassoCreatorMetadataStorage_File(BX), CX
 MOVQ $1, DX
 LEAQ serviceLassoCreatorMetadataStorage_Standard(BX), R8
 MOVQ $24, R9
 CALL ·serviceLassoQueryCreatorFileMetadata<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_metadata_failed
 MOVQ serviceLassoCreatorMetadataStorage_StandardCall+StdCallInfo_R1(BX), AX
 CALL ·serviceLassoNativeBOOL<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_metadata_failed
 LEAQ serviceLassoCreatorMetadataStorage_IDCall(BX), AX
 MOVQ serviceLassoCreatorMetadataStorage_File(BX), CX
 MOVQ $18, DX
 LEAQ serviceLassoCreatorMetadataStorage_ID(BX), R8
 MOVQ $24, R9
 CALL ·serviceLassoQueryCreatorFileMetadata<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_metadata_failed
 MOVQ serviceLassoCreatorMetadataStorage_IDCall+StdCallInfo_R1(BX), AX
 CALL ·serviceLassoNativeBOOL<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_metadata_failed
 MOVQ $1, serviceLassoCreatorMetadataStorage_Returned(BX)
 MOVQ $1, AX
 RET
creator_metadata_failed:
 MOVQ $1, serviceLassoCreatorMetadataStorage_Failure(BX)
 MOVQ $1, serviceLassoCreatorMetadataStorage_Returned(BX)
creator_metadata_refused:
 XORQ AX, AX
 RET

// Dedicated fixed four-argument native call seam. Only its complete fresh
// parent calls it; native results stay in that exact caller-owned occurrence.
// BX is preserved by the inspected NativeCall/StdCall/native ABI chain.
TEXT ·serviceLassoQueryCreatorFileMetadata<ABI0>(SB),NOSPLIT|NOFRAME,$0
 MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
 MOVQ DX, serviceLassoNativeCallStorage_Arguments+8(AX)
 MOVQ R8, serviceLassoNativeCallStorage_Arguments+16(AX)
 MOVQ R9, serviceLassoNativeCallStorage_Arguments+24(AX)
 MOVQ ·serviceLassoGetImageFileInformation(SB), DX
 MOVQ DX, StdCallInfo_Fn(AX)
 MOVQ $4, StdCallInfo_N(AX)
 CALL ·serviceLassoNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_metadata_call_refused
 // NativeCall returns transition status only. Parent independently projects
 // raw BOOL from its exact retained Basic/Standard/ID call occurrence.
 RET
creator_metadata_call_refused:
 XORQ AX, AX
 RET
