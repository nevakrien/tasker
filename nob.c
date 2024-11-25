// nob.c
#define NOB_IMPLEMENTATION
#include "nob.h"

// Define the path separator
#ifdef _WIN32
    #define PATH_SEP "\\"
#else
     #define PATH_SEP "/"
#endif

#define BIN "bin"PATH_SEP
#define SRC "src"PATH_SEP

int main(int argc, char **argv){
	NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra","-O2","--std=c11", "-Iinclude");
    size_t base_count = cmd.count;
	
	nob_cmd_append(&cmd,"-o",BIN"hello_world", SRC"hello_world.c");
    if (!nob_cmd_run_sync(cmd)) return 1;
    cmd.count = base_count;

    nob_cmd_append(&cmd,"-c", "-o",BIN"file_ops.o", SRC"file_ops.c");
    if (!nob_cmd_run_sync(cmd)) return 1;
    cmd.count = base_count;


    return 0;
}