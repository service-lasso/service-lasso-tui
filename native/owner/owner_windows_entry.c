#ifdef _WIN32
#include "owner_windows.h"
/* Own the actual process standard-handle observations and independent held
 * views before internal channel/console/process creation. No SetStdHandle or
 * global console mode/disposition change occurs. Null is an observed absent
 * standard handle; INVALID_HANDLE_VALUE is the actual failed native result. */
int sl_win_prepare_public_streams(sl_win_entry *entry) {
    unsigned index;
    DWORD error;
    BOOL result;
    if (!entry || entry->public_streams_attempted || entry->birth.attempted) return 0;
    for(index=0;index<3;index++){
        sl_win_public_stream *s=&entry->public_streams[index];
        if(s->selector||s->original||s->held||s->observed_absent||s->observed_invalid||s->duplicate_failed||
           s->get_handle.source||s->get_handle.subject||s->get_handle.result||s->get_handle.last_error||s->get_handle.entered||s->get_handle.returned||
           s->duplicate.source||s->duplicate.subject||s->duplicate.result||s->duplicate.last_error||s->duplicate.entered||s->duplicate.returned)return 0;
    }
    entry->public_streams_attempted=1;
    entry->current_process_query.source="public.entry.GetCurrentProcess.pseudo";
    entry->current_process_query.subject=entry;entry->current_process_query.entered=1;
    entry->current_process=GetCurrentProcess();
    entry->current_process_query.result=(ULONG_PTR)entry->current_process;
    entry->current_process_query.returned=1;
    entry->public_streams[0].selector=STD_INPUT_HANDLE;
    entry->public_streams[1].selector=STD_OUTPUT_HANDLE;
    entry->public_streams[2].selector=STD_ERROR_HANDLE;
    for (index=0;index<3;index++) {
        sl_win_public_stream *stream=&entry->public_streams[index];
        stream->get_handle.source="public.entry.GetStdHandle";
        stream->get_handle.subject=stream;
        stream->get_handle.entered=1;
        stream->original=GetStdHandle(stream->selector);
        error=GetLastError();
        stream->get_handle.result=(ULONG_PTR)stream->original;
        stream->get_handle.last_error=error;
        stream->get_handle.returned=1;
        if (stream->original==INVALID_HANDLE_VALUE) stream->observed_invalid=1;
        if (!stream->original) stream->observed_absent=1;
    }
    for (index=0;index<3;index++) {
        sl_win_public_stream *stream=&entry->public_streams[index];
        if (stream->observed_absent||stream->observed_invalid) continue;
        stream->duplicate.source="public.entry.DuplicateHandle.standard-view";
        stream->duplicate.subject=stream;
        stream->duplicate.entered=1;
        /* Current-process pseudo handles are intrinsic call arguments. This
         * does not claim a retained independently authenticated process handle.
         * Output held view is never inheritable; child HANDLE_LIST is separate. */
        result=DuplicateHandle(entry->current_process,stream->original,
                               entry->current_process,&stream->held,0,FALSE,DUPLICATE_SAME_ACCESS);
        error=GetLastError();
        stream->duplicate.result=(ULONG_PTR)result;
        stream->duplicate.last_error=error;
        stream->duplicate.returned=1;
        if (!result) stream->duplicate_failed=1;
    }
    /* Prepared means all original observations are captured. It does not mean
     * every stream is usable. Help/config may not borrow a missing stream;
     * each later native operation must consume its actual stream occurrence.
     * Failed duplicate outputs stay raw and are never adopted or closed. */
    entry->public_streams_prepared=1;
    return 1;
}
int sl_win_prepare_entry_birth(sl_win_entry *entry) {
    if (!entry || entry->birth.attempted) return 0;
    if (!entry->public_streams_prepared && !sl_win_prepare_public_streams(entry)) return 0;
    /* Exact endpoint/gate/console inputs and all source/ABI/rights/loaded/fit
     * admission remain required before this call; no pseudo handle or standard
     * duplicate supplies that authority. No Resume/Close occurs in this path. */
    return sl_win_prepare_birth(&entry->birth);
}
#endif
