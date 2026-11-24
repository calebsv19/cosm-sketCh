/* Exercises the real local transport with controlled owner-dispatch outcomes. */
#include "../src/control/control_internal.h"
#include <SDL.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
static unsigned mode, calls;
static atomic_bool sending;
/* Supplies ordinary, pressured, or allocation-failed owner results. */
char* sketch_control_dispatch(SketchControl* b, DrawingProgramAppContext* a,
                             const char* r, bool busy, bool* changed) {
    (void)b; (void)a; (void)busy; assert(!strcmp(r,"{}")); ++calls; *changed=false;
    if(mode>=2) return NULL;
    if(!mode) return strdup("{\"ok\":true}");
    size_t n=8u*1024u*1024u; char* p=malloc(n+1); assert(p);
    memset(p,'x',n); p[n]=0; return p;
}
/* Injects only the fallback allocation failure, after dispatch has returned. */
static char* fallback_copy(const char* s) { return mode==3 ? NULL : strdup(s); }
/* Observes actual socket output without replacing timeout/backpressure behavior. */
static ssize_t observed_send(int fd,const void* p,size_t n,int flags) {
    atomic_store(&sending,true); return send(fd,p,n,flags);
}
#define strdup fallback_copy
#define send observed_send
#include "../src/control/control_bridge.c"
#undef send
#undef strdup
/* Waits for transport publication under its owning mutex. */
static void wait_pending(SketchControl* b) {
    Uint64 end=SDL_GetTicks64()+5000;
    for(;;) { pthread_mutex_lock(&b->mutex); bool p=b->pending;
        pthread_mutex_unlock(&b->mutex); if(p)return;
        assert(SDL_GetTicks64()<end); SDL_Delay(1); }
}
/* Opens a private client and submits one complete request. */
static int client(const char* path) {
    int fd=socket(AF_UNIX,SOCK_STREAM,0); assert(fd>=0);
    struct sockaddr_un a={.sun_family=AF_UNIX}; strcpy(a.sun_path,path);
    assert(connect(fd,(struct sockaddr*)&a,sizeof(a))==0);
    struct timeval tv={.tv_sec=6}; assert(!setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv)));
    assert(send(fd,"{}\n",3,0)==3); return fd;
}
/* Covers pending stop, slow reply, fallback failure and endpoint reuse. */
int main(void) {
    alarm(35); assert(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_TIMER)==0);
    const char* tmp_root=getenv("TMPDIR");
    if(!tmp_root || !*tmp_root) tmp_root="/tmp";
    char path[sizeof(((struct sockaddr_un*)0)->sun_path)];
    char dir[sizeof(path)];
    int length=snprintf(dir,sizeof(dir),"%s/sk-l3-XXXXXX",tmp_root);
    assert(length>0 && (size_t)length+sizeof("/socket")<=sizeof(path));
    assert(mkdtemp(dir));
    snprintf(path,sizeof(path),"%s/socket",dir);
    static DrawingProgramAppContext app;
    for(unsigned c=0;c<5;++c) {
        mode=c ? c-1 : 0; atomic_store(&sending,false);
        SketchControl* b=sketch_control_start(path,false,NULL); if(!b)perror("sketch_control_start"); assert(b);
        int fd=client(path); wait_pending(b); unsigned before=calls;
        if(c) { assert(!sketch_control_poll(b,&app,false)); assert(calls==before+1); }
        if(c==2) { Uint64 end=SDL_GetTicks64()+5000;
            while(!atomic_load(&sending)) {assert(SDL_GetTicks64()<end);SDL_Delay(1);} }
        if(c==1 || c==3) { char response[256]={0}; ssize_t n=recv(fd,response,255,0);
            assert(n>0); assert(strstr(response,c==1 ? "true" : "allocation_failed")); }
        Uint64 start=SDL_GetTicks64(); sketch_control_stop(b);
        assert(SDL_GetTicks64()-start<6000); assert(access(path,F_OK)!=0);
        if(!c)assert(calls==before); close(fd);
        printf("case %u passed; shutdown %llu ms\n",c,(unsigned long long)(SDL_GetTicks64()-start));
    }
    assert(rmdir(dir)==0); SDL_Quit(); puts("sketCh control lifetime passed"); return 0;
}
