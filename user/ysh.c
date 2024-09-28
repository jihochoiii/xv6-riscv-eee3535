#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define buf_size    128     // Max length of user input
#define max_args    16      // Max number of arguments

int runcmd(char *cmd);      // Run a command.

// Read a shell input.
char* readcmd(char *buf) {
    // Read an input from stdin.
    fprintf(1, "$ ");
    memset(buf, 0, buf_size);
    char *cmd = gets(buf, buf_size);

    // Chop off the trailing '\n'.
    if(cmd) { cmd[strlen(cmd)-1] = 0; }

    return cmd;
}

int main(int argc, char **argv) {
    int fd = 0;
    char *cmd = 0;
    char buf[buf_size];

    // Ensure three file descriptors are open.
    while((fd = open("console", O_RDWR)) >= 0) {
        if(fd >= 3) { close(fd); break; }
    }

    fprintf(1, "EEE3535 Operating Systems: starting ysh\n");

    // Read and run input commands.
    while((cmd = readcmd(buf)) && runcmd(cmd)) ;

    fprintf(1, "EEE3535 Operating Systems: closing ysh\n");
    exit(0);
}


void pipe_commands(char **argv);              // Handle commands with pipes.

// Run a command.
int runcmd(char *cmd) {
    if(!*cmd) { return 1; }                   // Empty command

    // Skip leading white spaces.
    while(*cmd == ' ') { cmd++; }
    // Remove trailing white spaces.
    for(char *c = cmd+strlen(cmd)-1; *c == ' '; c--) { *c = 0; }

    if(!strcmp(cmd, "exit")) { return 0; }    // exit command
    else if(!strncmp(cmd, "cd ", 3)) {        // cd command
        if(chdir(cmd+3) < 0) { fprintf(2, "Cannot cd %s\n", cmd+3); }
    }
    else {
        // EEE3535 Operating Systems
        // Assignment 1: Shell

        // Check whether the input is commands in a series (i.e., contains ";").
        char *cmd_series_pos = strchr(cmd, ';');
        
        // Commands in a series
        if(cmd_series_pos) {
            *cmd_series_pos = 0;

            // Execute the command before the semicolon.
            if(!runcmd(cmd)) { return 0; }
            // Execute the command after the semicolon.
            if(!runcmd(++cmd_series_pos)) { return 0; }
        }

        // A single command (including background, redirection, pipe)
        else {
            // Check whether the command is a background command (i.e., contains "&").
            int background_flag = 0;
            char *cmd_bg_pos = strchr(cmd, '&');

            if(cmd_bg_pos) {
                background_flag = 1;    // If the command contains "&", set flag = 1.
                *cmd_bg_pos = 0;
            }

            // Check whether the command output should be redirected to a file (i.e., the command contains ">").
            char *cmd_rdr_pos = strchr(cmd, '>');

            int pid = fork();
            if(pid < 0) { fprintf(2, "fork() failed\n"); exit(1); }
            // Parent process
            else if(pid > 0) {
                // The parent process waits for the child to finish if the command is not a background command.
                if(!background_flag) { wait(0); }
            }
            // Child process
            else {
                // Split the command-line arguments.
                char *argv[max_args];
                char *cmd_space_pos = cmd;
                char *space;
                int i = 1;

                argv[0] = cmd;
                while((space = strchr(cmd_space_pos, ' '))) {
                    *(space++) = 0;
                    while(*space == ' ') { space++; }    // Skip leading white spaces.
                    argv[i++] = space;
                    cmd_space_pos = space;
                }
                argv[i] = 0;

                // Redirect the output to a file.
                if(cmd_rdr_pos) {
                    // Get the command & file name.
                    int k = 0;
                    while(strcmp(argv[k], ">")) { k++; }
                    argv[k] = 0;
                    char *file_name = argv[++k];

                    close(1);                                          // Close the standard output.
                    open(file_name, O_WRONLY | O_CREATE | O_TRUNC);    // Redirect the output to a file.
                }

                // Handle the pipes, if they exist.
                pipe_commands(argv);

                // Execute the command.
                exec(argv[0], argv);
            }
        }
    }
    return 1;
}

// Commands with pipes
void pipe_commands(char **argv) {
    // Check whether the command contains "|".
    int pipe_flag = 0;
    int i = 0;
    while(argv[i]) {
        if(!strcmp(argv[i], "|")) {
            pipe_flag = 1;    // If the command contains "|", set flag = 1.
            argv[i++] = 0;
            break;
        }
        i++;
    }

    if(pipe_flag) {
        // Create a pipe.
        // It returns two file descriptors, fd[0] for a read end and fd[1] for a write end.
        int fd[2];
        if(pipe(fd)) { fprintf(2, "pipe() failed\n"); exit(1); }

        int pid = fork();
        if(pid < 0) { fprintf(2, "fork() failed\n"); exit(1); }
        // Write pipe (child process)
        else if(!pid) {
            close(1);                      // Close the standard output.
            dup(fd[1]);                    // Bind the write end of the pipe to FD = 1.
            close(fd[0]); close(fd[1]);    // Close unused file descriptors.
        }
        // Read pipe (parent process)
        else {
            close(0);                      // Close the standard input.
            dup(fd[0]);                    // Bind the read end of the pipe to FD = 0.
            close(fd[0]); close(fd[1]);    // Close unused file descriptors.

            int k = 0;
            while(argv[i]) {
                argv[k++] = argv[i++];
            }
            argv[k] = 0;
            pipe_commands(argv);
        }
    }
}
