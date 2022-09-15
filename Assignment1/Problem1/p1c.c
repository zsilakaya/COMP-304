#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>


int main(){

if(fork()==0){
  printf("I am the child %d\n",getpid());
  exit(0);

}else{
  printf("Zombie process created.\n");
  sleep(5);
  printf("Zombie process killed.\n");
  wait();
}
}


