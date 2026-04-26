#include "drawing_program/drawing_program_native_dialogs.h"

#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static CoreResult native_dialog_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

static CoreResult native_dialog_io_error(const char *message) {
    return (CoreResult){ CORE_ERR_IO, message };
}

static int native_dialog_escape_applescript_string(const char *src, char *out, size_t out_cap) {
    size_t write_index = 0u;
    size_t i;
    if (!src || !out || out_cap == 0u) {
        return 0;
    }
    for (i = 0u; src[i] != '\0'; ++i) {
        char ch = src[i];
        if (ch == '"' || ch == '\\') {
            if (write_index + 2u >= out_cap) {
                return 0;
            }
            out[write_index++] = '\\';
            out[write_index++] = ch;
            continue;
        }
        if ((unsigned char)ch < 32u) {
            if (write_index + 1u >= out_cap) {
                return 0;
            }
            out[write_index++] = ' ';
            continue;
        }
        if (write_index + 1u >= out_cap) {
            return 0;
        }
        out[write_index++] = ch;
    }
    out[write_index] = '\0';
    return 1;
}

static void native_dialog_trim_path(char *path) {
    size_t len = 0u;
    if (!path) {
        return;
    }
    len = strlen(path);
    while (len > 0u && (path[len - 1u] == '\n' || path[len - 1u] == '\r' || path[len - 1u] == ' ')) {
        path[--len] = '\0';
    }
    while (len > 1u && path[len - 1u] == '/') {
        path[--len] = '\0';
    }
}

static CoreResult native_dialog_run_applescript_pick(const char *script_pick,
                                                     const char *script_result,
                                                     char *out_path,
                                                     size_t out_cap,
                                                     const char *spawn_error_message,
                                                     const char *wait_error_message,
                                                     const char *cancelled_error_message) {
#if defined(__APPLE__)
    int pipe_fds[2] = { -1, -1 };
    posix_spawn_file_actions_t actions;
    pid_t child_pid = (pid_t)0;
    int status = 0;
    char *argv[7];
    ssize_t bytes_read = 0;
    size_t total_read = 0u;
    int spawn_result = 0;
    out_path[0] = '\0';
    if (!script_pick || !script_result || !out_path || out_cap == 0u) {
        return native_dialog_invalid("invalid native dialog request");
    }
    if (pipe(pipe_fds) != 0) {
        return native_dialog_io_error("failed to create native dialog pipe");
    }
    if (posix_spawn_file_actions_init(&actions) != 0) {
        (void)close(pipe_fds[0]);
        (void)close(pipe_fds[1]);
        return native_dialog_io_error("failed to initialize native dialog actions");
    }
    (void)posix_spawn_file_actions_adddup2(&actions, pipe_fds[1], STDOUT_FILENO);
    (void)posix_spawn_file_actions_addclose(&actions, pipe_fds[0]);
    (void)posix_spawn_file_actions_addclose(&actions, pipe_fds[1]);
    argv[0] = "/usr/bin/osascript";
    argv[1] = "-e";
    argv[2] = (char *)script_pick;
    argv[3] = "-e";
    argv[4] = (char *)script_result;
    argv[5] = 0;
    spawn_result = posix_spawn(&child_pid, "/usr/bin/osascript", &actions, 0, argv, environ);
    (void)posix_spawn_file_actions_destroy(&actions);
    (void)close(pipe_fds[1]);
    if (spawn_result != 0) {
        (void)close(pipe_fds[0]);
        return native_dialog_io_error(spawn_error_message);
    }
    while ((bytes_read = read(pipe_fds[0], out_path + total_read, out_cap - total_read - 1u)) > 0) {
        total_read += (size_t)bytes_read;
        if (total_read >= out_cap - 1u) {
            break;
        }
    }
    (void)close(pipe_fds[0]);
    out_path[total_read] = '\0';
    if (waitpid(child_pid, &status, 0) < 0) {
        out_path[0] = '\0';
        return native_dialog_io_error(wait_error_message);
    }
    native_dialog_trim_path(out_path);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || out_path[0] == '\0') {
        out_path[0] = '\0';
        return native_dialog_io_error(cancelled_error_message);
    }
    return core_result_ok();
#else
    (void)script_pick;
    (void)script_result;
    (void)out_path;
    (void)out_cap;
    (void)spawn_error_message;
    (void)wait_error_message;
    (void)cancelled_error_message;
    return native_dialog_io_error("native dialogs only supported on macOS");
#endif
}

CoreResult drawing_program_native_choose_directory(const char *prompt,
                                                   const char *initial_dir,
                                                   char *out_path,
                                                   size_t out_cap) {
#if defined(__APPLE__)
    char escaped_prompt[256];
    char escaped_initial_dir[768];
    char script_pick[1200];
    if (!prompt || !out_path || out_cap == 0u) {
        return native_dialog_invalid("invalid native directory chooser request");
    }
    if (!native_dialog_escape_applescript_string(prompt, escaped_prompt, sizeof(escaped_prompt))) {
        return native_dialog_invalid("directory chooser prompt too long");
    }
    if (initial_dir && initial_dir[0] != '\0') {
        if (!native_dialog_escape_applescript_string(initial_dir, escaped_initial_dir, sizeof(escaped_initial_dir))) {
            return native_dialog_invalid("directory chooser initial path too long");
        }
        (void)snprintf(script_pick,
                       sizeof(script_pick),
                       "set chosenFolder to choose folder with prompt \"%s\" default location POSIX file \"%s\"",
                       escaped_prompt,
                       escaped_initial_dir);
    } else {
        (void)snprintf(script_pick,
                       sizeof(script_pick),
                       "set chosenFolder to choose folder with prompt \"%s\"",
                       escaped_prompt);
    }
    return native_dialog_run_applescript_pick(script_pick,
                                              "POSIX path of chosenFolder",
                                              out_path,
                                              out_cap,
                                              "failed to spawn macOS directory chooser",
                                              "failed to wait for macOS directory chooser",
                                              "directory chooser cancelled or failed");
#else
    (void)prompt;
    (void)initial_dir;
    (void)out_path;
    (void)out_cap;
    return native_dialog_io_error("native directory chooser only supported on macOS");
#endif
}

CoreResult drawing_program_native_choose_file(const char *prompt,
                                              const char *initial_dir,
                                              char *out_path,
                                              size_t out_cap) {
#if defined(__APPLE__)
    char escaped_prompt[256];
    char escaped_initial_dir[768];
    char script_pick[1200];
    if (!prompt || !out_path || out_cap == 0u) {
        return native_dialog_invalid("invalid native file chooser request");
    }
    if (!native_dialog_escape_applescript_string(prompt, escaped_prompt, sizeof(escaped_prompt))) {
        return native_dialog_invalid("file chooser prompt too long");
    }
    if (initial_dir && initial_dir[0] != '\0') {
        if (!native_dialog_escape_applescript_string(initial_dir, escaped_initial_dir, sizeof(escaped_initial_dir))) {
            return native_dialog_invalid("file chooser initial path too long");
        }
        (void)snprintf(script_pick,
                       sizeof(script_pick),
                       "set chosenFile to choose file with prompt \"%s\" default location POSIX file \"%s\"",
                       escaped_prompt,
                       escaped_initial_dir);
    } else {
        (void)snprintf(script_pick,
                       sizeof(script_pick),
                       "set chosenFile to choose file with prompt \"%s\"",
                       escaped_prompt);
    }
    return native_dialog_run_applescript_pick(script_pick,
                                              "POSIX path of chosenFile",
                                              out_path,
                                              out_cap,
                                              "failed to spawn macOS file chooser",
                                              "failed to wait for macOS file chooser",
                                              "file chooser cancelled or failed");
#else
    (void)prompt;
    (void)initial_dir;
    (void)out_path;
    (void)out_cap;
    return native_dialog_io_error("native file chooser only supported on macOS");
#endif
}

CoreResult drawing_program_native_choose_save_file(const char *prompt,
                                                   const char *initial_dir,
                                                   const char *default_name,
                                                   char *out_path,
                                                   size_t out_cap) {
#if defined(__APPLE__)
    char escaped_prompt[256];
    char escaped_initial_dir[768];
    char escaped_default_name[256];
    char script_pick[1500];
    if (!prompt || !default_name || !out_path || out_cap == 0u) {
        return native_dialog_invalid("invalid native save chooser request");
    }
    if (!native_dialog_escape_applescript_string(prompt, escaped_prompt, sizeof(escaped_prompt))) {
        return native_dialog_invalid("save chooser prompt too long");
    }
    if (!native_dialog_escape_applescript_string(default_name, escaped_default_name, sizeof(escaped_default_name))) {
        return native_dialog_invalid("save chooser default name too long");
    }
    if (initial_dir && initial_dir[0] != '\0') {
        if (!native_dialog_escape_applescript_string(initial_dir, escaped_initial_dir, sizeof(escaped_initial_dir))) {
            return native_dialog_invalid("save chooser initial path too long");
        }
        (void)snprintf(script_pick,
                       sizeof(script_pick),
                       "set chosenFile to choose file name with prompt \"%s\" default location POSIX file \"%s\" default name \"%s\"",
                       escaped_prompt,
                       escaped_initial_dir,
                       escaped_default_name);
    } else {
        (void)snprintf(script_pick,
                       sizeof(script_pick),
                       "set chosenFile to choose file name with prompt \"%s\" default name \"%s\"",
                       escaped_prompt,
                       escaped_default_name);
    }
    return native_dialog_run_applescript_pick(script_pick,
                                              "POSIX path of chosenFile",
                                              out_path,
                                              out_cap,
                                              "failed to spawn macOS save chooser",
                                              "failed to wait for macOS save chooser",
                                              "save chooser cancelled or failed");
#else
    (void)prompt;
    (void)initial_dir;
    (void)default_name;
    (void)out_path;
    (void)out_cap;
    return native_dialog_io_error("native save chooser only supported on macOS");
#endif
}
