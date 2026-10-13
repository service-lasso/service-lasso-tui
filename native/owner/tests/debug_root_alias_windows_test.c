#ifdef _WIN32
/* Actual production preflight regression. No native operation is expected;
 * synthetic storage proves no provider, authentication, admission or fit. */
#include "../debug_root_windows.h"
#include <assert.h>
#include <string.h>
#define SL_WINDOWS_ROOT_RVA 4096u
#define SL_WINDOWS_ROOT_EXTENT SL_WINDOWS_NATIVE_ROOT_SOURCE_EXTENT
#define SL_WINDOWS_PAYLOAD_IMAGE_SIZE 8192u
#include "../debug_root_windows.c"

typedef union fixture_storage {
 sl_win_birth birth;
 sl_win_debug_event_observation event;
 sl_win_mapped_root_observation mapping;
 sl_win_root_write write;
 sl_win_debug_release release;
 sl_windows_native_root_storage source;
} fixture_storage;

int main(void){
 fixture_storage storage,snapshot;
 sl_win_debug_event_observation event={0};
 sl_win_mapped_root_observation mapping={0};
 sl_win_root_write write={0};
 sl_windows_native_root_storage source={0};
 sl_win_birth birth={0};

 /* Input pointer is permitted, but Attempted/call outputs overlap birth. */
 memset(&storage,0,sizeof(storage));storage.event.birth_original=&storage.birth;
 memcpy(&snapshot,&storage,sizeof(storage));
 assert(!sl_win_observe_debug_event(&storage.event));
 assert(!memcmp(&storage,&snapshot,sizeof(storage)));

 /* Distinct event, transitive native birth is still protected. */
 memset(&storage,0,sizeof(storage));event.actual_birth=&storage.birth;
 storage.mapping.event_original=&event;
 memcpy(&snapshot,&storage,sizeof(storage));
 assert(!sl_win_observe_mapped_root(&storage.mapping));
 assert(!memcmp(&storage,&snapshot,sizeof(storage)));

 /* Distinct mapping/source do not authorize overwriting their birth. */
 memset(&storage,0,sizeof(storage));event.actual_birth=&storage.birth;
 mapping.actual_event=&event;
 storage.write.before_original=&mapping;storage.write.source_original=&source;
 memcpy(&snapshot,&storage,sizeof(storage));
 assert(!sl_win_write_exact_root(&storage.write));
 assert(!memcmp(&storage,&snapshot,sizeof(storage)));

 /* Release output cannot consume its retained immutable root source. */
 memset(&storage,0,sizeof(storage));event.actual_birth=&birth;
 mapping.actual_event=&event;write.actual_before=&mapping;
 write.actual_source=&storage.source;storage.release.write_original=&write;
 memcpy(&snapshot,&storage,sizeof(storage));
 assert(!sl_win_release_debug_root(&storage.release));
 assert(!memcmp(&storage,&snapshot,sizeof(storage)));
 return 0;
}
#endif
