#include <sys/types.h> 
#include <stdio.h> 
#include <unistd.h> 

int main (){
int level=0;
printf("Base Process ID: %d, level: %d\n",getpid(),level);
for(int i=0; i<4;i++){
pid_t pid=fork();
if(pid==0){
level++;
printf("PID %d,PARENT ID: %d,level: %d\n",getpid(),getppid(),level);
}
else{
wait();
}
}


return 0;
}

