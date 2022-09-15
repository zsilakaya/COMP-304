#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <math.h>
#include "pthread_sleep_v2.c"
#include "queue.c"




FILE *logfd;//logfile
char initCurDir[100];
int question=1;
int n;
int totalQues;
int maxSpeak;
double probability;
int done=0;
int event=0;
int* status;
sem_t answered;


pthread_barrier_t barrierAnsGen;

pthread_mutex_t mutexQuestion;
pthread_mutex_t *mutexAnswer;
pthread_mutex_t mutexAnswered;
pthread_mutex_t mutexEvent;
pthread_mutex_t mutexEventHappens;

pthread_cond_t *condAnswer;
pthread_cond_t condAnswered;
pthread_cond_t condGenerate;
pthread_cond_t condEvent;
pthread_cond_t condEventHappens;


    
struct Queue* queue=NULL;
double rand_double();
const char* get_time ();
void* ask_question(void* arg);
void* generate_answer(void* arg);
void* answer_question();
void* waitEvent();

int main(int argc, char* argv[]) {
    
    //create log file in the current directory.
    getcwd(initCurDir,sizeof(initCurDir));//stores the path of the main.c in the global array.
    strcat(initCurDir,"/File.log");
    logfd = fopen (initCurDir, "w");
    
    queue=createQueue(1000);
    
    //storing inputs 
    n=atoi(argv[2]);
    probability=atof(argv[4]);
    totalQues=atoi(argv[6]);
    maxSpeak=atoi(argv[8]);
    double b=atof(argv[10]);
    
    //creating condAnswer and mutexAnswer with malloc since they should be in size n.
    status=malloc(n*sizeof(int));
    condAnswer=(pthread_cond_t*) malloc(n * sizeof(pthread_cond_t));
    mutexAnswer=(pthread_mutex_t*) malloc(n * sizeof(pthread_mutex_t));

    //printf("n:%d p:%f q:%d t:%d b:%f\n",n,p,q,t,b);
    //totalQues=q;
    //maxSpeak=t;
    //probability=p;
    
    //hardcoded parameters
    
    /*int n=4;
    double p=0.75;
    int q=5;
    int t=3;
    double b=0.05;
    srand(time(0));*/
    
    //creating threads.
    pthread_t breakingEvent;//waits for breaking news event 
    pthread_t moderator;//moderator thread
    pthread_t comm[n];//commentators threads
    
    //initializing the mutexes,barrier, condition variables and semaphores.    
    sem_init(&answered, 0, 1);   
        
    pthread_barrier_init(&barrierAnsGen, NULL, n);
    
    
    pthread_mutex_init(&mutexQuestion, NULL);   
    pthread_mutex_init(mutexAnswer, NULL); 
    pthread_mutex_init(&mutexAnswered, NULL); 
    pthread_mutex_init(&mutexEvent, NULL); 
    pthread_mutex_init(&mutexEventHappens, NULL); 
    
   
    pthread_cond_init(condAnswer ,NULL);
    pthread_cond_init(&condAnswered, NULL);
    pthread_cond_init(&condGenerate, NULL);
    pthread_cond_init(&condEvent, NULL);
    pthread_cond_init(&condEventHappens, NULL);
    
    for(int i=0;i<=(n+1);i++){
    	if((i!=(n))&&(i!=(n+1))){
    		int* id=malloc(sizeof(int));
        	*id=i;
        	if (pthread_create(&comm[i], NULL, &generate_answer, id) != 0) {	
    		perror("Failed to create thread");
        }
        	
    		
        }else if(i==n){
	       if (pthread_create(&breakingEvent, NULL,&waitEvent , NULL) != 0) {	
    		perror("Failed to create thread");
        } 
        }else{
        	if (pthread_create(&moderator, NULL, &ask_question, NULL) != 0) {
    		perror("Failed to create thread");
    		} 
    }
    }
    
    while(!done){
    	double random=rand_double();
    	pthread_mutex_lock(&mutexEvent);
    	//printf("%s |B ",get_time());
    	//printf("%f \n",random);
    	if(random<=b){
    		event=1;
    		pthread_mutex_unlock(&mutexEvent);
    		pthread_cond_signal(&condEvent);
    		pthread_sleep(1);
    		
    	}else{
	    	pthread_mutex_unlock(&mutexEvent);
	    	pthread_sleep(1);
    	} 	
    }
    
    //if  program is finished cancel all the threads.
    if(done){
	pthread_cancel(breakingEvent); 
	for(int i=0;i<n;i++){
		pthread_cancel(comm[i]); 
	}
	pthread_cancel(moderator);
    }
    
 
   for(int i=0;i<=n+1;i++){
   	if(i<n){	
   		
    		if(pthread_join(comm[i], NULL)){
	    		
	    		perror("Failed to join the thread\n");
	    	}
        }else if(i==n){
        	
        	if (pthread_join(breakingEvent, NULL) != 0) {
        		
    			perror("Failed to join the thread");	
    		} 
        }else{
        	
        	if (pthread_join(moderator, NULL) != 0) {
        		
    			perror("Failed to join the threads");
        	}
       }
        
   }
    // free allocated memory and destroy mutexes,condition variables and barrier.
    free(status);
    free(condAnswer);
    free(mutexAnswer);
    
    pthread_barrier_destroy(&barrierAnsGen);
    
    pthread_mutex_destroy(&mutexQuestion);
    pthread_mutex_destroy(mutexAnswer);
    pthread_mutex_destroy(&mutexAnswered);
    pthread_mutex_destroy(&mutexEvent);
    pthread_mutex_destroy(&mutexEventHappens); 
    
    pthread_cond_destroy(condAnswer);
    pthread_cond_destroy(&condAnswered);
    pthread_cond_destroy(&condGenerate);
    pthread_cond_destroy(&condEvent);
    pthread_cond_destroy(&condEventHappens);
    return 0;
}
void* ask_question(void* arg) {
	
        while(question<=totalQues){
               pthread_mutex_lock(&mutexQuestion);    
               fprintf(logfd,"%s |Moderator asks question %d\n",get_time(), question);  
	       printf("%s |Moderator asks question %d\n",get_time(), question);
	       pthread_mutex_unlock(&mutexQuestion);    
	       pthread_cond_broadcast(&condGenerate);
	       pthread_sleep(1);
	       while(queue->size!=0){
	       	while(event==1);
	       	int id=front(queue);//takes front element from the queue	  
	     		pthread_mutex_lock(&mutexAnswer[id]);
			pthread_mutex_unlock(&mutexAnswer[id]);
			pthread_cond_signal(&condAnswer[id]);		
			sem_wait(&answered);
			
	       }
	       question++;		       
     }
     //printf("%s |Moderator done\n",get_time());
     done=1;	
     pthread_exit(0);
    	
	
}
void* generate_answer(void* arg){

	int id=*(int*) arg;
	while(question<=totalQues){
		pthread_mutex_lock(&mutexQuestion);
		pthread_cond_wait(&condGenerate, &mutexQuestion);
		double random=rand_double();
		if(random<=probability){
			enqueue(queue,id);
			status[id]=1;//generates an answer
			fprintf(logfd,"%s |Commentator #%d generates an answer,position in queue: %d.\n",get_time(),id,queue->size);  
			printf("%s |Commentator #%d generates an answer, position in queue: %d\n",get_time(),id,queue->size);	
		}
		else{
			//printf("%s |Commentator #%d COULD NOT generates an answer",get_time(),id);
			//printf("with prob:%f.\n",random);	
		}	
		pthread_mutex_unlock(&mutexQuestion);
		pthread_barrier_wait(&barrierAnsGen);//wait all threads to generate their answers.
		
	
		if(status[id]==1){
			
			pthread_mutex_lock(&mutexAnswer[id]);
			pthread_cond_wait(&condAnswer[id],&mutexAnswer[id]);			
			answer_question();
			status[id]=0;			
			pthread_mutex_unlock(&mutexAnswer[id]);			
			sem_post(&answered);//question is answered signal the moderator		
		}
		
		if(question==totalQues){
			while(!done);
			pthread_exit(0);
			printf("thread %d is done.\n",id);
		}
	}
	
}

void* answer_question() { 

	double t_speak=rand_double()* maxSpeak;
        struct timeval tp;
        struct timespec timetoexpire;
        gettimeofday(&tp, NULL);
        long new_nsec = tp.tv_usec * 1000 + (t_speak - (long)t_speak) * 1e9;
        timetoexpire.tv_sec = tp.tv_sec + (long)t_speak + (new_nsec / (long)1e9);
        timetoexpire.tv_nsec = new_nsec % (long)1e9;
	int comm=front(queue);	
	fprintf(logfd,"%s |Commentator #%d's turn to speak for ",get_time(),comm);
	fprintf(logfd,"%.3f seconds.\n",t_speak);    
	printf("%s |Commentator #%d's turn to speak for ",get_time(),comm);
	printf("%.3f seconds.\n",t_speak);
	dequeue(queue);
	pthread_mutex_lock(&mutexEventHappens);
	int rc = pthread_cond_timedwait(&condEventHappens, &mutexEventHappens, &timetoexpire);
	pthread_mutex_unlock(&mutexEventHappens);  
	
	if(rc==0){
		fprintf(logfd,"%s |Commentator #%d cut short due to breaking event.\n",get_time(),comm);
		printf("%s |Commentator #%d cut short due to breaking event.\n",get_time(),comm);
		pthread_sleep(5);
		
	}else{	
		fprintf(logfd,"%s |Commentator #%d finished speaking.\n",get_time(),comm);
		printf("%s |Commentator #%d finished speaking.\n",get_time(),comm);
	}
	
	
}
void* waitEvent(){
//function of the Breaking Event thread.
	while(1){
		pthread_mutex_lock(&mutexEvent);
		pthread_cond_wait(&condEvent, &mutexEvent);
		fprintf(logfd,"%s |Breaking news!\n",get_time());
		printf("%s |Breaking news!\n",get_time());
		pthread_mutex_unlock(&mutexEventHappens);
		pthread_cond_signal(&condEventHappens);
		pthread_sleep(5);
		event=0;
		//pthread_mutex_unlock(&mutexEventHappens);
		
		fprintf(logfd,"%s |Breaking news ends.\n",get_time());
		printf("%s |Breaking news ends.\n",get_time());
		pthread_mutex_unlock(&mutexEvent);
	}
}


double rand_double(){
	//returns a real number between 0-1.
	return ((double) rand())/(double)RAND_MAX;
}

const char* get_time () {
	char time[30];
	char buffer[26];
	char millis[4];
	int millisec;
	struct tm* tm_info;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	millisec = (tv.tv_usec/1000.0); // Round to nearest millisec
	if (millisec>=1000) { // Allow for rounding up to nearest second
		millisec -=1000;
		tv.tv_sec++;
	}
	tm_info = localtime(&tv.tv_sec);
	strftime(buffer, 26, "%H:%M:%S", tm_info);
	snprintf(time, 30, "%s.%03d",buffer,millisec);
	int len=strlen(buffer);
	buffer[len]='\0';
	const char* ret=time;
	return ret;
}

    
    
    
    
    
    
    
