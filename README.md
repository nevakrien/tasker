# tasker (work in progress)
basic task runner and build system, made in C with the goal of being cross platform and not sucking.

this is not MPI, we are not aiming at a fast distributed system. 
the overhead per task is fairly high for the sake of code size and platform things.
however unlike MPI you dont need an entire freaking compiler. 
tasker is extremly simple to build relying only on the very basic APIs that are almost universal.

pretty much any C compiler should be able to build this.


# Architcture
we are using popen ( \_popen on windows) to run side processes and tinysocket to comunicate between them.
because of this we only depend on popen which is very standard 

the sockets are global for the most part and thats because having them set up that way allows us to clean them up.
this is not ideal but we are stuck with what the operating system gives us.
you will see 

```c
/*sockets*/
```
in any function signature that uses that global state.
in terms of actually building on windows I have managed to make make work.
ideally we would have just a crapton of build systems including cmake and visual studio but this we do in the polish state.

right now a lot of things are in header files and thats mostly because we are during rapid devlopment.
when structure irons out we will add .c files

# TODO
1. add tcp protocol to the test
2. finishup worker logic
3. move stuff out of headers into .c file