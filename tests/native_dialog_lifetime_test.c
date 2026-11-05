/* Proves native picker child ownership with real deterministic child processes. */
#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <spawn.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
static int mode, interrupted, read_interrupted;
static pid_t child;
/* Substitutes a finite child for the interactive AppleScript process. */
static int spawn_child(pid_t* pid, const char* path, const posix_spawn_file_actions_t* actions,
                       const posix_spawnattr_t* attr, char* const argv[], char* const env[]) {
    (void)path; (void)argv;
    if(mode==3)return EAGAIN;
    char* args[]={"/bin/sh","-c",mode==1 ? "printf /some/very/long/path" :
        mode==2 ? "exit 1" : "printf '/tmp/selected\\n'",NULL};
    int r=posix_spawn(pid,"/bin/sh",actions,attr,args,env);if(!r)child=*pid;return r;
}
/* Forces one interrupted wait without taking ownership of the actual child. */
static pid_t interrupted_wait(pid_t p,int* status,int flags) {
    if(mode==4 && !interrupted++){errno=EINTR;return -1;}
    return waitpid(p,status,flags);
}
/* Retries EINTR on the picker output pipe without losing bytes. */
static ssize_t interrupted_read(int fd,void* data,size_t count) {
    if(mode==5 && !read_interrupted++){errno=EINTR;return -1;}
    return read(fd,data,count);
}
/* Refuses pipe-action allocation before a child is admitted. */
static int fail_dup(posix_spawn_file_actions_t* a,int fd,int target) {
    return mode==6 ? ENOMEM : posix_spawn_file_actions_adddup2(a,fd,target);
}
#define posix_spawn_file_actions_adddup2 fail_dup
#define read interrupted_read
#define posix_spawn spawn_child
#define waitpid interrupted_wait
#include "../src/io/session/drawing_program_native_dialogs.c"
#undef posix_spawn_file_actions_adddup2
#undef read
#undef posix_spawn
#undef waitpid
/* Every successful spawn must be reaped; truncation and cancellation are refusals. */
int main(int argc,char** argv) {
    alarm(10); int only=argc>1 ? atoi(argv[1]) : -1;
    for(mode=0;mode<7;++mode) {
        if(only>=0 && mode!=only)continue;
        char path[64]="old"; child=0; interrupted=0; read_interrupted=0;
        CoreResult r=drawing_program_native_choose_file("test",NULL,path,mode==1?8:sizeof(path));
        bool ok=r.code==CORE_OK;
        bool reaped=true;if(child){int status;errno=0;pid_t p=waitpid(child,&status,WNOHANG);reaped=p==-1&&errno==ECHILD;if(p==0)waitpid(child,&status,0);}
        printf("mode %d ok=%d reaped=%d path=%s\n",mode,ok,reaped,path);fflush(stdout);
        assert(reaped);assert(ok==(mode==0||mode==4||mode==5));
        if(ok)assert(!strcmp(path,"/tmp/selected"));else assert(!path[0]);
    }
    puts("native dialog lifetime passed");return 0;
}
