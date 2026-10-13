#ifdef _WIN32
#include "owner_windows.h"

static void sl_win_enter(sl_win_call *call, const char *source, void *subject) {
    call->source = source;
    call->subject = subject;
    call->entered = 1;
}
static int sl_win_boolean_result(sl_win_call *call, BOOL result, DWORD error) {
    call->result = (ULONG_PTR)result;
    call->last_error = error;
    call->returned = 1;
    return result != FALSE;
}

int sl_win_prepare_birth(sl_win_birth *birth) {
    BOOL result;
    DWORD error;
    SIZE_T unit;unsigned security_index;
    if (!birth || birth->attempted || birth->creator_thread_query.source ||
        birth->creator_thread_query.subject || birth->creator_thread_query.result ||
        birth->creator_thread_query.last_error || birth->creator_thread_query.entered ||
        birth->creator_thread_query.returned || birth->creator_thread_id || birth->creation_flags ||
        birth->creator_thread_pseudo || birth->held_creator_thread ||
        birth->creator_thread_pseudo_query.source || birth->creator_thread_pseudo_query.subject ||
        birth->creator_thread_pseudo_query.result || birth->creator_thread_pseudo_query.last_error ||
        birth->creator_thread_pseudo_query.entered || birth->creator_thread_pseudo_query.returned ||
        birth->creator_thread_duplicate.source || birth->creator_thread_duplicate.subject ||
        birth->creator_thread_duplicate.result || birth->creator_thread_duplicate.last_error ||
        birth->creator_thread_duplicate.entered || birth->creator_thread_duplicate.returned) return 0;
    birth->attempted = 1;
    /* The actual inputs/backing are retained by this same owner. No fallback
     * constructor, inherited ambient handle list or postbirth assignment. */
    if (!birth->attribute_backing || !birth->attribute_capacity ||
        !birth->payload_path || !birth->command_line ||
        !birth->command_line_original || !birth->command_line_units ||
        !birth->inherited_child_endpoint || !birth->inherited_entry_gate) return 0;

    /* All descriptor observations are funded before any new Job/process.
     * Actual full allocation/alias ownership is the fixed constructor's
     * responsibility; these bounds reject concrete region overlap. */
    for(security_index=0;security_index<6;security_index++){
        sl_win_object_security *o=&birth->security[security_index];
        ULONG_PTR begin=(ULONG_PTR)o->backing,end;
        if(!begin||!o->capacity||o->attempted||begin>~(ULONG_PTR)0-o->capacity)return 0;
        end=begin+o->capacity;
        if(begin<(ULONG_PTR)birth+sizeof(*birth)&&(ULONG_PTR)birth<end)return 0;
        for(unsigned other=0;other<security_index;other++){
            sl_win_object_security *prior=&birth->security[other];ULONG_PTR previous=(ULONG_PTR)prior->backing;
            if(begin<previous+prior->capacity&&previous<end)return 0;
        }
    }
    for(unsigned token_index=0;token_index<2;token_index++){
        sl_win_token_observation *o=&birth->tokens[token_index];ULONG_PTR begin=(ULONG_PTR)o->backing,end;
        if(!begin||o->capacity<sizeof(TOKEN_ACCESS_INFORMATION)||o->attempted||begin>~(ULONG_PTR)0-o->capacity)return 0;
        end=begin+o->capacity;if(begin<(ULONG_PTR)birth+sizeof(*birth)&&(ULONG_PTR)birth<end)return 0;
        for(unsigned other=0;other<token_index;other++){sl_win_token_observation *p=&birth->tokens[other];ULONG_PTR previous=(ULONG_PTR)p->backing;if(begin<previous+p->capacity&&previous<end)return 0;}
        for(unsigned other=0;other<6;other++){sl_win_object_security *p=&birth->security[other];ULONG_PTR previous=(ULONG_PTR)p->backing;if(begin<previous+p->capacity&&previous<end)return 0;}
    }
    sl_win_enter(&birth->owner_process_query,"GetCurrentProcess.security-subject",birth);
    birth->owner_process=GetCurrentProcess();
    birth->owner_process_query.result=(ULONG_PTR)birth->owner_process;birth->owner_process_query.returned=1;
    if(!sl_win_observe_process_token(&birth->tokens[0],birth->owner_process)){birth->token_failure_original=&birth->tokens[0];return 0;}
    HANDLE existing[3]={birth->owner_process,birth->inherited_child_endpoint,birth->inherited_entry_gate};
    for(security_index=0;security_index<3;security_index++){
        if(!sl_win_observe_object_security(&birth->security[security_index],existing[security_index])){
            birth->security_failure_original=&birth->security[security_index];return 0;
        }
    }
    sl_win_enter(&birth->create_job,"CreateJobObjectW",birth);
    birth->job = CreateJobObjectW(NULL,NULL);
    error = GetLastError();
    birth->create_job.result = (ULONG_PTR)birth->job;
    birth->create_job.last_error = error;
    birth->create_job.returned = 1;
    if (!birth->job) return 0;
    if(!sl_win_observe_object_security(&birth->security[3],birth->job)){birth->security_failure_original=&birth->security[3];return 0;}

    /* No BREAKAWAY_OK or SILENT_BREAKAWAY_OK. Kill-on-close is deliberately
     * absent: losing this owner is lost custody, never normal family zero. */
    ZeroMemory(&birth->limits,sizeof(birth->limits));
    birth->limits.BasicLimitInformation.LimitFlags = 0;
    sl_win_enter(&birth->set_limits,"SetInformationJobObject",birth);
    result = SetInformationJobObject(birth->job,JobObjectExtendedLimitInformation,
                                    &birth->limits,sizeof(birth->limits));
    error = GetLastError();
    if (!sl_win_boolean_result(&birth->set_limits,result,error)) return 0;

    /* Selected original-owner operations preserve each genuine console/file/
     * pipe subject. Do not substitute a newly created pseudo console for it.
     * Native stream-operation and early hook readiness remain mandatory. */

    sl_win_enter(&birth->attribute_query,"InitializeProcThreadAttributeList.query",birth);
    result = InitializeProcThreadAttributeList(NULL,2,0,&birth->attribute_required);
    error = GetLastError();
    sl_win_boolean_result(&birth->attribute_query,result,error);
    if (result || error != ERROR_INSUFFICIENT_BUFFER ||
        !birth->attribute_required || birth->attribute_required > birth->attribute_capacity) return 0;
    birth->attributes = (LPPROC_THREAD_ATTRIBUTE_LIST)birth->attribute_backing;
    sl_win_enter(&birth->attribute_initialize,"InitializeProcThreadAttributeList",birth);
    result = InitializeProcThreadAttributeList(birth->attributes,2,0,&birth->attribute_required);
    error = GetLastError();
    if (!sl_win_boolean_result(&birth->attribute_initialize,result,error)) return 0;

    sl_win_enter(&birth->attribute_job,"UpdateProcThreadAttribute.JobList",birth);
    result = UpdateProcThreadAttribute(birth->attributes,0,
             PROC_THREAD_ATTRIBUTE_JOB_LIST,&birth->job,sizeof(birth->job),NULL,NULL);
    error = GetLastError();
    if (!sl_win_boolean_result(&birth->attribute_job,result,error)) return 0;
    birth->inherited_handles[0] = birth->inherited_child_endpoint;
    birth->inherited_handles[1] = birth->inherited_entry_gate;
    sl_win_enter(&birth->attribute_handles,"UpdateProcThreadAttribute.HandleList",birth);
    result = UpdateProcThreadAttribute(birth->attributes,0,
             PROC_THREAD_ATTRIBUTE_HANDLE_LIST,birth->inherited_handles,
             sizeof(birth->inherited_handles),NULL,NULL);
    error = GetLastError();
    if (!sl_win_boolean_result(&birth->attribute_handles,result,error)) return 0;
    ZeroMemory(&birth->startup,sizeof(birth->startup));
    birth->startup.StartupInfo.cb = sizeof(birth->startup);
    birth->startup.lpAttributeList = birth->attributes;
    /* CreateProcess may modify its command-line buffer. Keep the exact
     * preowned original and use the distinct preowned mutable backing. */
    for (unit=0;unit<birth->command_line_units;unit++)
        birth->command_line[unit]=birth->command_line_original[unit];
    if (birth->command_line[birth->command_line_units-1]!=L'\0') return 0;
    sl_win_enter(&birth->creator_thread_query,"GetCurrentThreadId.birth.creator",birth);
    birth->creator_thread_id=GetCurrentThreadId();error=GetLastError();
    birth->creator_thread_query.result=birth->creator_thread_id;
    birth->creator_thread_query.last_error=error;birth->creator_thread_query.returned=1;
    if(!birth->creator_thread_id)return 0;
    sl_win_enter(&birth->creator_thread_pseudo_query,"GetCurrentThread.birth.creator",birth);
    birth->creator_thread_pseudo=GetCurrentThread();error=GetLastError();
    birth->creator_thread_pseudo_query.result=(ULONG_PTR)birth->creator_thread_pseudo;
    birth->creator_thread_pseudo_query.last_error=error;birth->creator_thread_pseudo_query.returned=1;
    sl_win_enter(&birth->creator_thread_duplicate,"DuplicateHandle.birth.creator-thread",birth->creator_thread_pseudo);
    result=DuplicateHandle(birth->owner_process,birth->creator_thread_pseudo,birth->owner_process,
      &birth->held_creator_thread,THREAD_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,0);
    error=GetLastError();
    if(!sl_win_boolean_result(&birth->creator_thread_duplicate,result,error)||!birth->held_creator_thread)return 0;
    birth->creation_flags=EXTENDED_STARTUPINFO_PRESENT|CREATE_SUSPENDED|CREATE_UNICODE_ENVIRONMENT|DEBUG_ONLY_THIS_PROCESS;
    sl_win_enter(&birth->create_process,"CreateProcessW.JobList.Suspended.DebugOnly",birth);
    result = CreateProcessW(birth->payload_path,birth->command_line,NULL,NULL,TRUE,
             birth->creation_flags,
             birth->environment,birth->directory,&birth->startup.StartupInfo,
             &birth->process);
    error = GetLastError();
    if(!sl_win_boolean_result(&birth->create_process,result,error))return 0;
    /* The actual suspended newborn remains retained on either failure. */
    if(!sl_win_observe_object_security(&birth->security[4],birth->process.hProcess)){birth->security_failure_original=&birth->security[4];return 0;}
    if(!sl_win_observe_object_security(&birth->security[5],birth->process.hThread)){birth->security_failure_original=&birth->security[5];return 0;}
    if(!sl_win_observe_process_token(&birth->tokens[1],birth->process.hProcess)){birth->token_failure_original=&birth->tokens[1];return 0;}
    return 1;
}
#endif
