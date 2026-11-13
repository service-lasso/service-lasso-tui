#ifndef SERVICE_LASSO_CREATOR_ELF_LINUX_H
#define SERVICE_LASSO_CREATOR_ELF_LINUX_H
#include "creator_file_linux.h"
typedef struct sl_linux_elf_program {
 unsigned long type,flags,offset,address,physical,file_bytes,memory_bytes,alignment;
 unsigned long ordinal,interpreter_bytes;
 unsigned interpreted;
} sl_linux_elf_program;
typedef struct sl_linux_creator_elf {
 const sl_linux_creator_file *file_original;
 sl_linux_elf_program *programs;unsigned long capacity;
 const sl_linux_creator_file *actual_file;
 sl_linux_elf_program *actual_programs;unsigned long actual_capacity;
 unsigned long type,machine,entry,program_offset,program_count;
 unsigned long extended_count_offset,load_count,interpreter_count,first_interpreter;
 unsigned extended_count,interpreter_after_load,attempted,returned,complete;
 long result;
} sl_linux_creator_elf;
_Static_assert(sizeof(sl_linux_elf_program)==88,"retained ELF program row");
_Static_assert(sizeof(sl_linux_creator_elf)==152,"retained ELF source interpretation");
long sl_linux_interpret_creator_elf(sl_linux_creator_elf *original);
/* Readonly file representation, no mapped-image/provider/kernel admission.
 * Unknown program types and duplicate/order observations stay retained. */
#endif
