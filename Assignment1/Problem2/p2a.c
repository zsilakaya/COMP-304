
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>

#define BUFFER_SIZE 64
#define READ_END	0
#define WRITE_END	1

int main(void)
{
	char write_msg_0[BUFFER_SIZE];
	char read_msg_0[BUFFER_SIZE];

	pid_t pid;
	pid_t pid2;
/*	int fd[0] pipe A to B 
        int fd[1] pipe B to C
        int fd[2] pipe C to B
 	int fd[3] pipe B to A*/
	int fd[4][2];
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        assert(strftime(write_msg_0, sizeof(write_msg_0), "%c", tm));

	/* create the pipes */
	for(int i=0;i<4;i++){		
	if (pipe(fd[i]) == -1) {
		fprintf(stderr,"Pipe failed");
		return 1;
	}
	}


	/*fork a child process */
	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "Fork failed");
		return 1;
	}

	if (pid > 0) {  /* parent process of A */
		/* close the unused end of the pipe */
		close(fd[0][READ_END]);
		/* write to the pipe */
		write(fd[0][WRITE_END], write_msg_0, strlen(write_msg_0)+1); 

		/* close the write end of the pipe */
		close(fd[0][WRITE_END]);
		
		wait();

		close(fd[1][READ_END]);
                close(fd[1][WRITE_END]);
                close(fd[2][READ_END]);
                close(fd[2][WRITE_END]);
                close(fd[3][WRITE_END]);
               
	       	/* read from the pipe */
		char read_msg_3[BUFFER_SIZE];
                read(fd[3][READ_END], read_msg_3, BUFFER_SIZE);
                printf("Process A is reading the time which B sent: %s\n",read_msg_3);
		close(fd[3][READ_END]);
		kill(pid,SIGKILL);
		exit(0);

	}
	else { /* child process */
		/* close the unused end of the pipe */
		close(fd[0][WRITE_END]);
		
		/* read from the pipe */
		read(fd[0][READ_END], read_msg_0, BUFFER_SIZE);
		printf("Process B is reading the time which A sent  %s\n",read_msg_0);

		/* close the write end of the pipe */
		close(fd[0][READ_END]);

		sleep(3);

		pid2=fork();
		if(pid2<0){
			fprintf(stderr, "Fork failed");
               	        return 1;
        }
		if(pid2>0){//parent process of B
			/* close the unused end of the pipe */
			close(fd[1][READ_END]);

			char write_msg_1[BUFFER_SIZE];

			time_t t = time(NULL);
       			struct tm *tm = localtime(&t);
       			assert(strftime(write_msg_1, sizeof(write_msg_1), "%c", tm));
			
			write(fd[1][WRITE_END], write_msg_1, strlen(write_msg_1)+1);
			close(fd[1][WRITE_END]);
			wait();
			char read_msg_2[BUFFER_SIZE];
		
			close(fd[2][WRITE_END]);
                        read(fd[2][READ_END], read_msg_2, BUFFER_SIZE);
			int len=strlen(read_msg_2);
			read_msg_2[len]='\0';
                        printf("Process B is reading the time which C sent  %s\n",read_msg_2);
                        close(fd[2][READ_END]);
			sleep(3);
				
			/* write to the pipe */
			char write_msg_3[BUFFER_SIZE];
			time_t tim = time(NULL);
                        struct tm *time = localtime(&tim);
                        assert(strftime(write_msg_3, sizeof(write_msg_3), "%c", time));
			close(fd[3][READ_END]);
                        write(fd[3][WRITE_END], write_msg_3, strlen(write_msg_3)+1);
                        close(fd[3][WRITE_END]);
			kill(pid2,SIGKILL);


		}else{//child process of C
		        
			/* close the unused end of the pipe */
			close(fd[1][WRITE_END]);
			close(fd[0][READ_END]);
                        close(fd[0][WRITE_END]);
			
			/* read from the pipe */
			char read_msg_1[BUFFER_SIZE];
			read(fd[1][READ_END], read_msg_1, BUFFER_SIZE);
                       
		       	printf("Process C is reading the time which B sent  %s\n",read_msg_1);
         	        sleep(3);
                        wait();
			close(fd[1][READ_END]);
			
			/* write to the pipe */
			char write_msg_2[BUFFER_SIZE];
			time_t	t = time(NULL);
                	struct tm *tm = localtime(&t);
                        assert(strftime(write_msg_2, sizeof(write_msg_2), "%c", tm));

			/* close the unused end of the pipe */
			close(fd[2][READ_END]);
                        write(fd[2][WRITE_END], write_msg_2, strlen(write_msg_2)+1);
                        close(fd[2][WRITE_END]);
			close(fd[3][READ_END]);
                        close(fd[3][WRITE_END]);

	
}
}
return 0;

}
