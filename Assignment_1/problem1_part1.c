#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<sys/time.h>

unsigned long get_time(){
struct timeval tv;
gettimeofday(&tv, NULL);
unsigned long ret = tv.tv_usec;
ret /= 1000;
ret += (tv.tv_sec * 1000);
return ret;
}


int child()
{
pid_t pid;
printf("I'm the child and my pid is: %d\n", getpid());
int i=1;
while(i<=5){
sleep(1);
printf("Child reporting: The time is: %ld and my pid is %d\n", get_time(), getpid());
i++;
}

return 0;
}

int parent(pid_t child_pid){
pid_t pid;
printf("I'm the parent and my pid is: %d\n", getpid());

int i=1;
while(i<=5){
sleep(1);
printf("Parent waiting ...%d\n", i);
i++;
}

//kill block
int r;
r = kill(child_pid, SIGTERM);
if(r==0){
printf("Kill successful: Child %d killed.\n", child_pid);
} else {
perror("Kill failed");
}
}

int main()
{
printf("Before any fork, I am declaring my pid: %d\n", getpid());
fflush(stdout);
pid_t pid;
pid=fork();
if(pid<0){
perror("fork failed");
exit(-1);
}
if (pid==0){
return child();
} else {
return parent(pid);
}
}
