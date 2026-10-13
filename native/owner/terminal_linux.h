#ifndef SERVICE_LASSO_TERMINAL_LINUX_H
#define SERVICE_LASSO_TERMINAL_LINUX_H
#include "owner_linux.h"
#include <asm/termbits.h>
typedef struct sl_linux_window_size { uint16_t rows,columns,x_pixels,y_pixels; } sl_linux_window_size;
struct sl_linux_mode_episode;
typedef struct sl_linux_terminal_views {
    const sl_linux_public_stream *input_original,*output_original;
    sl_linux_call input_query,output_query,fallback_open,fallback_query;
    sl_linux_fd_lift fallback_lift;
    struct termios input_state,output_state,fallback_state;
    struct sl_linux_mode_episode *active_mode;
    int selected_input_fd,fallback_raw_fd;
    unsigned attempted,fallback_attempted,input_is_terminal,output_is_terminal;
} sl_linux_terminal_views;
typedef struct sl_linux_window_observation {
    sl_linux_terminal_views *terminal_original;
    sl_linux_call query,copy;
    sl_linux_window_size window;
    int pty_fd;
    unsigned attempted;
} sl_linux_window_observation;
typedef struct sl_linux_mode_episode {
    sl_linux_terminal_views *terminal_original;
    sl_linux_call query,change;
    struct sl_linux_mode_restore *restore_original;
    struct sl_linux_input_worker *input_original;
    struct termios original_state,raw_state;
    int input_fd;
    unsigned attempted;
} sl_linux_mode_episode;
typedef struct sl_linux_mode_restore {
    sl_linux_mode_episode *mode_original;
    sl_linux_call restore;
    unsigned attempted;
} sl_linux_mode_restore;
long sl_linux_make_public_raw(sl_linux_mode_episode *episode);
long sl_linux_restore_public_mode(sl_linux_terminal_views *terminal,sl_linux_mode_restore *restoration);
long sl_linux_observe_public_terminal(sl_linux_terminal_views *terminal);
long sl_linux_open_public_input_fallback(sl_linux_terminal_views *terminal);
long sl_linux_observe_public_window(sl_linux_window_observation *observation);
#endif
