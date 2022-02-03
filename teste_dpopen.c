#include <dlfcn.h> 
#include <errno.h>
#include <stdio.h>


int main(){
	
	void* teste = dlopen("./libMyLoopPass.so", RTLD_LAZY|RTLD_GLOBAL);
	
	if(!teste){
		printf("%s\n", dlerror());
	}
	

	return 0;
}	
