//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Uncalled parameterized source primitive until complete authentic entry is
// installed. Numeric endpoint/framing/counts never grant read authority.
// Outer entry preserves native registers/SP and retains every call/backing.
TEXT ·serviceLassoReadArgumentHeader<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoHeaderReadOriginal(SB), BX
	MOVQ BX, DX
	MOVQ $const_serviceLassoHeaderReadExtent, R8
reader_storage_fresh:
	CMPQ 0(DX), $0
	JNE reader_refused
	ADDQ $8, DX
	SUBQ $8, R8
	JNE reader_storage_fresh
	LEAQ ·serviceLassoArgumentHeaderOriginal(SB), DX
	MOVQ $48, R8
reader_header_fresh:
	CMPQ 0(DX), $0
	JNE reader_refused
	ADDQ $8, DX
	SUBQ $8, R8
	JNE reader_header_fresh
	MOVQ $1, serviceLassoHeaderReadStorage_Attempted(BX)
	MOVQ ·serviceLassoNativeRoot+serviceLassoNativeRootStorage_Endpoint(SB), CX
	MOVQ CX, serviceLassoHeaderReadStorage_Endpoint(BX)
	TESTQ CX, CX
	JE reader_failed
	CMPQ CX, $-1
	JE reader_failed
reader_next:
	MOVQ serviceLassoHeaderReadStorage_Used(BX), AX
	CMPQ AX, $48
	JAE reader_failed
	IMULQ $const_serviceLassoHeaderReadCallExtent, AX
	LEAQ serviceLassoHeaderReadStorage_Calls(BX)(AX*1), AX
	MOVQ serviceLassoHeaderReadStorage_Completed(BX), DX
	MOVQ DX, serviceLassoHeaderReadCall_Offset(AX)
	MOVQ $48, R8
	SUBQ DX, R8
	MOVQ R8, serviceLassoHeaderReadCall_Requested(AX)
	LEAQ ·serviceLassoArgumentHeaderOriginal(SB), CX
	ADDQ DX, CX
	MOVQ CX, serviceLassoHeaderReadCall_Call+serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ serviceLassoHeaderReadStorage_Endpoint(BX), CX
	MOVQ CX, serviceLassoHeaderReadCall_Call+serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ R8, serviceLassoHeaderReadCall_Call+serviceLassoNativeCallStorage_Arguments+16(AX)
	LEAQ serviceLassoHeaderReadCall_Count(AX), CX
	MOVQ CX, serviceLassoHeaderReadCall_Call+serviceLassoNativeCallStorage_Arguments+24(AX)
	// Fresh fifth argument remains NULL for the actual synchronous endpoint.
	MOVQ ·serviceLassoReadFile(SB), CX
	MOVQ CX, serviceLassoHeaderReadCall_Call+StdCallInfo_Fn(AX)
	MOVQ $5, serviceLassoHeaderReadCall_Call+StdCallInfo_N(AX)
	INCQ serviceLassoHeaderReadStorage_Used(BX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE reader_failed
	// Recompute from retained owner state; volatile registers crossed StdCall.
	MOVQ serviceLassoHeaderReadStorage_Used(BX), DX
	DECQ DX
	IMULQ $const_serviceLassoHeaderReadCallExtent, DX
	LEAQ serviceLassoHeaderReadStorage_Calls(BX)(DX*1), DX
	MOVQ serviceLassoHeaderReadCall_Call+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	TESTQ AX, AX
	JE reader_failed
	MOVL serviceLassoHeaderReadCall_Count(DX), CX
	TESTQ CX, CX
	JE reader_failed
	CMPQ CX, serviceLassoHeaderReadCall_Requested(DX)
	JA reader_failed
	ADDQ CX, serviceLassoHeaderReadStorage_Completed(BX)
	CMPQ serviceLassoHeaderReadStorage_Completed(BX), $48
	JB reader_next
	MOVQ $1, serviceLassoHeaderReadStorage_Returned(BX)
	MOVQ $1, AX
	RET
reader_failed:
	MOVQ $1, serviceLassoHeaderReadStorage_Failure(BX)
	MOVQ $1, serviceLassoHeaderReadStorage_Returned(BX)
reader_refused:
	XORQ AX, AX
	RET

// Pure representation interpretation, uncalled until authentic completed read.
TEXT ·serviceLassoInterpretArgumentHeader<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoHeaderInterpretOriginal(SB), BX
	MOVQ BX, DX
	MOVQ $const_serviceLassoHeaderInterpretExtent, R8
header_interpret_fresh:
	CMPQ 0(DX), $0
	JNE header_interpret_refused
	ADDQ $8, DX
	SUBQ $8, R8
	JNE header_interpret_fresh
	MOVQ $1, serviceLassoHeaderInterpretStorage_Attempted(BX)
	LEAQ ·serviceLassoHeaderReadOriginal(SB), DX
	CMPQ serviceLassoHeaderReadStorage_Returned(DX), $1
	JNE header_interpret_failed
	CMPQ serviceLassoHeaderReadStorage_Failure(DX), $0
	JNE header_interpret_failed
	CMPQ serviceLassoHeaderReadStorage_Completed(DX), $48
	JNE header_interpret_failed
	LEAQ ·serviceLassoArgumentHeaderOriginal(SB), DX
	LEAQ ·serviceLassoNativeRoot+serviceLassoNativeRootStorage_ArgumentHeader(SB), CX
	MOVQ $48, R8
header_source_compare:
	MOVQ 0(DX), R9
	CMPQ R9, 0(CX)
	JNE header_interpret_failed
	ADDQ $8, DX
	ADDQ $8, CX
	SUBQ $8, R8
	JNE header_source_compare
	LEAQ ·serviceLassoArgumentHeaderOriginal(SB), DX
	MOVQ $0x3156475241574c53, CX
	CMPQ 0(DX), CX
	JNE header_interpret_failed
	CMPL 8(DX), $1
	JNE header_interpret_failed
	MOVL 12(DX), CX
	MOVQ CX, serviceLassoHeaderInterpretStorage_Flags(BX)
	CMPQ CX, $1
	JA header_interpret_failed
	MOVQ 16(DX), R8
	MOVQ R8, serviceLassoHeaderInterpretStorage_RawUnits(BX)
	TESTQ R8, R8
	JE header_interpret_failed
	CMPQ R8, $const_serviceLassoRawArgumentUnits
	JA header_interpret_failed
	MOVQ 24(DX), R9
	MOVQ R9, serviceLassoHeaderInterpretStorage_ModuleUnits(BX)
	MOVQ $const_serviceLassoModuleUnitsIncludingNUL, AX
	CMPQ R9, AX
	JA header_interpret_failed
	ADDQ R8, R9
	JC header_interpret_failed
	SHLQ $1, R9
	JC header_interpret_failed
	MOVQ R9, serviceLassoHeaderInterpretStorage_BodyExtent(BX)
	CMPQ R9, 40(DX)
	JNE header_interpret_failed
	MOVQ $const_serviceLassoArgumentBodyBytes, AX
	CMPQ R9, AX
	JA header_interpret_failed
	MOVQ 32(DX), R9
	MOVQ R9, serviceLassoHeaderInterpretStorage_ExpectedCount(BX)
	CMPQ R9, ·serviceLassoNativeRoot+serviceLassoNativeRootStorage_PublicCount(SB)
	JNE header_interpret_failed
	TESTQ CX, CX
	JE header_nonfallback
	CMPQ R8, $1
	JNE header_interpret_failed
	CMPQ serviceLassoHeaderInterpretStorage_ModuleUnits(BX), $0
	JE header_interpret_failed
	CMPQ R9, $1
	JNE header_interpret_failed
	JMP header_interpret_done
header_nonfallback:
	CMPQ R8, $1
	JBE header_interpret_failed
	CMPQ serviceLassoHeaderInterpretStorage_ModuleUnits(BX), $0
	JNE header_interpret_failed
	CMPQ R9, $const_serviceLassoMaximumPublicArguments
	JA header_interpret_failed
header_interpret_done:
	MOVQ $1, serviceLassoHeaderInterpretStorage_Returned(BX)
	MOVQ $1, AX
	RET
header_interpret_failed:
	MOVQ $1, serviceLassoHeaderInterpretStorage_Failure(BX)
	MOVQ $1, serviceLassoHeaderInterpretStorage_Returned(BX)
header_interpret_refused:
	XORQ AX, AX
	RET

// Exact compact prefix guard. Existing native helper accesses offsets0..63
// and Arguments base64; pinned StdCall copies exactly N5 words, never args6+.
// No generic400-byte extent is read or assumed by this route.
TEXT ·serviceLassoBodyReadNativeCall<ABI0>(SB),NOSPLIT|NOFRAME,$0
	TESTQ AX, AX
	JE body_native_refused
	CMPQ StdCallInfo_N(AX), $5
	JNE body_native_refused
	CMPL serviceLassoBodyReadCall_Count(AX), $0
	JNE body_native_refused
	JMP ·serviceLassoNativeCall<ABI0>(SB)
body_native_refused:
	XORQ AX, AX
	RET

// Uncalled until full authentic entry/finance/D/provider/issuing-thread proof.
// All records remain original; partial positive counts consume distinct rows.
TEXT ·serviceLassoReadArgumentBody<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoBodyReadOriginal(SB), BX
	MOVQ BX, DX
	MOVQ $const_serviceLassoBodyReadExtent, R8
body_owner_fresh:
	CMPQ 0(DX), $0
	JNE body_read_refused
	ADDQ $8, DX
	SUBQ $8, R8
	JNE body_owner_fresh
	LEAQ ·serviceLassoBodyReadCalls(SB), DX
	MOVQ $const_serviceLassoBodyReadRecordsBytes, R8
body_calls_fresh:
	CMPQ 0(DX), $0
	JNE body_read_refused
	ADDQ $8, DX
	SUBQ $8, R8
	JNE body_calls_fresh
	LEAQ ·serviceLassoArgumentBodyOriginal(SB), DX
	MOVQ $const_serviceLassoArgumentBodyBytes, R8
body_target_fresh:
	CMPB 0(DX), $0
	JNE body_read_refused
	INCQ DX
	DECQ R8
	JNE body_target_fresh
	MOVQ $1, serviceLassoBodyReadStorage_Attempted(BX)
	LEAQ ·serviceLassoHeaderInterpretOriginal(SB), DX
	CMPQ serviceLassoHeaderInterpretStorage_Returned(DX), $1
	JNE body_read_failed
	CMPQ serviceLassoHeaderInterpretStorage_Failure(DX), $0
	JNE body_read_failed
	CMPQ serviceLassoHeaderInterpretStorage_BodyExtent(DX), $0
	JE body_read_failed
	MOVQ serviceLassoHeaderInterpretStorage_BodyExtent(DX), CX
	MOVQ $const_serviceLassoArgumentBodyBytes, AX
	CMPQ CX, AX
	JA body_read_failed
	MOVQ ·serviceLassoHeaderReadOriginal+serviceLassoHeaderReadStorage_Endpoint(SB), CX
	MOVQ CX, serviceLassoBodyReadStorage_Endpoint(BX)
	TESTQ CX, CX
	JE body_read_failed
	CMPQ CX, $-1
	JE body_read_failed
body_read_next:
	MOVQ serviceLassoBodyReadStorage_Used(BX), AX
	MOVQ $const_serviceLassoArgumentBodyBytes, CX
	CMPQ AX, CX
	JAE body_read_failed
	IMULQ $const_serviceLassoBodyReadCallExtent, AX
	LEAQ ·serviceLassoBodyReadCalls(SB), DX
	ADDQ DX, AX
	MOVQ serviceLassoBodyReadStorage_Completed(BX), DX
	MOVQ DX, serviceLassoBodyReadCall_Offset(AX)
	MOVQ ·serviceLassoHeaderInterpretOriginal+serviceLassoHeaderInterpretStorage_BodyExtent(SB), R8
	SUBQ DX, R8
	// Actual ReadFile request is DWORD; preserve a zero-extended request.
	MOVL $0xffffffff, CX
	CMPQ R8, CX
	JBE body_request_ready
	MOVQ CX, R8
body_request_ready:
	MOVQ R8, serviceLassoBodyReadCall_Requested(AX)
	MOVQ R8, serviceLassoBodyReadCall_Arguments+16(AX)
	LEAQ ·serviceLassoArgumentBodyOriginal(SB), CX
	ADDQ DX, CX
	MOVQ CX, serviceLassoBodyReadCall_Arguments+8(AX)
	MOVQ serviceLassoBodyReadStorage_Endpoint(BX), CX
	MOVQ CX, serviceLassoBodyReadCall_Arguments(AX)
	LEAQ serviceLassoBodyReadCall_Count(AX), CX
	MOVQ CX, serviceLassoBodyReadCall_Arguments+24(AX)
	// Fifth native argument remains fresh NULL (synchronous ReadFile).
	MOVQ ·serviceLassoReadFile(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	INCQ serviceLassoBodyReadStorage_Used(BX)
	CALL ·serviceLassoBodyReadNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE body_read_failed
	MOVQ serviceLassoBodyReadStorage_Used(BX), DX
	DECQ DX
	IMULQ $const_serviceLassoBodyReadCallExtent, DX
	LEAQ ·serviceLassoBodyReadCalls(SB), CX
	ADDQ CX, DX
	MOVQ StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	TESTQ AX, AX
	JE body_read_failed
	MOVL serviceLassoBodyReadCall_Count(DX), CX
	TESTQ CX, CX
	JE body_read_failed
	CMPQ CX, serviceLassoBodyReadCall_Requested(DX)
	JA body_read_failed
	ADDQ CX, serviceLassoBodyReadStorage_Completed(BX)
	MOVQ ·serviceLassoHeaderInterpretOriginal+serviceLassoHeaderInterpretStorage_BodyExtent(SB), CX
	CMPQ serviceLassoBodyReadStorage_Completed(BX), CX
	JB body_read_next
	MOVQ $1, serviceLassoBodyReadStorage_Returned(BX)
	MOVQ $1, AX
	RET
body_read_failed:
	MOVQ $1, serviceLassoBodyReadStorage_Failure(BX)
	MOVQ $1, serviceLassoBodyReadStorage_Returned(BX)
body_read_refused:
	XORQ AX, AX
	RET
