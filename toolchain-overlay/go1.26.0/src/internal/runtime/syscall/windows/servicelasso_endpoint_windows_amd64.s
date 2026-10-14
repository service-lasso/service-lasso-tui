//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Assembly-only acquisition before Go TLS initialization. The complete entry caller must save
// native nonvolatile state (including this BX and trampoline SI/DI use).
// This acquires two genuine SAME-client observations; AX1 is not authentication
// and cannot authorize bootstrap reads or transfer to ordinary rt0.
TEXT ·serviceLassoObserveEndpoint<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoEndpointOriginal(SB), BX
	CMPQ serviceLassoEndpointNativeStorage_Attempted(BX), $0
	JNE endpoint_refused
	CMPQ serviceLassoEndpointNativeStorage_Returned(BX), $0
	JNE endpoint_refused
	CMPQ serviceLassoEndpointNativeStorage_Endpoint(BX), $0
	JNE endpoint_refused
	CMPL serviceLassoEndpointNativeStorage_ServerProcessID(BX), $0
	JNE endpoint_refused
	CMPL serviceLassoEndpointNativeStorage_Flags(BX), $0
	JNE endpoint_refused
	CMPL serviceLassoEndpointNativeStorage_OutboundBytes(BX), $0
	JNE endpoint_refused
	CMPL serviceLassoEndpointNativeStorage_InboundBytes(BX), $0
	JNE endpoint_refused
	CMPL serviceLassoEndpointNativeStorage_MaximumInstances(BX), $0
	JNE endpoint_refused
	MOVQ $1, serviceLassoEndpointNativeStorage_Attempted(BX)
	LEAQ ·serviceLassoNativeRoot(SB), DX
	CMPQ serviceLassoNativeRootStorage_SourceExtent(DX), $const_serviceLassoNativeRootExtent
	JNE endpoint_failed
	MOVQ serviceLassoNativeRootStorage_Endpoint(DX), CX
	TESTQ CX, CX
	JE endpoint_failed
	CMPQ CX, $-1
	JE endpoint_failed
	MOVQ CX, serviceLassoEndpointNativeStorage_Endpoint(BX)
	LEAQ serviceLassoEndpointNativeStorage_ServerCall(BX), AX
	MOVQ ·serviceLassoPipeServerProcess(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $2, StdCallInfo_N(AX)
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	LEAQ serviceLassoEndpointNativeStorage_ServerProcessID(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+8(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE endpoint_failed
	LEAQ serviceLassoEndpointNativeStorage_ServerCall(BX), AX
	MOVQ StdCallInfo_R1(AX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE endpoint_failed
	LEAQ serviceLassoEndpointNativeStorage_InformationCall(BX), AX
	MOVQ ·serviceLassoPipeInformation(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoEndpointNativeStorage_Endpoint(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	LEAQ serviceLassoEndpointNativeStorage_Flags(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+8(AX)
	LEAQ serviceLassoEndpointNativeStorage_OutboundBytes(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+16(AX)
	LEAQ serviceLassoEndpointNativeStorage_InboundBytes(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoEndpointNativeStorage_MaximumInstances(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE endpoint_failed
	LEAQ serviceLassoEndpointNativeStorage_InformationCall(BX), AX
	MOVQ StdCallInfo_R1(AX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE endpoint_failed
	MOVQ $1, serviceLassoEndpointNativeStorage_Returned(BX)
	MOVQ $1, AX
	RET
endpoint_failed:
	MOVQ $1, serviceLassoEndpointNativeStorage_Returned(BX)
endpoint_refused:
	XORQ AX, AX
	RET

// Separate retained acquisition. The actual endpoint original is its source;
// matching PID or process creation time remains an observation in the full
// creator/image/token/descriptor/birth/loaded-code conjunction.
TEXT ·serviceLassoObserveCreator<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoCreatorOriginal(SB), BX
	CMPQ serviceLassoCreatorNativeStorage_Attempted(BX), $0
	JNE creator_refused
	CMPQ serviceLassoCreatorNativeStorage_Returned(BX), $0
	JNE creator_refused
	MOVQ $1, serviceLassoCreatorNativeStorage_Attempted(BX)
	LEAQ ·serviceLassoEndpointOriginal(SB), DX
	CMPQ serviceLassoEndpointNativeStorage_Returned(DX), $1
	JNE creator_failed
	MOVQ serviceLassoEndpointNativeStorage_ServerCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE creator_failed
	MOVQ serviceLassoEndpointNativeStorage_InformationCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE creator_failed
	MOVL serviceLassoEndpointNativeStorage_ServerProcessID(DX), CX
	TESTL CX, CX
	JE creator_failed
	MOVQ CX, serviceLassoCreatorNativeStorage_ServerProcessID(BX)
	LEAQ serviceLassoCreatorNativeStorage_OpenCall(BX), AX
	MOVQ ·serviceLassoOpenProcess(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $3, StdCallInfo_N(AX)
	// QUERY_INFORMATION | QUERY_LIMITED_INFORMATION | VM_READ | READ_CONTROL | SYNCHRONIZE.
	// bInheritHandle is false; no capability is added to child inheritance.
	MOVQ $0x121410, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $0, serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE creator_failed
	MOVQ serviceLassoCreatorNativeStorage_OpenCall+StdCallInfo_R1(BX), CX
	TESTQ CX, CX
	JE creator_failed
	MOVQ CX, serviceLassoCreatorNativeStorage_Process(BX)
	LEAQ serviceLassoCreatorNativeStorage_TimesCall(BX), AX
	MOVQ ·serviceLassoProcessTimes(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	LEAQ serviceLassoCreatorNativeStorage_CreationTime(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+8(AX)
	LEAQ serviceLassoCreatorNativeStorage_ExitTime(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+16(AX)
	LEAQ serviceLassoCreatorNativeStorage_KernelTime(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoCreatorNativeStorage_UserTime(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE creator_failed
	MOVQ serviceLassoCreatorNativeStorage_TimesCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE creator_failed
	LEAQ serviceLassoCreatorNativeStorage_LifeCall(BX), AX
	MOVQ ·serviceLassoWaitForSingleObject(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $2, StdCallInfo_N(AX)
	MOVQ serviceLassoCreatorNativeStorage_Process(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $0, serviceLassoNativeCallStorage_Arguments+8(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE creator_failed
	// A single genuine WAIT_TIMEOUT observation is not a lifetime lease.
	// WAIT_OBJECT_0, WAIT_FAILED and every other raw result remain retained.
	CMPL serviceLassoCreatorNativeStorage_LifeCall+StdCallInfo_R1(BX), $258
	JNE creator_failed
	MOVQ $1, serviceLassoCreatorNativeStorage_Returned(BX)
	MOVQ $1, AX
	RET
creator_failed:
	MOVQ $1, serviceLassoCreatorNativeStorage_Returned(BX)
creator_refused:
	XORQ AX, AX
	RET

// Acquire TOKEN_QUERY only. A failed OpenProcessToken output remains raw
// unknown backing; no query or close adopts that occurrence on failure.
TEXT ·serviceLassoObserveCreatorTokenBefore<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoTokenOriginal(SB), BX
	CMPQ serviceLassoTokenNativeStorage_Attempted(BX), $0
	JNE token_refused
	CMPQ serviceLassoTokenNativeStorage_Returned(BX), $0
	JNE token_refused
	MOVQ $1, serviceLassoTokenNativeStorage_Attempted(BX)
	LEAQ ·serviceLassoCreatorOriginal(SB), DX
	CMPQ serviceLassoCreatorNativeStorage_Returned(DX), $1
	JNE token_failed
	CMPL serviceLassoCreatorNativeStorage_LifeCall+StdCallInfo_R1(DX), $258
	JNE token_failed
	MOVQ serviceLassoCreatorNativeStorage_Process(DX), CX
	TESTQ CX, CX
	JE token_failed
	MOVQ CX, serviceLassoTokenNativeStorage_Process(BX)
	LEAQ serviceLassoTokenNativeStorage_OpenCall(BX), AX
	MOVQ ·serviceLassoOpenProcessToken(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $3, StdCallInfo_N(AX)
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $8, serviceLassoNativeCallStorage_Arguments+8(AX)
	LEAQ serviceLassoTokenNativeStorage_Token(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+16(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE token_failed
	MOVQ serviceLassoTokenNativeStorage_OpenCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE token_failed
	MOVQ serviceLassoTokenNativeStorage_Token(BX), CX
	TESTQ CX, CX
	JE token_failed
	CMPQ CX, $-1
	JE token_failed
	LEAQ serviceLassoTokenNativeStorage_BeforeCall(BX), AX
	MOVQ ·serviceLassoGetTokenInformation(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $10, serviceLassoNativeCallStorage_Arguments+8(AX)
	LEAQ serviceLassoTokenNativeStorage_Before(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVQ $56, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoTokenNativeStorage_BeforeLength(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE token_failed
	MOVQ serviceLassoTokenNativeStorage_BeforeCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE token_failed
	CMPL serviceLassoTokenNativeStorage_BeforeLength(BX), $56
	JNE token_failed
	LEAQ serviceLassoTokenNativeStorage_UserCall(BX), AX
	MOVQ ·serviceLassoGetTokenInformation(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoTokenNativeStorage_Token(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $1, serviceLassoNativeCallStorage_Arguments+8(AX)
	LEAQ serviceLassoTokenNativeStorage_User(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVQ $const_serviceLassoTokenUserCapacity, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoTokenNativeStorage_UserLength(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE token_failed
	MOVQ serviceLassoTokenNativeStorage_UserCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE token_failed
	CMPL serviceLassoTokenNativeStorage_UserLength(BX), $16
	JB token_failed
	CMPL serviceLassoTokenNativeStorage_UserLength(BX), $const_serviceLassoTokenUserCapacity
	JA token_failed
	// Do not dereference User.SID here. The independent bounded interpreter
	// must validate that actual pointer and its complete native SID extent.
	MOVQ $1, serviceLassoTokenNativeStorage_Returned(BX)
	MOVQ $1, AX
	RET
token_failed:
	MOVQ $1, serviceLassoTokenNativeStorage_Returned(BX)
token_refused:
	XORQ AX, AX
	RET

// Zero-length sizing has no descriptor/token-body allocation or stream read.
// The caller still needs independently financed backing and the full policy
// interpreter before any fill or creator authentication can succeed.
TEXT ·serviceLassoObserveSecuritySizes<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoSecuritySizingOriginal(SB), BX
	CMPQ serviceLassoSecuritySizingStorage_Attempted(BX), $0
	JNE sizes_refused
	CMPQ serviceLassoSecuritySizingStorage_Returned(BX), $0
	JNE sizes_refused
	MOVQ $1, serviceLassoSecuritySizingStorage_Attempted(BX)
	LEAQ ·serviceLassoTokenOriginal(SB), DX
	CMPQ serviceLassoTokenNativeStorage_Returned(DX), $1
	JNE sizes_failed
	MOVQ serviceLassoTokenNativeStorage_BeforeCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE sizes_failed
	MOVQ serviceLassoTokenNativeStorage_UserCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE sizes_failed
	CMPL serviceLassoTokenNativeStorage_UserLength(DX), $16
	JB sizes_failed
	CMPL serviceLassoTokenNativeStorage_UserLength(DX), $const_serviceLassoTokenUserCapacity
	JA sizes_failed
	MOVQ serviceLassoTokenNativeStorage_Process(DX), CX
	MOVQ CX, serviceLassoSecuritySizingStorage_Process(BX)
	MOVQ serviceLassoTokenNativeStorage_Token(DX), CX
	MOVQ CX, serviceLassoSecuritySizingStorage_Token(BX)
	LEAQ ·serviceLassoEndpointOriginal(SB), DX
	MOVQ serviceLassoEndpointNativeStorage_Endpoint(DX), CX
	MOVQ CX, serviceLassoSecuritySizingStorage_Endpoint(BX)
	LEAQ serviceLassoSecuritySizingStorage_EndpointCall(BX), AX
	MOVQ ·serviceLassoGetKernelObjectSecurity(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $7, serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ $0, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVQ $0, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoSecuritySizingStorage_EndpointRequired(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE sizes_failed
	MOVQ serviceLassoSecuritySizingStorage_EndpointCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JNE sizes_failed
	CMPL serviceLassoSecuritySizingStorage_EndpointCall+StdCallInfo_Err(BX), $122
	JNE sizes_failed
	CMPL serviceLassoSecuritySizingStorage_EndpointRequired(BX), $0
	JE sizes_failed
	LEAQ serviceLassoSecuritySizingStorage_ProcessCall(BX), AX
	MOVQ ·serviceLassoGetKernelObjectSecurity(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoSecuritySizingStorage_Process(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $7, serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ $0, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVQ $0, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoSecuritySizingStorage_ProcessRequired(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE sizes_failed
	MOVQ serviceLassoSecuritySizingStorage_ProcessCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JNE sizes_failed
	CMPL serviceLassoSecuritySizingStorage_ProcessCall+StdCallInfo_Err(BX), $122
	JNE sizes_failed
	CMPL serviceLassoSecuritySizingStorage_ProcessRequired(BX), $0
	JE sizes_failed
	LEAQ serviceLassoSecuritySizingStorage_TokenCall(BX), AX
	MOVQ ·serviceLassoGetTokenInformation(SB), DX
	MOVQ DX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoSecuritySizingStorage_Token(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $22, serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ $0, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVQ $0, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoSecuritySizingStorage_TokenRequired(BX), DX
	MOVQ DX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE sizes_failed
	MOVQ serviceLassoSecuritySizingStorage_TokenCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JNE sizes_failed
	CMPL serviceLassoSecuritySizingStorage_TokenCall+StdCallInfo_Err(BX), $122
	JNE sizes_failed
	CMPL serviceLassoSecuritySizingStorage_TokenRequired(BX), $0
	JE sizes_failed
	MOVQ $1, serviceLassoSecuritySizingStorage_Returned(BX)
	MOVQ $1, AX
	RET
sizes_failed:
	MOVQ $1, serviceLassoSecuritySizingStorage_Returned(BX)
sizes_refused:
	XORQ AX, AX
	RET

// Native fill primitive, reached only by the fully reserved entry caller.
// Every pointer/capacity comes from reviewed generated source storage, never
// from root scalars or LengthNeeded. Acquired bytes are not yet policy/auth.
TEXT ·serviceLassoObserveSecurityFill<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoSecurityFillOriginal(SB), BX
	CMPQ serviceLassoSecurityFillStorage_Attempted(BX), $0
	JNE fill_refused
	CMPQ serviceLassoSecurityFillStorage_Returned(BX), $0
	JNE fill_refused
	MOVQ $1, serviceLassoSecurityFillStorage_Attempted(BX)
	LEAQ ·serviceLassoSecuritySizingOriginal(SB), DX
	CMPQ serviceLassoSecuritySizingStorage_Returned(DX), $1
	JNE fill_failed
	// All three sizing calls must have genuinely returned FALSE/122.
	MOVQ serviceLassoSecuritySizingStorage_EndpointCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JNE fill_failed
	CMPL serviceLassoSecuritySizingStorage_EndpointCall+StdCallInfo_Err(DX), $122
	JNE fill_failed
	MOVQ serviceLassoSecuritySizingStorage_ProcessCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JNE fill_failed
	CMPL serviceLassoSecuritySizingStorage_ProcessCall+StdCallInfo_Err(DX), $122
	JNE fill_failed
	MOVQ serviceLassoSecuritySizingStorage_TokenCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JNE fill_failed
	CMPL serviceLassoSecuritySizingStorage_TokenCall+StdCallInfo_Err(DX), $122
	JNE fill_failed
	MOVL serviceLassoSecuritySizingStorage_EndpointRequired(DX), CX
	TESTL CX, CX
	JE fill_failed
	CMPL CX, $const_serviceLassoEndpointSecurityCapacity
	JA fill_failed
	MOVL serviceLassoSecuritySizingStorage_ProcessRequired(DX), CX
	TESTL CX, CX
	JE fill_failed
	CMPL CX, $const_serviceLassoProcessSecurityCapacity
	JA fill_failed
	MOVL serviceLassoSecuritySizingStorage_TokenRequired(DX), CX
	TESTL CX, CX
	JE fill_failed
	CMPL CX, $const_serviceLassoTokenAccessCapacity
	JA fill_failed
	LEAQ ·serviceLassoEndpointSecurityOriginal(SB), AX
	CMPQ serviceLassoEndpointSecurityRegion_Anchor(AX), $0
	JNE fill_failed
	LEAQ serviceLassoEndpointSecurityRegion_Bytes(AX), CX
	MOVQ CX, serviceLassoSecurityFillStorage_EndpointBuffer(BX)
	LEAQ ·serviceLassoProcessSecurityOriginal(SB), AX
	CMPQ serviceLassoProcessSecurityRegion_Anchor(AX), $0
	JNE fill_failed
	LEAQ serviceLassoProcessSecurityRegion_Bytes(AX), CX
	MOVQ CX, serviceLassoSecurityFillStorage_ProcessBuffer(BX)
	LEAQ ·serviceLassoTokenAccessOriginal(SB), AX
	CMPQ serviceLassoTokenAccessRegion_Anchor(AX), $0
	JNE fill_failed
	LEAQ serviceLassoTokenAccessRegion_Bytes(AX), CX
	MOVQ CX, serviceLassoSecurityFillStorage_TokenBuffer(BX)
	MOVL $const_serviceLassoEndpointSecurityCapacity, CX
	MOVQ CX, serviceLassoSecurityFillStorage_EndpointCapacity(BX)
	MOVL $const_serviceLassoProcessSecurityCapacity, CX
	MOVQ CX, serviceLassoSecurityFillStorage_ProcessCapacity(BX)
	MOVL $const_serviceLassoTokenAccessCapacity, CX
	MOVQ CX, serviceLassoSecurityFillStorage_TokenCapacity(BX)
	MOVQ $1, ·serviceLassoEndpointSecurityOriginal(SB)
	MOVQ $1, ·serviceLassoProcessSecurityOriginal(SB)
	MOVQ $1, ·serviceLassoTokenAccessOriginal(SB)
	LEAQ serviceLassoSecurityFillStorage_EndpointCall(BX), AX
	MOVQ ·serviceLassoGetKernelObjectSecurity(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoSecuritySizingStorage_Endpoint(DX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $7, serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ serviceLassoSecurityFillStorage_EndpointBuffer(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVL $const_serviceLassoEndpointSecurityCapacity, CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoSecurityFillStorage_EndpointRequired(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE fill_failed
	MOVQ serviceLassoSecurityFillStorage_EndpointCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE fill_failed
	LEAQ ·serviceLassoSecuritySizingOriginal(SB), DX
	MOVL serviceLassoSecuritySizingStorage_EndpointRequired(DX), CX
	CMPL CX, serviceLassoSecurityFillStorage_EndpointRequired(BX)
	JNE fill_failed
	LEAQ serviceLassoSecurityFillStorage_ProcessCall(BX), AX
	MOVQ ·serviceLassoGetKernelObjectSecurity(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoSecuritySizingStorage_Process(DX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $7, serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ serviceLassoSecurityFillStorage_ProcessBuffer(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVL $const_serviceLassoProcessSecurityCapacity, CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoSecurityFillStorage_ProcessRequired(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE fill_failed
	MOVQ serviceLassoSecurityFillStorage_ProcessCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE fill_failed
	LEAQ ·serviceLassoSecuritySizingOriginal(SB), DX
	MOVL serviceLassoSecuritySizingStorage_ProcessRequired(DX), CX
	CMPL CX, serviceLassoSecurityFillStorage_ProcessRequired(BX)
	JNE fill_failed
	LEAQ serviceLassoSecurityFillStorage_TokenCall(BX), AX
	MOVQ ·serviceLassoGetTokenInformation(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoSecuritySizingStorage_Token(DX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $22, serviceLassoNativeCallStorage_Arguments+8(AX)
	MOVQ serviceLassoSecurityFillStorage_TokenBuffer(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVL $const_serviceLassoTokenAccessCapacity, CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoSecurityFillStorage_TokenRequired(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE fill_failed
	MOVQ serviceLassoSecurityFillStorage_TokenCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE fill_failed
	LEAQ ·serviceLassoSecuritySizingOriginal(SB), DX
	MOVL serviceLassoSecuritySizingStorage_TokenRequired(DX), CX
	CMPL CX, serviceLassoSecurityFillStorage_TokenRequired(BX)
	JNE fill_failed
	LEAQ serviceLassoSecurityFillStorage_AfterCall(BX), AX
	MOVQ ·serviceLassoGetTokenInformation(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoSecuritySizingStorage_Token(DX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	MOVQ $10, serviceLassoNativeCallStorage_Arguments+8(AX)
	LEAQ serviceLassoSecurityFillStorage_After(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
	MOVQ $56, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoSecurityFillStorage_AfterLength(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE fill_failed
	MOVQ serviceLassoSecurityFillStorage_AfterCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE fill_failed
	CMPL serviceLassoSecurityFillStorage_AfterLength(BX), $56
	JNE fill_failed
	MOVQ $1, serviceLassoSecurityFillStorage_Returned(BX)
	MOVQ $1, AX
	RET
fill_failed:
	MOVQ $1, serviceLassoSecurityFillStorage_Returned(BX)
fill_refused:
	XORQ AX, AX
	RET

// No Go wrapper/Go G or TLS/heap or native-pointer dereference before range proof.
// BX plus volatile scratch are clobbered under the private entry contract.
TEXT ·serviceLassoInterpretTokenUser<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoTokenUserInterpretationOriginal(SB), BX
	CMPQ serviceLassoTokenUserInterpretationStorage_Attempted(BX), $0
	JNE user_refused
	CMPQ serviceLassoTokenUserInterpretationStorage_Returned(BX), $0
	JNE user_refused
	MOVQ $1, serviceLassoTokenUserInterpretationStorage_Attempted(BX)
	LEAQ ·serviceLassoTokenOriginal(SB), DX
	MOVQ DX, serviceLassoTokenUserInterpretationStorage_NativeOriginal(BX)
	CMPQ serviceLassoTokenNativeStorage_Returned(DX), $1
	JNE user_failed
	MOVQ serviceLassoTokenNativeStorage_UserCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE user_failed
	CMPQ serviceLassoTokenNativeStorage_UserCall+serviceLassoNativeCallStorage_Returned(DX), $1
	JNE user_failed
	MOVL serviceLassoTokenNativeStorage_UserLength(DX), R9
	CMPL R9, $16
	JB user_failed
	CMPL R9, $const_serviceLassoTokenUserCapacity
	JA user_failed
	LEAQ serviceLassoTokenNativeStorage_User(DX), R8
	MOVQ R8, serviceLassoTokenUserInterpretationStorage_InputBase(BX)
	MOVQ R9, serviceLassoTokenUserInterpretationStorage_InputExtent(BX)
	MOVQ serviceLassoTokenUserBody_SID(R8), R10
	MOVQ R10, serviceLassoTokenUserInterpretationStorage_RawSID(BX)
	MOVQ R8, CX
	ADDQ $16, CX
	JC user_failed
	CMPQ R10, CX
	JB user_failed
	MOVQ R8, R11
	ADDQ R9, R11
	JC user_failed
	MOVQ R11, DX
	SUBQ $8, R11
	CMPQ R10, R11
	JA user_failed
	MOVQ R10, CX
	ANDQ $3, CX
	JNE user_failed
	// Only now is the actual numeric native SID view safe to inspect.
	CMPB 0(R10), $1
	JNE user_failed
	MOVBLZX 1(R10), CX
	CMPL CX, $15
	JA user_failed
	SHLQ $2, CX
	ADDQ $8, CX
	MOVQ R10, AX
	ADDQ CX, AX
	JC user_failed
	CMPQ AX, DX
	JA user_failed
	MOVQ R10, serviceLassoTokenUserInterpretationStorage_SID(BX)
	MOVQ CX, serviceLassoTokenUserInterpretationStorage_SIDExtent(BX)
	MOVQ $1, serviceLassoTokenUserInterpretationStorage_Returned(BX)
	MOVQ $1, AX
	RET
user_failed:
	MOVQ $1, serviceLassoTokenUserInterpretationStorage_Failure(BX)
	MOVQ $1, serviceLassoTokenUserInterpretationStorage_Returned(BX)
user_refused:
	XORQ AX, AX
	RET

// Private leaf: AX actual descriptor base, CX completed extent, DX relative
// SID offset. Only bounded original memory is inspected. Clobbers AX/CX/DX/R10.
TEXT ·serviceLassoRelativeSID<ABI0>(SB),NOSPLIT|NOFRAME,$0
	CMPQ DX, $20
	JB relative_sid_failed
	MOVQ DX, R10
	ANDQ $3, R10
	JNE relative_sid_failed
	CMPQ DX, CX
	JA relative_sid_failed
	SUBQ DX, CX
	CMPQ CX, $8
	JB relative_sid_failed
	ADDQ DX, AX
	JC relative_sid_failed
	MOVQ AX, R10
	CMPB 0(R10), $1
	JNE relative_sid_failed
	MOVBLZX 1(R10), DX
	CMPL DX, $15
	JA relative_sid_failed
	SHLQ $2, DX
	ADDQ $8, DX
	CMPQ DX, CX
	JA relative_sid_failed
	MOVQ DX, CX
	MOVQ R10, DX
	MOVQ $1, AX
	RET
relative_sid_failed:
	XORQ AX, AX
	RET

// Exact constructor representation, independent of later Go interpretation.
// AX1 is not creator/entry/policy authority. BX/R12/R13 are private clobbers;
// the complete default-entry caller preserves native nonvolatile state.
TEXT ·serviceLassoInterpretEndpointDACL<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoEndpointInterpretationOriginal(SB), BX
	CMPQ serviceLassoEndpointInterpretationStorage_Attempted(BX), $0
	JNE dacl_refused
	CMPQ serviceLassoEndpointInterpretationStorage_Returned(BX), $0
	JNE dacl_refused
	MOVQ $1, serviceLassoEndpointInterpretationStorage_Attempted(BX)
	LEAQ ·serviceLassoSecurityFillOriginal(SB), DX
	MOVQ DX, serviceLassoEndpointInterpretationStorage_NativeOriginal(BX)
	CMPQ serviceLassoSecurityFillStorage_EndpointCall+serviceLassoNativeCallStorage_Returned(DX), $1
	JNE dacl_failed
	MOVQ serviceLassoSecurityFillStorage_EndpointCall+StdCallInfo_R1(DX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE dacl_failed
	MOVL serviceLassoSecurityFillStorage_EndpointRequired(DX), R9
	CMPL R9, $20
	JB dacl_failed
	CMPL R9, $const_serviceLassoEndpointSecurityCapacity
	JA dacl_failed
	LEAQ ·serviceLassoEndpointSecurityOriginal+serviceLassoEndpointSecurityRegion_Bytes(SB), R8
	CMPQ R8, serviceLassoSecurityFillStorage_EndpointBuffer(DX)
	JNE dacl_failed
	CMPQ R8, serviceLassoSecurityFillStorage_EndpointCall+serviceLassoNativeCallStorage_Arguments+16(DX)
	JNE dacl_failed
	MOVQ R8, serviceLassoEndpointInterpretationStorage_InputBase(BX)
	MOVQ R9, serviceLassoEndpointInterpretationStorage_InputExtent(BX)
	MOVQ R8, AX
	ADDQ R9, AX
	JC dacl_failed
	CMPB 0(R8), $1
	JNE dacl_failed
	CMPB 1(R8), $0
	JNE dacl_failed
	MOVWLZX 2(R8), AX
	ANDQ $0x9004, AX
	CMPQ AX, $0x9004
	JNE dacl_failed
	MOVQ R8, AX
	MOVQ R9, CX
	MOVL 4(R8), DX
	CALL ·serviceLassoRelativeSID<ABI0>(SB)
	CMPQ AX, $1
	JNE dacl_failed
	MOVQ DX, serviceLassoEndpointInterpretationStorage_OwnerSID(BX)
	MOVQ CX, serviceLassoEndpointInterpretationStorage_OwnerExtent(BX)
	MOVQ R8, AX
	MOVQ R9, CX
	MOVL 8(R8), DX
	CALL ·serviceLassoRelativeSID<ABI0>(SB)
	CMPQ AX, $1
	JNE dacl_failed
	MOVQ DX, serviceLassoEndpointInterpretationStorage_GroupSID(BX)
	MOVQ CX, serviceLassoEndpointInterpretationStorage_GroupExtent(BX)
	MOVL 16(R8), R10
	CMPQ R10, $20
	JB dacl_failed
	MOVQ R10, AX
	ANDQ $3, AX
	JNE dacl_failed
	CMPQ R10, R9
	JA dacl_failed
	MOVQ R9, R11
	SUBQ R10, R11
	CMPQ R11, $8
	JB dacl_failed
	ADDQ R8, R10
	JC dacl_failed
	MOVQ R10, serviceLassoEndpointInterpretationStorage_DACL(BX)
	CMPB 0(R10), $2
	JNE dacl_failed
	CMPB 1(R10), $0
	JNE dacl_failed
	CMPW 6(R10), $0
	JNE dacl_failed
	CMPW 4(R10), $1
	JNE dacl_failed
	MOVWLZX 2(R10), R12
	CMPQ R12, $16
	JB dacl_failed
	CMPQ R12, R11
	JA dacl_failed
	MOVQ R12, serviceLassoEndpointInterpretationStorage_DACLExtent(BX)
	LEAQ 8(R10), R11
	SUBQ $8, R12
	CMPB 0(R11), $0
	JNE dacl_failed
	CMPB 1(R11), $0
	JNE dacl_failed
	MOVWLZX 2(R11), AX
	CMPQ AX, R12
	JNE dacl_failed
	CMPQ AX, $16
	JB dacl_failed
	CMPL 4(R11), $0x0012018f
	JNE dacl_failed
	LEAQ 8(R11), R12
	SUBQ $8, AX
	MOVQ R12, serviceLassoEndpointInterpretationStorage_ACESID(BX)
	MOVQ AX, serviceLassoEndpointInterpretationStorage_ACESIDExtent(BX)
	LEAQ ·serviceLassoTokenUserInterpretationOriginal(SB), DX
	MOVQ DX, serviceLassoEndpointInterpretationStorage_UserOriginal(BX)
	CMPQ serviceLassoTokenUserInterpretationStorage_Returned(DX), $1
	JNE dacl_failed
	CMPQ serviceLassoTokenUserInterpretationStorage_Failure(DX), $0
	JNE dacl_failed
	CMPQ AX, serviceLassoTokenUserInterpretationStorage_SIDExtent(DX)
	JNE dacl_failed
	MOVQ AX, CX
	MOVQ serviceLassoTokenUserInterpretationStorage_SID(DX), R13
	XORQ AX, AX
dacl_sid_compare:
	MOVBLZX (R12)(AX*1), DX
	MOVBLZX (R13)(AX*1), R10
	CMPL DX, R10
	JNE dacl_failed
	INCQ AX
	CMPQ AX, CX
	JB dacl_sid_compare
	MOVQ $1, serviceLassoEndpointInterpretationStorage_Returned(BX)
	MOVQ $1, AX
	RET
dacl_failed:
	MOVQ $1, serviceLassoEndpointInterpretationStorage_Failure(BX)
	MOVQ $1, serviceLassoEndpointInterpretationStorage_Returned(BX)
dacl_refused:
	XORQ AX, AX
	RET

// Join SAME native token and the genuine successful statistics calls around
// the retained access-information fill. ModifiedId equality is necessary,
// never a substitute for the full own-writer/lifetime/policy conjunction.
TEXT ·serviceLassoInterpretTokenBracket<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoTokenBracketOriginal(SB), BX
	CMPQ serviceLassoTokenBracketStorage_Attempted(BX), $0
	JNE bracket_refused
	CMPQ serviceLassoTokenBracketStorage_Returned(BX), $0
	JNE bracket_refused
	MOVQ $1, serviceLassoTokenBracketStorage_Attempted(BX)
	LEAQ ·serviceLassoTokenOriginal(SB), R8
	LEAQ ·serviceLassoSecurityFillOriginal(SB), R9
	MOVQ R8, serviceLassoTokenBracketStorage_BeforeOriginal(BX)
	MOVQ R9, serviceLassoTokenBracketStorage_AfterOriginal(BX)
	CMPQ serviceLassoTokenNativeStorage_BeforeCall+serviceLassoNativeCallStorage_Returned(R8), $1
	JNE bracket_failed
	MOVQ serviceLassoTokenNativeStorage_BeforeCall+StdCallInfo_R1(R8), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE bracket_failed
	CMPL serviceLassoTokenNativeStorage_BeforeLength(R8), $56
	JNE bracket_failed
	CMPQ serviceLassoSecurityFillStorage_TokenCall+serviceLassoNativeCallStorage_Returned(R9), $1
	JNE bracket_failed
	MOVQ serviceLassoSecurityFillStorage_TokenCall+StdCallInfo_R1(R9), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE bracket_failed
	CMPQ serviceLassoSecurityFillStorage_AfterCall+serviceLassoNativeCallStorage_Returned(R9), $1
	JNE bracket_failed
	MOVQ serviceLassoSecurityFillStorage_AfterCall+StdCallInfo_R1(R9), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE bracket_failed
	CMPL serviceLassoSecurityFillStorage_AfterLength(R9), $56
	JNE bracket_failed
	MOVQ serviceLassoTokenNativeStorage_Token(R8), AX
	TESTQ AX, AX
	JE bracket_failed
	MOVQ AX, serviceLassoTokenBracketStorage_Token(BX)
	CMPQ AX, serviceLassoTokenNativeStorage_BeforeCall+serviceLassoNativeCallStorage_Arguments(R8)
	JNE bracket_failed
	CMPQ AX, serviceLassoSecurityFillStorage_TokenCall+serviceLassoNativeCallStorage_Arguments(R9)
	JNE bracket_failed
	CMPQ AX, serviceLassoSecurityFillStorage_AfterCall+serviceLassoNativeCallStorage_Arguments(R9)
	JNE bracket_failed
	LEAQ serviceLassoTokenNativeStorage_Before(R8), R10
	LEAQ serviceLassoSecurityFillStorage_After(R9), R11
	CMPQ R10, serviceLassoTokenNativeStorage_BeforeCall+serviceLassoNativeCallStorage_Arguments+16(R8)
	JNE bracket_failed
	CMPQ R11, serviceLassoSecurityFillStorage_AfterCall+serviceLassoNativeCallStorage_Arguments+16(R9)
	JNE bracket_failed
	MOVQ serviceLassoTokenStatistics_TokenID(R10), AX
	CMPQ AX, serviceLassoTokenStatistics_TokenID(R11)
	JNE bracket_failed
	MOVQ serviceLassoTokenStatistics_AuthenticationID(R10), AX
	CMPQ AX, serviceLassoTokenStatistics_AuthenticationID(R11)
	JNE bracket_failed
	MOVQ serviceLassoTokenStatistics_ModifiedID(R10), AX
	CMPQ AX, serviceLassoTokenStatistics_ModifiedID(R11)
	JNE bracket_failed
	MOVL serviceLassoTokenStatistics_TokenType(R10), AX
	CMPL AX, serviceLassoTokenStatistics_TokenType(R11)
	JNE bracket_failed
	// ImpersonationLevel is meaningful only for an impersonation token.
	CMPL AX, $1
	JE bracket_stable
	CMPL AX, $2
	JNE bracket_failed
	MOVL serviceLassoTokenStatistics_ImpersonationLevel(R10), AX
	CMPL AX, $3
	JA bracket_failed
	CMPL AX, serviceLassoTokenStatistics_ImpersonationLevel(R11)
	JNE bracket_failed
bracket_stable:
	MOVQ $1, serviceLassoTokenBracketStorage_Returned(BX)
	MOVQ $1, AX
	RET
bracket_failed:
	MOVQ $1, serviceLassoTokenBracketStorage_Failure(BX)
	MOVQ $1, serviceLassoTokenBracketStorage_Returned(BX)
bracket_refused:
	XORQ AX, AX
	RET

// Native current-child acquisition only, private clobbers BX and volatile
// registers. No copied root identity or public parser input is consulted.
TEXT ·serviceLassoObserveChild<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoChildOriginal(SB), BX
	CMPQ serviceLassoChildNativeStorage_Attempted(BX), $0
	JNE child_refused
	CMPQ serviceLassoChildNativeStorage_Returned(BX), $0
	JNE child_refused
	MOVQ $1, serviceLassoChildNativeStorage_Attempted(BX)
	LEAQ serviceLassoChildNativeStorage_ProcessCall(BX), AX
	MOVQ ·serviceLassoCurrentProcess(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $0, StdCallInfo_N(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE child_failed
	// GetCurrentProcess returns the calling process pseudo-handle, not a new
	// closeable native entry. Preserve the genuine raw result in its call.
	MOVQ serviceLassoChildNativeStorage_ProcessCall+StdCallInfo_R1(BX), CX
	CMPQ CX, $-1
	JNE child_failed
	MOVQ CX, serviceLassoChildNativeStorage_Process(BX)
	LEAQ serviceLassoChildNativeStorage_ProcessIDCall(BX), AX
	MOVQ ·serviceLassoCurrentProcessID(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $0, StdCallInfo_N(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE child_failed
	MOVL serviceLassoChildNativeStorage_ProcessIDCall+StdCallInfo_R1(BX), CX
	TESTQ CX, CX
	JE child_failed
	MOVQ CX, serviceLassoChildNativeStorage_ProcessID(BX)
	LEAQ serviceLassoChildNativeStorage_ThreadIDCall(BX), AX
	MOVQ ·serviceLassoCurrentThreadID(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $0, StdCallInfo_N(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE child_failed
	MOVL serviceLassoChildNativeStorage_ThreadIDCall+StdCallInfo_R1(BX), CX
	TESTQ CX, CX
	JE child_failed
	MOVQ CX, serviceLassoChildNativeStorage_ThreadID(BX)
	LEAQ serviceLassoChildNativeStorage_TimesCall(BX), AX
	MOVQ ·serviceLassoProcessTimes(SB), CX
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $5, StdCallInfo_N(AX)
	MOVQ serviceLassoChildNativeStorage_Process(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	LEAQ serviceLassoChildNativeStorage_CreationTime(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+8(AX)
	LEAQ serviceLassoChildNativeStorage_ExitTime(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
	LEAQ serviceLassoChildNativeStorage_KernelTime(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+24(AX)
	LEAQ serviceLassoChildNativeStorage_UserTime(BX), CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments+32(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	CMPQ AX, $1
	JNE child_failed
	MOVQ serviceLassoChildNativeStorage_TimesCall+StdCallInfo_R1(BX), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	CMPQ AX, $0
	JE child_failed
	MOVQ $1, serviceLassoChildNativeStorage_Returned(BX)
	MOVQ $1, AX
	RET
child_failed:
	MOVQ $1, serviceLassoChildNativeStorage_Returned(BX)
child_refused:
	XORQ AX, AX
	RET
