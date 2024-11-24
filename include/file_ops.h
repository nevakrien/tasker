#ifndef FILE_OPS_H
#define FILE_OPS_H

#include <stdio.h>
#include <stdbool.h>

struct CopyTask {
    const char *in_path;
    const char *out_path;
    volatile bool *global_err;
};

int copy_file(struct CopyTask* task);
int parallel_copy_file(const char **in, const char **out, size_t num_files);

#endif // FILE_OPS_H
