#include "System2.h"
#include <stdint.h>

static inline System2CommandInfo make_info(){
	System2CommandInfo info; //= {0};
	memset(&info, 0, sizeof(System2CommandInfo));
	info.RedirectInput = true;
    info.RedirectOutput = true;
    return info;
}

static inline void abord_on_fail(SYSTEM2_RESULT result){
	if(result!=SYSTEM2_RESULT_SUCCESS){
		printf("system2 failed %d\n",result);
		fflush(stdout);
		exit(1);
	}
}

int main()  {
	printf("started\n");

	System2CommandInfo info = make_info();
	const char* args[] = {"gcc","-g3","-std=c99","-Wall","src/hello_world.c","-obin/hello_world"}; 
	SYSTEM2_RESULT result=System2RunSubprocess("gcc",args,sizeof(args),&info);
	abord_on_fail(result);

	int return_code;
	result = System2GetCommandReturnValueSync(&info,&return_code,true);
	if(return_code){
		printf("got return code %d\n",return_code);
	}
	abord_on_fail(result);

	uint32_t bytes_read = 0;
	char buffer[BUFSIZ] = {0};
	do {
		result =  System2ReadFromOutput(&info,buffer,BUFSIZ,&bytes_read);
		for (uint32_t i = 0; i < bytes_read; i++) {
	        putchar(buffer[i]);
	    }
			
	} while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);

	abord_on_fail(result);

	
	abord_on_fail(System2CleanupCommand(&info));

	printf("yay\n");
	return 0;
	
}