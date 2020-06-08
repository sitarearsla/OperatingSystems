#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

void hexConverter(char* input, char* output)
{
    int loop;
    int i; 
    
    i=0;
    loop=0;
    
    while(input[loop] != '\0')
    {
        sprintf((char*)(output+i),"%02X", input[loop]);
        loop+=1;
        i+=2;
    }
    //insert NULL at the end of the output string
    output[i++] = '\0';
}

int childA(int pipefd2[2], int pipefd3[2], void* ptr_a, char* shm_name_a)
{
	char message[1024];
	char * const done = "done";
	close(pipefd2[1]);          /* Close unused write end */
	read(pipefd2[0], &message, sizeof(message));
	sprintf(ptr_a, "%s", message);
	shm_unlink(shm_name_a);
	close(pipefd3[0]);          /* Close unused read end */
	write(pipefd3[1], &done, sizeof(done));
	return 0;
}

int childB(int pipefd[2], int pipefd4[2], void* ptr_b, char* shm_name_b)
{
	char message[1024];
	char new_message[2048];
	char * const done = "done";
	close(pipefd[1]);          /* Close unused write end */
	read(pipefd[0], &message, sizeof(message));

	hexConverter(message, new_message);
	sprintf(ptr_b, "%s", new_message);
	shm_unlink(shm_name_b);
	close(pipefd4[0]);          /* Close unused read end */
	write(pipefd4[1], &done, sizeof(done));
	return 0;
}

int main()
{
	FILE *f;
	FILE *f1;
	FILE *f2;

	pid_t pid;
	pid_t pid1;	

	pid=fork();
	
	int pipefd[2]; 
	int pipefd1[2]; 
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

	char buf[1024];
	size_t size_a =sizeof(buf);
	size_t size_b = size_a*2;
	printf("Size: %zu and %zu.\n",size_a);

	int shmfd_a;
	void *ptr_a;
	char* const shm_name_a="shared_memory_a";

	int shmfd_b;
	void *ptr_b;
	char* const shm_name_b="shared_memory_b";

	shmfd_a=shm_open(shm_name_a, O_CREAT| O_RDWR, 0666); //returns a fd that we can use in subsequent calls
	shmfd_b=shm_open(shm_name_b, O_CREAT| O_RDWR, 0666); //returns a fd that we can use in subsequent calls

	ftruncate(shmfd_a, size_a);
	ftruncate(shmfd_b, size_b);

	ptr_a=mmap(NULL, size_a, PROT_WRITE|PROT_READ, MAP_SHARED, shmfd_a, 0);
	ptr_b=mmap(NULL, size_b, PROT_WRITE|PROT_READ, MAP_SHARED, shmfd_b, 0);

	if(pid==-1)
	{
		perror("fork failed");
		exit(-1);
	}

	//first child process A
	if (pid==0)
	{
		return childA(pipefd2, pipefd3, ptr_a, shm_name_a);
	} else {
		pid1 = fork();
		//second child process B
		if(pid1==0){
			return childB(pipefd, pipefd4, ptr_b, shm_name_b);
		} else {
			close(pipefd[0]);
			close(pipefd2[0]);
			
			}
	}
	
}
