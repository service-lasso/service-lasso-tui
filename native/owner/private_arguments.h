#ifndef SERVICE_LASSO_PRIVATE_ARGUMENTS_H
#define SERVICE_LASSO_PRIVATE_ARGUMENTS_H
/* Source-owned preexec private argv, retaining SAME original public aliases. */
typedef struct sl_private_arguments {
 int public_argc,custody_fd,actual_public_argc,actual_custody_fd;
 char **public_argv,**actual_public_argv,**private_argv,**actual_private_argv;
 unsigned long pointer_capacity,actual_pointer_capacity,private_count;
 unsigned long *lengths,*actual_lengths,length_capacity,actual_length_capacity;
 unsigned long string_byte_capacity,actual_string_byte_capacity,public_bytes;
 const unsigned long *security_quantity_original,*terminal_quantity_original,*representation_quantity_original;
 unsigned long actual_security_bytes,actual_terminal_calls,actual_representation_bytes;
 char descriptor[32],count[32],security[32],terminal[32],representation[32];
 unsigned long descriptor_length,count_length,security_length,terminal_length,representation_length;
 long result;unsigned attempted,returned;
} sl_private_arguments;
long sl_prepare_private_arguments(sl_private_arguments *arguments);
#endif