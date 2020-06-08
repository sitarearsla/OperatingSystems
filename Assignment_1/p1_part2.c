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


int child(int childNumber)
{
pid_t pid;
printf("I'm child %d and my pid is: %d\n", childNumber, getpid());
int i=1;
while(i<=5){
sleep(1);
printf("Child no %d with PID %d reporting: The time is %ld\n", childNumber, getpid(), get_time());
i++;
}

return 0;
}

int parent(pid_t child_pid, pid_t child_pid1, pid_t child_pid2, pid_t child_pid3){
pid_t pid;
printf("I'm the parent and my pid is: %d\n", getpid());

int i=1;
while(i<=5){
sleep(1);
printf("Parent waiting ...%d\n", i);
i++;
}


//kill block 1
int r;
r = kill(child_pid, SIGTERM);
if(r==0){
printf("Kill successful: Child %d killed.\n", child_pid);
} else {
perror("Kill failed");
}

//kill block 2
int a;
a = kill(child_pid1, SIGTERM);
if(r==0){
printf("Kill successful: Child %d killed.\n", child_pid1);
} else {
perror("Kill failed");
}

//kill block 3 
int b;
b = kill(child_pid2, SIGTERM);
if(r==0){
printf("Kill successful: Child %d killed.\n", child_pid2);
} else {
perror("Kill failed");
}

//kill block
int c;
c = kill(child_pid3, SIGTERM);
if(r==0){
printf("Kill successful: Child %d killed.\n", child_pid3);
} else {
perror("Kill failed");
}
}

int main()
{
printf("Before any fork, I am declaring my pid: %d\n", getpid());
fflush(stdout);
pid_t pid, pid1, pid2, pid3;
pid=fork();
if(pid<0){
perror("fork failed");
exit(-1);
}
//first child process
if (pid==0){
//sleep(5);
return child(1);
} else {
pid1 = fork();
//second child process
if(pid1==0){
//sleep(4);
return child(2);
} else {
pid2 = fork();
//third child process
if(pid2 == 0){
//sleep(3);
return child(3);
} else {
pid3 = fork();
//fourth child process
if (pid3 == 0){
return child(4);
} else {
return parent(pid, pid1, pid2, pid3);
}
}
}

}
}
