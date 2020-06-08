#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include<sys/wait.h>

int grandchild(int pipefd2[2], int pipefd3[2])
{
	int parent_answer;
	close(pipefd2[1]);          /* Close unused write end */
	read(pipefd2[0], &parent_answer, sizeof(parent_answer));
	printf("%d grandchild reads result from child\n", parent_answer);
	fflush(stdout);
	close(pipefd2[0]);

	int new_answer;
	new_answer=parent_answer*2;
	close(pipefd3[0]);          /* Close unused read end */
	write(pipefd3[1], &new_answer, sizeof(new_answer));
	close(pipefd3[1]);
	printf("Grandchild: Input %d, Output %d*2=%d\n", parent_answer, parent_answer, new_answer);
	printf("%d grandchild writes result to child\n",new_answer);
	fflush(stdout);
//
	return 0;
}

int child(int pipefd3[2], int pipefd2[2], int pipefd1[2], int pipefd[2])
{
	int parent_answer;
	close(pipefd1[1]);          /* Close unused write end */
	read(pipefd1[0], &parent_answer, sizeof(parent_answer));
	printf("%d child reads result from parent\n", parent_answer);
	fflush(stdout);
	close(pipefd1[0]);

	sleep(1);

	int new_answer;
	new_answer=parent_answer*2;
	close(pipefd2[0]);          /* Close unused read end */
	write(pipefd2[1], &new_answer, sizeof(new_answer));
	close(pipefd2[1]);
	printf("Child: Input %d, Output %d*2=%d\n", parent_answer, parent_answer, new_answer);
	printf("%d child writes result to grandchild\n",new_answer);
	fflush(stdout);

	sleep(1);

	int gc_answer;
	close(pipefd3[1]);          /* Close unused write end */
	read(pipefd3[0], &gc_answer, sizeof(gc_answer));
	printf("%d child reads result from grandchild\n", gc_answer);
	fflush(stdout);
	close(pipefd3[0]);

	sleep(1);

	close(pipefd[0]);          /* Close unused read end */
	write(pipefd[1], &gc_answer, sizeof(gc_answer));
	close(pipefd[1]);
	printf("%d child writes result to parent\n",gc_answer);
	fflush(stdout);

	wait(NULL);
	return 0;

}

int parent(int pipefd[2], int pipefd1[2], int input, pid_t child_pid)
{
    //sleep(5);
	int parent_answer;
	parent_answer = input;
	close(pipefd1[0]);          /* Close unused read end */
	write(pipefd1[1], &parent_answer, sizeof(parent_answer));
	close(pipefd1[1]);
	printf("%d parent writes result to child\n",parent_answer);
	fflush(stdout);

	wait(NULL);

	int answer;
	close(pipefd[1]);          //Close unused write end 
	read(pipefd[0], &answer, sizeof(answer));
	printf("%d parent reads result from child\n", answer);
	fflush(stdout);
	close(pipefd[0]);

    return 0;
}

int main()
{
	int input;
	scanf("%d", &input);
	pid_t pid;
	pid_t pid1;	
	
	int pipefd[2]; //child writes, parent reads
	int pipefd1[2]; //child reads, parent writes
	int pipefd2[2];
	int pipefd3[2];

   	if (pipe(pipefd) == -1)
   	 {
	    perror(" pipe");
	    exit(EXIT_FAILURE);
    	}

 	if (pipe(pipefd1) == -1)
    	{
	    perror(" pipe");
	    exit(EXIT_FAILURE);
    	}

	if (pipe(pipefd2) == -1)
    	{
	    perror(" pipe");
	    exit(EXIT_FAILURE);
    	}

	if (pipe(pipefd3) == -1)
   	 {
	    perror(" pipe");
	    exit(EXIT_FAILURE);
    	}

	pid=fork();

	if(pid==-1)
	{
		perror("fork failed");
		exit(-1);
	}
	if(pid==0) // we are in the child process
	{
		pid1 = fork();
		if(pid1==0) // we are in the child process
		{
		return grandchild(pipefd2, pipefd3);
		} else {
		return child(pipefd3, pipefd2, pipefd1, pipefd);	
		}
	}
	else // we are in the parent process
	{
		return parent(pipefd, pipefd1, input, pid);
	}
}
