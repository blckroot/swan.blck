#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define STACKSIZE (1920 * 1080)
static char stack[STACKSIZE];
void print_err(char const * const reason)
{
    fprintf(stderr, "Error %s: %s\n", reason, strerror(errno));
}
int exec(void * args)
{
    // Remount proc
    if (mount("proc", "/proc", "proc", 0, "") != 0) {
        print_err("mounting proc");
        return 1;
    }
    // Set a new hostname
    char const * const hostname = "blckswan";
    if (sethostname(hostname, strlen(hostname)) != 0) {
        print_err("setting hostname");
        return 1;
    }
    // Create a message queue
    key_t key = {0};
    if (msgget(key, IPC_CREAT) == -1) {
        print_err("creating message queue");
        return 1;
    }
    // Execute the given command
    char ** const argv = args;
    if (execvp(argv[0], argv) != 0) {
        print_err("executing command");
        return 0;
    }
    return 0;
}
int main(int argc, char ** argv)
{
    // Provide some feedback about the usage
    if (argc < 2) {
        fprintf(stderr, "Hi You called for blckswan?!\n");
        return 0;
    }
    // Namespace flags
    const int flags = CLONE_NEWNET | CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWIPC |
                      CLONE_NEWPID | CLONE_NEWUSER | SIGCHLD;
    // Create a new child process
    pid_t pid = clone(exec, stack + STACKSIZE, flags, &argv[1]);
    if (pid < 0) {
        print_err("calling clone");
        return 0;
    }
    // Wait for the process to finish
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        print_err("waiting for pid");
        return 0;
    }
    // Return the exit code
    return WEXITSTATUS(status);
}
