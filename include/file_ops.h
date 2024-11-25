#ifndef FILE_OPS_H
#define FILE_OPS_H

#include <stdio.h>
#include <stdbool.h>

typedef struct
{
    const char *in_path;
    const char *out_path;
    volatile bool *global_err;
} CopyTask;

int copy_file(CopyTask *task);
int parallel_copy_file(const char **in, const char **out, size_t num_files);

#endif // FILE_OPS_H
