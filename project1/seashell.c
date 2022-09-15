//authors: Ece Güz (69002) , Zeynep Sıla Kaya (69101)

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>            //termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <strings.h>
#include <ctype.h>
void sdDelete(char* name);
const char * sysname = "seashell";
#define MAX_LINE 120
char initCurDir[MAX_LINE];// path of the directory which stores seashell.c
FILE *sdFile;//FILE which used in shortdir methods.
enum return_codes {
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};
struct command_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3]; // in/out redirection
	struct command_t *next; // for piping
};
/**
 * Prints a command struct
 * @param struct command_t *
 */
//prototypes 
void sdJumpName(char* name);
int process_command(struct command_t *command);

void print_command(struct command_t * command)
{
	int i=0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background?"yes":"no");
	printf("\tNeeds Auto-complete: %s\n", command->auto_complete?"yes":"no");
	printf("\tRedirects:\n");
	for (i=0;i<3;i++)
		printf("\t\t%d: %s\n", i, command->redirects[i]?command->redirects[i]:"N/A");
	printf("\tArguments (%d):\n", command->arg_count);
	for (i=0;i<command->arg_count;++i)
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	if (command->next)
	{
		printf("\tPiped to:\n");
		print_command(command->next);
	}


}
/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command)
{
	if (command->arg_count)
	{
		for (int i=0; i<command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}
	for (int i=0;i<3;++i)
		if (command->redirects[i])
			free(command->redirects[i]);
	if (command->next)
	{
		free_command(command->next);
		command->next=NULL;
	}
	free(command->name);
	free(command);
	return 0;
}
/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt()
{
	char cwd[1024], hostname[1024];
    gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}
/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command)
{
	const char *splitters=" \t"; // split at whitespace
	int index, len;
	len=strlen(buf);
	while (len>0 && strchr(splitters, buf[0])!=NULL) // trim left whitespace
	{
		buf++;
		len--;
	}
	while (len>0 && strchr(splitters, buf[len-1])!=NULL)
		buf[--len]=0; // trim right whitespace

	if (len>0 && buf[len-1]=='?') // auto-complete
		command->auto_complete=true;
	if (len>0 && buf[len-1]=='&') // background
		command->background=true;

	char *pch = strtok(buf, splitters);
	command->name=(char *)malloc(strlen(pch)+1);
	if (pch==NULL)
		command->name[0]=0;
	else
		strcpy(command->name, pch);

	command->args=(char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index=0;
	char temp_buf[1024], *arg;
	while (1)
	{
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch) break;
		arg=temp_buf;
		strcpy(arg, pch);
		len=strlen(arg);

		if (len==0) continue; // empty arg, go for next
		while (len>0 && strchr(splitters, arg[0])!=NULL) // trim left whitespace
		{
			arg++;
			len--;
		}
		while (len>0 && strchr(splitters, arg[len-1])!=NULL) arg[--len]=0; // trim right whitespace
		if (len==0) continue; // empty arg, go for next

		// piping to another command
		if (strcmp(arg, "|")==0)
		{
			struct command_t *c=malloc(sizeof(struct command_t));
			int l=strlen(pch);
			pch[l]=splitters[0]; // restore strtok termination
			index=1;
			while (pch[index]==' ' || pch[index]=='\t') index++; // skip whitespaces

			parse_command(pch+index, c);
			pch[l]=0; // put back strtok termination
			command->next=c;
			continue;
		}

		// background process
		if (strcmp(arg, "&")==0)
			continue; // handled before

		// handle input redirection
		redirect_index=-1;
		if (arg[0]=='<')
			redirect_index=0;
		if (arg[0]=='>')
		{
			if (len>1 && arg[1]=='>')
			{
				redirect_index=2;
				arg++;
				len--;
			}
			else redirect_index=1;
		}
		if (redirect_index != -1)
		{
			command->redirects[redirect_index]=malloc(len);
			strcpy(command->redirects[redirect_index], arg+1);
			continue;
		}

		// normal arguments
		if (len>2 && ((arg[0]=='"' && arg[len-1]=='"')
			|| (arg[0]=='\'' && arg[len-1]=='\''))) // quote wrapped arg
		{
			arg[--len]=0;
			arg++;
		}
		command->args=(char **)realloc(command->args, sizeof(char *)*(arg_index+1));
		command->args[arg_index]=(char *)malloc(len+1);
		strcpy(command->args[arg_index++], arg);
	}
	command->arg_count=arg_index;
	return 0;
}
void prompt_backspace()
{
	putchar(8); // go back 1
	putchar(' '); // write empty over
	putchar(8); // go back 1 again
}
/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command)
{
	int index=0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

    // tcgetattr gets the parameters of the current terminal
    // STDIN_FILENO will tell tcgetattr that it should write the settings
    // of stdin to oldt
    static struct termios backup_termios, new_termios;
    tcgetattr(STDIN_FILENO, &backup_termios);
    new_termios = backup_termios;
    // ICANON normally takes care that one line at a time will be processed
    // that means it will return if it sees a "\n" or an EOF or an EOL
    new_termios.c_lflag &= ~(ICANON | ECHO); // Also disable automatic echo. We manually echo each char.
    // Those new settings will be set to STDIN
    // TCSANOW tells tcsetattr to change attributes immediately.
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);


    //FIXME: backspace is applied before printing chars
	show_prompt();
	int multicode_state=0;
	buf[0]=0;
  	while (1)
  	{
		c=getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		if (c==9) // handle tab
		{
			buf[index++]='?'; // autocomplete
			break;
		}

		if (c==127) // handle backspace
		{
			if (index>0)
			{
				prompt_backspace();
				index--;
			}
			continue;
		}
		if (c==27 && multicode_state==0) // handle multi-code keys
		{
			multicode_state=1;
			continue;
		}
		if (c==91 && multicode_state==1)
		{
			multicode_state=2;
			continue;
		}
		if (c==65 && multicode_state==2) // up arrow
		{
			int i;
			while (index>0)
			{
				prompt_backspace();
				index--;
			}
			for (i=0;oldbuf[i];++i)
			{
				putchar(oldbuf[i]);
				buf[i]=oldbuf[i];
			}
			index=i;
			continue;
		}
		else
			multicode_state=0;

		putchar(c); // echo the character
		buf[index++]=c;
		if (index>=sizeof(buf)-1) break;
		if (c=='\n') // enter key
			break;
		if (c==4) // Ctrl+D
			return EXIT;
  	}
  	if (index>0 && buf[index-1]=='\n') // trim newline from the end
  		index--;
  	buf[index++]=0; // null terminate string

  	strcpy(oldbuf, buf);

  	parse_command(buf, command);

  	// print_command(command); // DEBUG: uncomment for debugging

    // restore the old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
  	return SUCCESS;
}
void red () {
  printf("\033[1;31m");
}

void green () {
  printf("\033[0;32m");
}

void blue () {
  printf("\033[0;34m");
}

void reset () {
  printf("\033[0m");
}

void highlight(const char* word ,char* color , char* fileName){
FILE *fp;
fp=fopen(fileName,"r");

char *sentences[MAX_LINE];//Stores the lines(sentences) in the file.
char *sentencescpy[MAX_LINE];// copy of the sentences array
char *wantedSen[MAX_LINE];//Stores the lines(sentences) which contanins desired word.
int BUFSIZE=1000;
if (fp == NULL)
   {
       printf("Error opening file.\n");
       exit(0);
   }
//initializes the array  
int i=0;
int wanNum=0;
sentences[i] = malloc(BUFSIZE);
 while (fgets(sentences[i], BUFSIZE, fp)) {
       i++;
       sentences[i] = malloc(BUFSIZE);
}
//copying the array
 for (int k=0;k<i;k++){
	 sentencescpy[k] = malloc(BUFSIZE);
	 strcpy(sentencescpy[k],sentences[k]);
 }
fclose(fp);

for(int j=0;j<i;j++){
	
char delim[]=",?!:;. ";//The other punctuations can also be added.
char *ptr =strtok(sentences[j],delim);

while (ptr!=NULL){ //getting the sentences which contains the word
	//printf("%s ",ptr);
	if(strcasecmp(ptr,word)==0){
		wantedSen[wanNum]=malloc(BUFSIZE);
		strcpy(wantedSen[wanNum],sentencescpy[j]);
		wanNum++;
		break;
	}
	
	ptr=strtok(NULL,delim);
}
}
/*It scans the sentences one by one and prints them by changing the color 
according to the color entered in the command when the desired word is encountered.*/
for(int i=0;i<wanNum;i++){
	char delim[]=" \t";
	char *ptr =strtok(wantedSen[i],delim);
	int len = strlen(ptr);
	while (ptr!=NULL){
		if(strncasecmp(ptr,word,len)==0 ){
			if(strcmp(color,"r")==0){
				red();
			}else if (strcmp(color,"g")==0){
				green();
			}else{
				blue();
			}
			printf("%s ",ptr);
			reset();
			ptr=strtok(NULL,delim);
		}
		if(ptr!=NULL){
		printf("%s ",ptr);
			ptr=strtok(NULL,delim);
			
		}
			

	}
}

}
//Returns the name of a given path if that path is already in the list.
char* getName(char* givenPath){
FILE* sdFile;
sdFile=fopen(initCurDir,"r");
char line[MAX_LINE];
char *name;
char *path;
      while(fgets(line, sizeof(line), sdFile)!=NULL){
         name=strtok(line," ");
         path=strtok(NULL," ");
         int len=strlen(path);
         path[len-1]='\0';
         if(strcmp(path,givenPath)==0){
         return name;
         }
         
      }
      return "NULL";

}
//Returns the name of a given name if that name is already in the list.
char* getName2(char* givenName){
FILE* sdFile;
sdFile=fopen(initCurDir,"r");
char line[MAX_LINE];
char *name;
char *path;
      while(fgets(line, sizeof(line), sdFile)!=NULL){
         name=strtok(line," ");
         path=strtok(NULL," ");
         if(strcmp(name,givenName)==0){
         return name;
         }
         
      }
      return "NULL";

}
void sdSetName(char* name){	
	char curDir[MAX_LINE];
	getcwd(curDir,sizeof(curDir));//stores the current directory in curDir.
	char *namesAndPaths[MAX_LINE];
	char *oldName;
	int pathsNum=0;
	int BUFSIZE=100;
	int set=1;//if set equals to 1 then it writes on the file otherwise it means it is already in the file.	
	int isAlias=0;
	
	//open the files and writes the name and the path.
	
	char *returnedName2=getName2(name);//checks if given name is already in the file.
	if(strcmp(returnedName2,name)==0){
	printf("This alias: %s is already in the list\n",name);
	isAlias=1;
	}
	//overwrites-changes the path name.
	char *returnedName=getName(curDir);//checks if given path is already in the file
	if(strcmp(returnedName,"NULL")!=0&&isAlias==0){
	sdDelete(returnedName);//delete and set=1 to overwrite	
	set=1;
	}
	
	if(set==1&&isAlias==0){//writes the given name and path to file
	sdFile=fopen(initCurDir,"a");
	fprintf(sdFile,"%s  %s\n",name,curDir);
	printf("%s is set as an alias for %s\n",name,curDir);
	}
}



void sdJumpName(char* name){
	FILE *fp=fopen(initCurDir,"r");
	char jumpDir[MAX_LINE];
	char* names_paths[MAX_LINE];
	int path_num=0;
	int BUFSIZE=100;
        names_paths[path_num]= malloc(BUFSIZE);
	while (fgets(names_paths[path_num], BUFSIZE, fp)) {
       	path_num++;
        names_paths[path_num] = malloc(BUFSIZE);
}
	for(int i=0;i<path_num;i++){
		char delim[]=" ";
		char *ptr =strtok(names_paths[i],delim);
			while (ptr!=NULL){
       	 		if(strcasecmp(ptr,name)==0){
          			ptr=strtok(NULL,delim);
				strcpy(jumpDir,ptr);
				break;
        }
        	ptr=strtok(NULL,delim);
}
}
	
	int len=strlen(jumpDir);
	jumpDir[len-1]='\0';
	chdir(jumpDir);
	printf("Curent directory is set to : %s\n",jumpDir);
	fclose(fp);
}
void sdPrintList(){
	char *namesAndPaths[MAX_LINE];
        int pathsNum=0;
        int BUFSIZE=100;
	sdFile=fopen(initCurDir,"r");
        namesAndPaths[pathsNum] = malloc(BUFSIZE);
        while (fgets(namesAndPaths[pathsNum], BUFSIZE, sdFile)) {
        pathsNum++;
        namesAndPaths[pathsNum] = malloc(BUFSIZE);
}
	if(pathsNum==0){
		printf("There is no name-directory association in the list.\n");
	}else{
	for(int i=0;i<pathsNum;i++){
        char delim[]=" ";
        char *ptr =strtok(namesAndPaths[i],delim);
        while (ptr!=NULL){
             	char *alias=ptr;
                ptr=strtok(NULL,delim);
                printf("%s is an alias for path : %s\n",alias,ptr);        
                ptr=strtok(NULL,delim);
      
}
}
}
}
void sdDelete(char* name){
	char *namesAndPaths[MAX_LINE];
	char *n_p_cpy[MAX_LINE];
        int pathsNum=0;
        int BUFSIZE=100;
        sdFile=fopen(initCurDir,"r");
        namesAndPaths[pathsNum] = malloc(BUFSIZE);
        while (fgets(namesAndPaths[pathsNum], BUFSIZE, sdFile)) {
        	pathsNum++;
        	namesAndPaths[pathsNum] = malloc(BUFSIZE);
}	
//Copying namesAndPaths array.
	 n_p_cpy[0] = malloc(BUFSIZE);
	 for (int k=0;k<pathsNum;k++){
         n_p_cpy[k] = malloc(BUFSIZE);
         strcpy(n_p_cpy[k],namesAndPaths[k]);
 }
	fclose(sdFile);
	FILE *fp;
        fp=fopen(initCurDir,"w");
	for(int i=0;i<pathsNum;i++){
		char delim[]=" ";
        	char *ptr =strtok(namesAndPaths[i],delim);
        	while (ptr!=NULL){	
			if(strcmp(ptr,name)!=0){
				//printf("ptr : %s\n",ptr);
                                fprintf(fp,"%s",n_p_cpy[i]);
                                ptr =strtok(NULL," "); 
				ptr =strtok(NULL," ");

			}else{
			        ptr=strtok(NULL," ");
			        ptr=strtok(NULL," ");

        }
}

}
fclose(sdFile);
}


void sdClearList(){
//opens a new file with the same path
	FILE *fp;
	fp=fopen(initCurDir,"w");
}

void goodMorning(struct command_t *command){
	FILE * fp;
   	int i;
   	//stores the hour and minute
	char *delim = ".";
	char *hour = strtok(command->args[1], delim);
        char *minute;
	if(hour != NULL){
        	minute = strtok(NULL, delim);
}

	/* open the file for writing*/
   	fp = fopen("cronFile.txt","w");
      fprintf(fp,"%s ",minute);
	fprintf(fp,"%s ",hour);
	fprintf(fp,"%s ","* * * XDG_RUNTIME_DIR=/run/user/$(id -u)");
	fprintf(fp,"DISPLAY=:0.0 /usr/bin/rhythmbox-client --play %s\n",command->args[2]);
   	fclose (fp);
   	//executes the crontab.
	pid_t childProcess=fork();
	if(childProcess==0){	
		char *cronArgs[] = { "/usr/bin/crontab", "cronFile.txt", 0 };
   		execv(cronArgs[0], cronArgs);	
	}else{
		wait(&childProcess);
	}
}
/*
it requires only the name of the file (without extensions such as ".bin" ".txt", the file should be in the same directory with this file
it counts the number of words and lines in the given file
*/
void wordCount(struct command_t *command){
	char filename[64];
	char *file_arg = command->args[1]; //getting the name of the file
	sprintf(filename,"%s.txt",file_arg); //adding txt extension to it
	FILE *fp;
	fp = fopen(filename,"r"); //openning the file in the same directory
	if(fp==NULL){ //cannot open the file
	printf("\nPlease check if file exist\nEnter file without txt extensions\n");
	exit(0);
       }
			
	printf("\nThe file name is: %s\n", filename);
			
	char delim; //for getting the each char in the file
	int counter = 0; //for counting words
	int line_counter = 0; //for counting lines
	
	while((delim=fgetc(fp))!=EOF){
		if(delim==' ' || delim=='\r' || delim=='\t' || delim=='\n' || delim==EOF){ //every space character shows the number of words
			counter++;
			
		}
			
}
	fseek(fp,0,SEEK_SET);
	while((delim=fgetc(fp))!=EOF){
		if(delim=='\n' || delim==EOF){ //new line and end of the file shows the number of lines
		line_counter++;
	}
	}
			
	printf("\nWord count is: %d\n", counter);
	printf("\nLine count is: %d\n", line_counter);//printing the results
	fclose(fp);//closing the file

}
void kdiff(struct command_t *command){
			
       		char* command_arg=""; //kdiff type
       		char line1[200];
       		char line2[200];
       		char* file1;
       		char* file2;
       		char* token1;
       		char* ext1;
       		char* token2;
       		char* ext2;
       		
       		

       		if(command->arg_count<5){ //without -a 
       			command_arg="-a";
       			file1 = command->args[1];
       			file2 = command->args[2];
       			
       			
       		}else{ // with -a 
       			command_arg = command->args[1];
       			file1 = command->args[2];
       			file2 = command->args[3];
       		}
       		
       		if(strcmp(command_arg,"-a")==0){
       		//getting the extension of the files
       		token1=strtok(file1,".");//the name
       		ext1=strtok(NULL,"."); //extension
       		token2=strtok(file2,".");
       		ext2=strtok(NULL,".");

       		//checking if the extension is txt	
       		if(strcmp(ext1,"txt")!=0 || strcmp(ext2,"txt")!=0){
			printf("\nPlease enter a txt file\n");
			exit(0);
       		}
       		strcat(file1,".txt");
       		strcat(file2,".txt");//adding .txt for openning
       		FILE* fp1;
       		FILE* fp2;
       		int line_counter = 0; //index of the lines
       		//lines are starting from 0, you can change it to 1
       		int diff_counter=0; //counter for differeneces
       		
             	fp1 = fopen(file1,"r");
       		fp2 = fopen(file2,"r"); //opening files
       		//checking if the files exist
       		if(fp1==NULL || fp2==NULL){
       			printf("\nNull file pointer\n");
       			exit(0);
       		}
       		
       		while(fgets(line1,(int) sizeof(line1),fp1)!=NULL &&
       		fgets(line2,(int) sizeof(line2),fp2)!=NULL){
       			if(strcmp(line1,line2)!=0){
       				printf("\nfile1.txt: Line %d : %s\n", line_counter, line1);
       				printf("\nfile2.txt: Line %d : %s\n", line_counter, line2);
       				diff_counter++;
       			}
       		
       			line_counter++;
       		}
       		if(diff_counter==0){
       			printf("\nThe two files are identical\n");
       		}else{
       			printf("\nThere are %d different lines\n", diff_counter);
       		}
       		fclose(fp1);
       		fclose(fp2);//closing the files
       		}//end of -a
       		
       		if(strcmp(command_arg,"-b")==0){
       			
       			FILE* fpb1;
       			FILE* fpb2; //file pointers
       			char* fileb1;
       			char* fileb2; //for getting the file name 
       			char byte1;
       			char byte2; //for comparing the bytes in files
       			int difference=0; //counter for differences
       			fileb1=command->args[2];
       			fileb2=command->args[3]; //getting the files from command
       			printf("fileb1:%s\n",fileb1);
       			printf("fileb2:%s\n",fileb2);
       			//getting the file name 
       			fpb1 = fopen(fileb1,"r");
       			fpb2 = fopen(fileb2,"r"); //opening files
       			int count1=0;
       			int count2=0; 
       			
       			if(fpb1==NULL || fpb2==NULL){//error while openning the files
       			printf("\nNull file pointer\n");
       			exit(0);
       			}
       			//size of first file = count1
       			while(fread(&byte1,sizeof(char),1,fpb1)==1 ){
       			count1++;
       			}
       			//size of second file = count2
       			while(fread(&byte2,sizeof(char),1,fpb2)==1){
       			count2++;
       			}
       			
       			//going to beginning of the files
       			fseek(fpb1,0,SEEK_SET);
       			fseek(fpb2,0,SEEK_SET);
       			//while at least one file is not finished
       			while(fread(&byte1,sizeof(char),1,fpb1)==1 && fread(&byte2,sizeof(char),1,fpb2)==1){
       			//comparing byte by byte
       			if(strncmp(&byte1,&byte2,1)!=0){
       				//printf("byte1: %c \n", byte1);
       				//printf("byte2: %c \n", byte2);
       				difference++;
       			}
       			}
       			//for different sized files
       			int size_difference = abs(count1-count2);
       			//subtracted -1 because we compared null terminator in smaller file and other non-null-terminator byte in longer file already!
       			if(size_difference!=0){
       			difference+=(size_difference-1);
       			}
       			
       			if(difference==0){
       			printf("\nThe files are identical\n");
       			}else{
       			printf("\nThere are %d byte differences\n", difference);
       		
       	}	
       		//closing the files
       		fclose(fpb1);
       		fclose(fpb2);
       	}// end of -b
       		
       	}//end of kdiff





//int process_command(struct command_t *command);

int main()
{       

	 getcwd(initCurDir,sizeof(initCurDir));//stores the path of the seashell.c in global array initCurDir
         strcat(initCurDir,"/sdFile.txt");
 	 sdFile=fopen(initCurDir,"r");
	 if(sdFile==NULL){
		sdFile=fopen(initCurDir,"w+");
	}
	while (1)
	{
		struct command_t *command=malloc(sizeof(struct command_t));
		memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

		int code;
		code = prompt(command);
		if (code==EXIT) break;

		code = process_command(command);
		if (code==EXIT) break;

		free_command(command);
	}

	fclose(sdFile);
	printf("\n");
	return 0;
}

int process_command(struct command_t *command)
{	 

	int r;
	if (strcmp(command->name, "")==0) return SUCCESS;

	if (strcmp(command->name, "exit")==0)
		return EXIT;

	if (strcmp(command->name, "cd")==0)
	{
		if (command->arg_count > 0)
		{
			r=chdir(command->args[0]);
			if (r==-1)
				printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
			return SUCCESS;
		}
	}
	       	if((strcmp(command->name,"shortdir")==0)&&(strcmp(command->args[0],"jump"))==0){
                        char *setName= command->args[1];
        		 sdJumpName(setName);                
                }



	pid_t pid=fork();

	if (pid==0) // child
	{
		/// This shows how to do exec with environ (but is not available on MacOs)
	    // extern char** environ; // environment variables
		// execvpe(command->name, command->args, environ); // exec+args+path+environ

		/// This shows how to do exec with auto-path resolve
		// add a NULL argument to the end of args, and the name to the beginning
		// as required by exec

		// increase args size by 2
	
			command->args=(char **)realloc(
			command->args, sizeof(char *)*(command->arg_count+=2));

		// shift everything forward by 1
		for (int i=command->arg_count-2;i>0;--i){
			command->args[i]=command->args[i-1];
		}

		// set args[0] as a copy of name
		command->args[0]=strdup(command->name);
		// set args[arg_count-1] (last) to NULL
		command->args[command->arg_count-1]=NULL;

	//	execvp(command->name, command->args); // exec+args+path
		if(strcmp(command->name, "wordcount")==0){
			wordCount(command);
		}
		
		if(strcmp(command->name,"goodMorning")==0){
			goodMorning(command);
		}
		if(strcmp(command->name,"highlight")==0){
			char *word_searched=command->args[1];
			char *color=command->args[2];
			char *fileName=command->args[3];
			highlight(word_searched,color,fileName);	
		}
		if(strcmp(command->name,"shortdir")==0){
			char *setName= command->args[2];
			if(strcmp(command->args[1],"set")==0){
				sdSetName(setName);
        	        }
			 if(strcmp(command->args[1],"del")==0){
                                sdDelete(setName);
                        }

			if(strcmp(command->args[1],"clear")==0){
				sdClearList();
			}

			if(strcmp(command->args[1],"list")==0){
				sdPrintList();                              	
                        }

		}
			if(strcmp(command->name, "kdiff")==0){
				kdiff(command);
       	}
		
	
 /// TODO: do your own exec with path resolving using execv()

		char path[120]="/bin/";
		strcat(path,command->args[0]);
		memcpy(command->args[0],path,strlen(path)+1);
		execv(path,command->args);		
		exit(0);
	
	}
	else
	{
		if (!command->background)
			wait(0); // wait for child process to finish
		return SUCCESS;
	}

	 }
	 
	 

//authors: Ece Güz (69002) , Zeynep Sıla Kaya (69101)
