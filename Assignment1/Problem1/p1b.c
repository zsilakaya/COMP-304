#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(){

pid_t pid;
pid=fork();

if(pid==0){

char *cmd[] = {"/bin/ps", "f", 0};         		
execv(cmd[0], cmd);

}else{
	
wait();
printf("Child finished execution.\n");

}
}
