# tasker (work in progress)
basic task runner and build system, made in C with the goal of being cross platform and not sucking.

this is not MPI, we are not aiming at a fast distributed system. 
the overhead per task is fairly high for the sake of code size and platform things.
however unlike MPI you dont need an entire freaking compiler. 
tasker is extremly simple to build relying only on the very basic APIs that are almost universal.

pretty much any C compiler should be able to build this.


# Architcture
every task has 2 associated temp files

1. the tasks printble output.
2. the tasks status.

a task is done if and only if its status file is there. 
if thats the case an intger would be there to indicate the error code.

### Why this?
we are using popen as a way to do multiprocessing and multithreading because its fairly standard on all platforms (including windows). for comunicating between threads we are using the file system as thats the only real way to run things in a non blocking way.

note that on windows there is some weirdnes with shared file access but the code needs to be made for the case a file has yet to be made anyway so this should not break correctnes.