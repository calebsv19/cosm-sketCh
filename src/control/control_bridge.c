#include "control_internal.h"
#include <SDL.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

// Checks transport cancellation without consulting mutable application state.
static bool bridge_stopping(SketchControl* bridge) {
    pthread_mutex_lock(&bridge->mutex);
    bool stopping = bridge->stopping;
    pthread_mutex_unlock(&bridge->mutex);
    return stopping;
}

// Sends a complete newline-framed response or abandons a disconnected reader.
static void send_response(SketchControl* bridge, int fd, const char* text) {
    size_t offset = 0, length = strlen(text);
    while (offset < length && !bridge_stopping(bridge)) {
        ssize_t sent = send(fd, text + offset, length - offset, 0);
        if (sent <= 0) return;
        offset += (size_t)sent;
    }
    (void)send(fd, "\n", 1, 0);
}

// Reads one bounded request and waits for owner-thread dispatch without touching the project.
static void serve_client(SketchControl* bridge, int fd) {
    struct timeval timeout = {.tv_sec = 2};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#ifdef SO_NOSIGPIPE
    int enabled = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
#endif
    uid_t uid;
    gid_t gid;
    if (getpeereid(fd, &uid, &gid) != 0 || uid != geteuid()) return;
    char* request = calloc(SKETCH_CONTROL_MAX_REQUEST + 1, 1);
    if (!request) return;
    size_t length = 0;
    bool complete = false;
    while (length < SKETCH_CONTROL_MAX_REQUEST && !bridge_stopping(bridge)) {
        ssize_t got = recv(fd, request + length, 1, 0);
        if (got <= 0 || request[length] == '\0') break;
        if (request[length++] == '\n') { complete = true; break; }
    }
    if (!complete) {
        send_response(bridge, fd, "{\"ok\":false,\"code\":\"invalid_frame\"}");
        free(request);
        return;
    }
    request[length - 1] = '\0';
    pthread_mutex_lock(&bridge->mutex);
    if (bridge->stopping) {
        pthread_mutex_unlock(&bridge->mutex);
        free(request);
        return;
    }
    bridge->request = request;
    bridge->pending = true;
    SDL_Event event = {.type = SDL_USEREVENT};
    (void)SDL_PushEvent(&event);
    while (!bridge->response && !bridge->stopping) pthread_cond_wait(&bridge->ready, &bridge->mutex);
    char* response = bridge->response;
    bridge->response = NULL;
    bridge->request = NULL;
    bridge->pending = false;
    pthread_mutex_unlock(&bridge->mutex);
    if (response) send_response(bridge, fd, response);
    free(response);
    free(request);
}

// Serializes local clients and leaves all authored work to application polling.
static void* bridge_thread(void* context) {
    SketchControl* bridge = context;
    while (!bridge_stopping(bridge)) {
        struct pollfd listener = {.fd = bridge->listener, .events = POLLIN};
        int ready = poll(&listener, 1, 100);
        if (ready < 0 && errno == EINTR) continue;
        if (ready < 0 || bridge_stopping(bridge)) break;
        if (!ready) continue;
        if (!(listener.revents & POLLIN)) break;
        int fd = accept(bridge->listener, NULL, NULL);
        if (fd < 0) {
            if (errno == EINTR) continue;
            break;
        }
        (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
        serve_client(bridge, fd);
        close(fd);
        pthread_mutex_lock(&bridge->mutex);
        bool stopping = bridge->stopping;
        pthread_mutex_unlock(&bridge->mutex);
        if (stopping) break;
    }
    return NULL;
}

// Removes only the socket inode originally created by this endpoint.
static void remove_socket(SketchControl* bridge) {
    struct stat info;
    if (lstat(bridge->path, &info) == 0 && info.st_dev == bridge->device && info.st_ino == bridge->inode)
        unlink(bridge->path);
}

// Requires an explicit private directory and never adopts or removes an existing endpoint.
SketchControl* sketch_control_start(const char* path, bool allow_write, const char* output_root) {
    if (!path || path[0] != '/' || strlen(path) >= sizeof(((SketchControl*)0)->path)) return NULL;
    char parent[104];
    strcpy(parent, path);
    char* slash = strrchr(parent, '/');
    if (!slash || slash == parent) return NULL;
    *slash = '\0';
    struct stat info;
    if (lstat(parent, &info) != 0 || !S_ISDIR(info.st_mode) || info.st_uid != geteuid() ||
        (info.st_mode & 077) != 0 || lstat(path, &info) == 0 || errno != ENOENT) return NULL;
    SketchControl* bridge = calloc(1, sizeof(*bridge));
    if (!bridge) return NULL;
    bridge->listener = socket(AF_UNIX, SOCK_STREAM, 0);
    if (bridge->listener < 0) { free(bridge); return NULL; }
    if (fcntl(bridge->listener, F_SETFD, FD_CLOEXEC) != 0) { close(bridge->listener); free(bridge); return NULL; }
    strcpy(bridge->path, path);
    bridge->allow_write = allow_write;
    if (output_root) {
        char resolved[PATH_MAX];
        if (!realpath(output_root, resolved) || strlen(resolved) >= sizeof(bridge->output_root) ||
            stat(resolved, &info) != 0 || !S_ISDIR(info.st_mode)) goto fail;
        strcpy(bridge->output_root, resolved);
    }
    if (allow_write && !bridge->output_root[0]) goto fail;
    unsigned char random[16];
    arc4random_buf(random, sizeof(random));
    for (int i = 0; i < 16; ++i) snprintf(bridge->instance + i * 2, 3, "%02x", random[i]);
    struct sockaddr_un address = {.sun_family = AF_UNIX};
    strcpy(address.sun_path, path);
    if (bind(bridge->listener, (struct sockaddr*)&address, sizeof(address)) != 0) goto fail;
    if (lstat(path, &info) != 0) goto fail;
    bridge->device = info.st_dev;
    bridge->inode = info.st_ino;
    if (chmod(path, 0600) != 0 || listen(bridge->listener, 4) != 0) goto fail;
    if (pthread_mutex_init(&bridge->mutex, NULL) != 0) goto fail;
    if (pthread_cond_init(&bridge->ready, NULL) != 0) {
        pthread_mutex_destroy(&bridge->mutex);
        goto fail;
    }
    if (pthread_create(&bridge->thread, NULL, bridge_thread, bridge) != 0) {
        pthread_cond_destroy(&bridge->ready);
        pthread_mutex_destroy(&bridge->mutex);
        goto fail;
    }
    return bridge;
fail:
    remove_socket(bridge);
    close(bridge->listener);
    free(bridge);
    return NULL;
}

// Processes one request with no socket IO or waiting on the application thread.
bool sketch_control_poll(SketchControl* bridge, DrawingProgramAppContext* state, bool busy) {
    if (!bridge || !state) return false;
    pthread_mutex_lock(&bridge->mutex);
    if (!bridge->pending || bridge->response || bridge->stopping) {
        pthread_mutex_unlock(&bridge->mutex);
        return false;
    }
    const char* request = bridge->request;
    pthread_mutex_unlock(&bridge->mutex);
    bool changed = false;
    char* response = sketch_control_dispatch(bridge, state, request, busy, &changed);
    if (!response) response = strdup("{\"ok\":false,\"code\":\"allocation_failed\",\"may_have_applied\":true}");
    pthread_mutex_lock(&bridge->mutex);
    bridge->response = response;
    if (!response) bridge->stopping = true;
    pthread_cond_signal(&bridge->ready);
    pthread_mutex_unlock(&bridge->mutex);
    return changed;
}

// Joins the transport before freeing request state and retained mutation receipts.
void sketch_control_stop(SketchControl* bridge) {
    if (!bridge) return;
    pthread_mutex_lock(&bridge->mutex);
    bridge->stopping = true;
    pthread_cond_signal(&bridge->ready);
    pthread_mutex_unlock(&bridge->mutex);
    pthread_join(bridge->thread, NULL);
    close(bridge->listener);
    remove_socket(bridge);
    for (int i = 0; i < bridge->receipt_count; ++i) {
        free(bridge->receipts[i].request);
        free(bridge->receipts[i].response);
    }
    pthread_cond_destroy(&bridge->ready);
    pthread_mutex_destroy(&bridge->mutex);
    free(bridge);
}
