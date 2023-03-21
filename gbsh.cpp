///////////////////NOTE: Preferable: Please run the included bash script with "./launch-shell.sh" or bash launch-shell.sh/////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <iostream> 
#include <cstring>
#include <errno.h>
#include <signal.h>
using namespace std;
extern char**environ;
string command;
char input[50];
char hostname[30];
char username[20];
char directory[40]; 
//char *path;
/*void newWindow()
{
	char* isnewind = getenv("newWindowLaunched");
	//fprintf(stdout, "Loading gbsh....\n");
	if(!isnewind || isnewind[0] != '1')
	{	
		
		pid_t pid = fork();
		if(!pid)
		{
			char temp[] = "newWindowLaunched=1";
			putenv(temp);
			sleep(1);
			//if(!system("bash launch-shell.sh"))
			//	exit(0);
			if(execlp("./launch-shell.sh","launch-shell.sh",NULL)<0)
			{
				if(!system("chmod +x launch-shell.sh"))
					execlp("./launch-shell.sh","launch-shell.sh",NULL);

			}
		}
		else
		{
			sleep(2);
			exit(0);
		}
	}
}
*/
///function implements piping between multiple programs dynamically/any amount
void dynamicPiping(char** args, bool bgexec,int numargs)
{
	char* progcur = args[0];
	args++;
	char* progcurargs[50];
	progcurargs[0] = progcur;
	int argtracker = 1;
	int oldcin = dup(0);
	int oldcout = dup(1);
	int fdreadend;
	while(*args != NULL) // incase more piping and redirecting is taking place
	{
		if(strncmp(*args,"|",1) == 0) //pipe command
		{
			int fd[2];
			pipe(fd);
			fcntl(fd[1],F_SETFL,O_NONBLOCK);
			fdreadend = fd[0];
			pid_t pid = fork();
			if(!pid)
			{
				dup2(fd[1],1);
				execvp(progcur,progcurargs);				
			}
			dup2(fd[0],0);
			progcur = *(++args);
			progcurargs[0]=progcur;
			progcurargs[1] = NULL;
			argtracker = 0;
		}

		if(*args)
		{	
			if(strncmp(*args,">",1)==0) //command for writing into another command
			{
				//argv[i] = NULL;
				args++;
				if(args[0])
				{

					int newdest = open(args[0],O_CREAT | O_WRONLY | O_TRUNC);
					if(newdest == -1)
						printf("\033[1;31m%s\n\033[0m",strerror(errno));
					else
					{
						dup2(newdest,1);
						close(newdest);
					}
					args++;
					break;
				}
			}
			else if(strncmp(args[0],"<",1)==0) //command for reading from another command
			{
				args++;
				if(args[0])
				{
					int newsrc = open(args[0],O_RDONLY);
					args++;
					if(newsrc == -1)
						printf("\033[1;31m%s\n\033[0m",strerror(errno));
					else
					{
						dup2(newsrc,0);
						if(args[0])
						{
							if(strncmp(args[0],">",1)==0) //command for writing into another command
							{
								args++;
								if(args[0])
								{
									int newdest = open(args[0],O_CREAT | O_WRONLY | O_TRUNC);
									if(newdest == -1)
										printf("\033[1;31m%s\n\033[0m",strerror(errno));
									else
										dup2(newdest,1);
									args++;
									break;
								}
							}
						}	
					}
				}
			}
		}
		progcurargs[argtracker++] = *args; 
	}
	if(progcur)
	{
		pid_t pid = fork();
		if(!pid)
		{
			execvp(progcur,progcurargs);				
		}
	}
	if(bgexec)
		wait(NULL);
	dup2(oldcin,0);
	dup2(oldcout,1);
	return;
}
//main program/command handling function
void processCommand(char* command)
{
	//ignore whitespaces at the start
	while(command[0] == ' ' || command[0] == '\n' || command[0] == '\t')
	{
		for(int i = 0; i < strlen(command); i++)
			command[i] = command[i+1];
	}
	//ignore whitespaces at the end
	while(command[strlen(command)-1] == ' ' || command[strlen(command)-1] == '\n' || command[strlen(command)-1] == '\t')
	{
		command[strlen(command)-1] = '\0';
	}

	// close shell
	if(strncmp(command,"exit",4) == 0)
	{
		fprintf(stdout, "Closing gbsh\033[5m....\033[0m\n");
		char temp[] = "newWindowLaunched=0";
		putenv(temp);
		sleep(1);
		exit(0);
	} ///pwd command implementation
	else if(strncmp(command,"pwd",3) == 0)
	{
		FILE* dest = stdout;
		strtok(command," \n\r\t");
		char* redirect = strtok(NULL, " \n\r\t");
		if(redirect)
		{
			char* filedest = strtok(NULL, " \n\r\t");
			if(filedest && strcmp(redirect,">")==0)
				dest = fopen(filedest,"w");
		}
		char temp[50];
		getcwd(temp,50);
		fprintf(dest, "%-s\n", temp);
		if(dest!=stdout)
			fclose(dest);
	}//clear command implementation
	else if(strncmp(command,"clear",5) == 0)
	{
		printf("\033[2J\033[1;1H");
	}//ls command implementation
	else if(strncmp(command, "ls",2)==0 && (command[2]==' ' || command[2]=='\0'))	/////ls block
	{
		struct dirent **directory;
		int n;
		char* prog = strtok(command," \n\r\t");
		char* dirarg = strtok(NULL," \n\r\t");
		char *redirect = strtok(NULL," \n\r\t");
		char* filedest = strtok(NULL, "\n\r\t");
		
		if(dirarg)
		{
				if(dirarg[0]=='>')
					n=scandir(".",&directory,NULL,versionsort);
				else
					n=scandir(dirarg,&directory,NULL,versionsort);	
		}
		else
			n=scandir(".",&directory,NULL,versionsort);
		
		if(n == -1)
		{
			printf("\033[1;31m%s\n\033[0m",strerror(errno));
			return;
		}
		else
		{
			FILE* dest = stdout;
			if(dirarg)
			{
				if(dirarg[0]=='>')
				{
					if(redirect)
					{	dest = fopen(redirect,"w"); cout << redirect << endl;}
				}				
				else if(redirect)
				{
					if(strcmp(redirect,">")==0 && filedest)
					{	
						dest = fopen(filedest,"w");
					}
				}
			}
			for(int i = 0; i < n; i++)
				 fprintf(dest,"%s\n",directory[i]->d_name);
			if(dest!=stdout)
				fclose(dest);	
		}
		free(directory);
	}//cd implementation
	else if(strncmp(command, "cd",2)==0 && (command[2]==' ' || command[2]=='\0'))	/////cd block
	{
		int n;
		if(strlen(command)==2)
			n=chdir("/home");
		else
			n=chdir(command+3);
		if(n == -1)
		{
			printf("\033[1;31m%s\n\033[0m",strerror(errno));
		}
		else
			getcwd(directory,40);
	}//environ implementation
	else if(strncmp(command,"environ",7) == 0)
	{
		FILE* dest = stdout;
		strtok(command," \n\r\t");
		char* redirect = strtok(NULL, " \n\r\t");
		if(redirect)
		{
			char* filedest = strtok(NULL, " \n\r\t");
			if(filedest && strcmp(redirect,">")==0)
				dest = fopen(filedest,"w");
		}
		char** envprint = environ;
		for(;*envprint != NULL;envprint++)
		{	
			fprintf(dest,"%-s\n",*envprint);
		}
		if(dest!=stdout)
			fclose(dest);
	}//setenv implementation
	else if(strncmp(command,"setenv",6) == 0)
	{
		strtok(command," \t");
		char* var = strtok(NULL," \t");
		char* val = strtok(NULL," \t");
		if(val)
			setenv(var,val,1);
		else
			setenv(var,"",1);
	}//unsetenv implementation
	else if(strncmp(command,"unsetenv",8) == 0)
	{
		strtok(command," \t");
		char* var = strtok(NULL," \t");
		unsetenv(var);
	}//handle any other commands
	else if(command)
	{
		/*char* token = strtok(command," \n\r\t"); 		//part 1 d)
		while(token)
		{
			printf("%-s\n",token);
			token = strtok(NULL," \n\r\t");
		}*/
		/*char* paths[20];
		int numpaths = 0;
		paths[0] = strtok(path,":");
		while(paths[numpaths]!=NULL)
			paths[++numpaths]=strtok(NULL,":");*/
		int src = dup(0);
		int dest = dup(1);	
		bool bgexec = false;
		if(command[strlen(command)-1]=='&') //& is a suffix for a given command to the shell, meaning the command will run in the background
		{
			bgexec=true;
			command[strlen(command)-1] = '\0';
		}
		char* program = strtok(command," \n\r\t");
		int numargs = 1;
		char* argv[50];
		argv[0] = program;

		char* arg = strtok(NULL, " \n\r\t");
		while(arg)
		{
			argv[numargs] = arg;
			numargs++;
			//printf("%-s\n",token);
			arg = strtok(NULL," \n\r\t");
		}
		argv[numargs] = NULL;
		for(int i = 0; i < numargs-1; i++) //check if pipe is part of command, send into dynamic piping function
		{
			if(strncmp(argv[i],"|",1)==0)
			{
				dynamicPiping(argv,bgexec,numargs);
				return;
			}
		}
		

		pid_t pid_child = fork();
		if(!pid_child)
		{
			for(int i = 1; i < numargs-1;i++)
			{
				if(strncmp(argv[i],">",1)==0) //writing output into a given file
				{
					argv[i] = NULL;
					if(argv[i+1])
					{

						int newdest = open(argv[i+1],O_CREAT | O_WRONLY | O_TRUNC);
						if(newdest == -1)
							printf("\033[1;31m%s\n\033[0m",strerror(errno));
						else
						{
							dup2(newdest,1);
							close(newdest);
						}
						argv[i+1] = NULL;
						break;
					}
				}
				else if(strncmp(argv[i],"<",1)==0) //reading from a given file argument
				{
					argv[i] = NULL;
					if(argv[i+1])
					{
						int newsrc = open(argv[i+1],O_RDONLY);
						argv[i+1] = NULL;
						if(newsrc == -1)
							printf("\033[1;31m%s\n\033[0m",strerror(errno));
						else
						{
							dup2(newsrc,0);
							if(i+2 < numargs)
							{
								if(strncmp(argv[i+2],">",1)==0) //writing into a given file argument
								{
									argv[i+2]=NULL;
									if(i+3 < numargs)
									{
										int newdest = open(argv[i+3],O_CREAT | O_WRONLY | O_TRUNC);
										if(newdest == -1)
											printf("\033[1;31m%s\n\033[0m",strerror(errno));
										else
											dup2(newdest,1);
										argv[i+3]=NULL;
										break;
									}
								}
							}	
						}
					}
				}
			}
			/*if(strncmp(program,"./gbsh",6)==0 || strncmp(program,"gbsh",4)==0 || !bgexec)
				setenv("newWindowLaunched","0",1);*/
			if(execvp(argv[0],argv)==-1)//error handling
			{	
				printf("\033[1;31m%s\n\033[0m",strerror(errno));
				char* token = strtok(command," \n\r\t"); 		//part 1 d)
				while(token)
				{
					printf("%-s\n",token);
					token = strtok(NULL," \n\r\t");
				}
				exit(0);
			}
			exit(0);
		}
		else if(!bgexec)
			wait(NULL);
	}
}

int main(int argc, char *argv[]) {

	gethostname(hostname, 30);
	getlogin_r(username,20);
	getcwd(directory, 40);
//	path = getenv("PATH");
	string shellenv = directory;
	shellenv = shellenv + "/gbsh";

	setenv("shell",shellenv.c_str(),1);
	
	signal(SIGINT,SIG_IGN);

	printf("\e]2;Grundlagen Betriebssysteme Shell: gbsh\a");   //  change window title during use of shell

	//basic loop that awaits input
	while(1)
	{
		fprintf(stdout, "\033[1;35m%-s\033[0;33m@\033[0;36m%-s:\033[0;32m%-s\033[0m> ", username, hostname, directory);		//printing multicoloured user prompt
		//cout <<
		fgets(input,50,stdin);		//using fgets to include whitespaces
		input[strlen(input)-1] = '\0';		//removing the terminating enter key input
		processCommand(input);

	}
	fprintf(stdout, "Closing gbsh....\n");
	sleep(1);
	exit(0); // exit normally	
}
