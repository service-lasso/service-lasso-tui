#if defined(__linux__) && defined(__x86_64__)
#include "terminal_linux.h"
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <fcntl.h>
_Static_assert(sizeof(sl_linux_window_size)==8,"Linux window ioctl output ABI");
static int sl_terminal_held_fd(const sl_linux_public_stream *stream) {
    if (!stream || !atomic_load_explicit(&stream->original_flags.returned,memory_order_acquire)) return -1;
    if (stream->observed_absent) return -1;
    if (!atomic_load_explicit(&stream->duplicate.returned,memory_order_acquire) ||
        stream->duplicate.result<3 || stream->duplicate.result>2147483647L) return -1;
    return (int)stream->duplicate.result;
}
long sl_linux_observe_public_terminal(sl_linux_terminal_views *terminal) {
    int input,output;
    long result;
    if (!terminal || terminal->attempted || !terminal->input_original || !terminal->output_original) return -1;
    terminal->attempted=1;terminal->active_mode=0;terminal->selected_input_fd=-1;terminal->fallback_raw_fd=-1;
    terminal->fallback_open.result=0;terminal->fallback_query.result=0;
    atomic_init(&terminal->input_query.entered,0);atomic_init(&terminal->input_query.returned,0);
    atomic_init(&terminal->output_query.entered,0);atomic_init(&terminal->output_query.returned,0);
    atomic_init(&terminal->fallback_open.entered,0);atomic_init(&terminal->fallback_open.returned,0);
    atomic_init(&terminal->fallback_query.entered,0);atomic_init(&terminal->fallback_query.returned,0);
    input=sl_terminal_held_fd(terminal->input_original);
    output=sl_terminal_held_fd(terminal->output_original);
    result=sl_linux_call6(&terminal->input_query,"public.terminal.input.tcgets",SYS_ioctl,
                         input,TCGETS,(long)&terminal->input_state,0,0,0);
    terminal->input_is_terminal=result==0;
    if (terminal->input_is_terminal) terminal->selected_input_fd=input;
    result=sl_linux_call6(&terminal->output_query,"public.terminal.output.tcgets",SYS_ioctl,
                         output,TCGETS,(long)&terminal->output_state,0,0,0);
    terminal->output_is_terminal=result==0;
    /* Capability observation alone cannot reject flag/config-only paths.
     * Fallback/window operations belong to the genuine later Program.Run
     * activation request, not pre-Go construction. Probe failures stay held. */
    return 0;
}
long sl_linux_open_public_input_fallback(sl_linux_terminal_views *terminal) {
    long result;
    if (!terminal || !terminal->attempted || terminal->fallback_attempted) return -1;
    terminal->fallback_attempted=1;
    if (terminal->input_is_terminal) return 0;
    /* Match actual BubbleTea defaultInput fallback. This is the original
     * public session's /dev/tty, before child setsid, never a fabricated private
     * PTY capability. Original probe failures stay held even on fallback. */
    result=sl_linux_call6(&terminal->fallback_open,"public.terminal.default-input.open-tty",SYS_openat,
                         AT_FDCWD,(long)"/dev/tty",O_RDONLY|O_CLOEXEC,0,0,0);
    if (result<0 || result>2147483647L) return result<0?result:-1;
    terminal->fallback_raw_fd=(int)result;
    result=sl_linux_lift_internal_fd(&terminal->fallback_lift,terminal->fallback_raw_fd);
    if (result<0) return result;
    terminal->selected_input_fd=(int)result;
    return sl_linux_call6(&terminal->fallback_query,"public.terminal.default-input.tcgets",SYS_ioctl,
                         terminal->selected_input_fd,TCGETS,(long)&terminal->fallback_state,0,0,0);
}
long sl_linux_make_public_raw(sl_linux_mode_episode *episode) {
    sl_linux_terminal_views *terminal;
    long result;
    if (!episode || episode->attempted || !episode->terminal_original) return -1;
    episode->attempted=1;terminal=episode->terminal_original;
    if (terminal->active_mode || terminal->selected_input_fd<3) return -1;
    episode->input_fd=terminal->selected_input_fd;
    result=sl_linux_call6(&episode->query,"public.terminal.raw.tcgets",SYS_ioctl,
                         episode->input_fd,TCGETS,(long)&episode->original_state,0,0,0);
    if (result<0) return result;
    episode->raw_state=episode->original_state;
    episode->raw_state.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
    episode->raw_state.c_oflag &= ~OPOST;
    episode->raw_state.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    episode->raw_state.c_cflag &= ~(CSIZE|PARENB);
    episode->raw_state.c_cflag |= CS8;
    episode->raw_state.c_cc[VMIN]=1;episode->raw_state.c_cc[VTIME]=0;
    /* Hold the actual original even if setter returns an uncertain failure. */
    terminal->active_mode=episode;
    return sl_linux_call6(&episode->change,"public.terminal.raw.tcsets",SYS_ioctl,
                         episode->input_fd,TCSETS,(long)&episode->raw_state,0,0,0);
}
long sl_linux_restore_public_mode(sl_linux_terminal_views *terminal,sl_linux_mode_restore *restoration) {
    sl_linux_mode_episode *episode;
    long result;
    if (!terminal || !restoration || restoration->attempted) return -1;
    restoration->attempted=1;episode=terminal->active_mode;restoration->mode_original=episode;
    if (!episode || episode->restore_original || !atomic_load_explicit(&episode->change.returned,memory_order_acquire) || episode->change.result!=0) return -1;
    episode->restore_original=restoration;
    result=sl_linux_call6(&restoration->restore,"public.terminal.restore.tcsets",SYS_ioctl,
                         episode->input_fd,TCSETS,(long)&episode->original_state,0,0,0);
    if (result==0) terminal->active_mode=0;
    /* A failed restoration cannot be retried by a fresh request. */
    else terminal->active_mode=episode;
    return result;
}
long sl_linux_observe_public_window(sl_linux_window_observation *observation) {
    long result;
    int output;
    if (!observation || observation->attempted || !observation->terminal_original) return -1;
    observation->attempted=1;
    if (!observation->terminal_original->output_is_terminal || observation->pty_fd<3) return -1;
    output=sl_terminal_held_fd(observation->terminal_original->output_original);
    atomic_init(&observation->query.entered,0);atomic_init(&observation->query.returned,0);
    atomic_init(&observation->copy.entered,0);atomic_init(&observation->copy.returned,0);
    result=sl_linux_call6(&observation->query,"public.terminal.output.window",SYS_ioctl,
                         output,TIOCGWINSZ,(long)&observation->window,0,0,0);
    if (result<0) return result;
    return sl_linux_call6(&observation->copy,"public.terminal.pty.window.copy",SYS_ioctl,
                         observation->pty_fd,TIOCSWINSZ,(long)&observation->window,0,0,0);
}
#endif
