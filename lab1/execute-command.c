// UCLA CS 111 Lab 1 command execution

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>

/* FIXME: You may need to add #include directives, macro definitions,
 static function definitions, etc.  */

void execute_if_command(command_t,int);
void execute_pipe_command(command_t,int);
void execute_sequence_command(command_t,int);
void execute_simple_command(command_t,int);
void execute_subshell_command(command_t,int);
void execute_until_command(command_t,int);
void execute_while_command(command_t,int);

void
printcmd (int fd, command_t cmd){
    switch (cmd->type) {
        case IF_COMMAND:
            write(fd, "if ", 3);
            printcmd(fd, cmd->u.command[0]);
            write(fd, "then ", 5);
            printcmd(fd, cmd->u.command[1]);
            if (cmd->u.command[2]){
                write(fd, "else ", 5);
                printcmd(fd, cmd->u.command[2]);
                write(fd, "fi ", 3);
            }
            break;
        case PIPE_COMMAND:
            printcmd(fd, cmd->u.command[0]);
            write(fd, "| ", 2);
            printcmd(fd, cmd->u.command[1]);
            break;
        case SEQUENCE_COMMAND:
            printcmd(fd, cmd->u.command[0]);
            write(fd, "; ", 2);
            printcmd(fd, cmd->u.command[1]);
            break;
        case SIMPLE_COMMAND:{
            char **w = cmd->u.word;
            write(fd, *w, strlen(*w));
            write(fd, " ", 1);
            while (*(++w)){
                write(fd, *w, strlen(*w));
                write(fd, " ", 1);
            }
            break;
        }
        case SUBSHELL_COMMAND:
            write(fd, "( ", 2);
            printcmd(fd, cmd->u.command[0]);
            write(fd, ") ", 2);
            break;
        case UNTIL_COMMAND:
            write(fd, "until ", 6);
            printcmd(fd, cmd->u.command[0]);
            write(fd, "do ", 3);
            printcmd(fd, cmd->u.command[1]);
            write(fd, "done ", 5);
            break;
        case WHILE_COMMAND:
            write(fd, "while ", 6);
            printcmd(fd, cmd->u.command[0]);
            write(fd, "do ", 3);
            printcmd(fd, cmd->u.command[1]);
            write(fd, "done ", 5);
            break;
        default:
            break;
    }
    if (cmd->input){
        write(fd, "<", 1);
        write(fd, cmd->input, strlen(cmd->input));
        write(fd, " ", 1);
    }
    if (cmd->output){
        write(fd, ">", 1);
        write(fd, cmd->output, strlen(cmd->output));
        write(fd, " ", 1);
    }
}

double
calctime(double tv_sec, double tv_nsec){
    tv_nsec/=1000000000;
    return tv_sec+tv_nsec;
}

double
calctime2(double tv_sec, double tv_usec){
    tv_usec/=1000000;
    return tv_sec+tv_usec;
}
int
prepare_profiling(char const *name)
{
    /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    //error (0, 0, "warning: profiling not yet implemented");
    int fd;
    fd = open(name, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if(fd < 0) {
        fprintf(stderr, "Cannot write to file: %s", name);
        exit(1);
    }
    return fd;
}

int
command_status(command_t c)
{
    return c->status;
}

void
execute_if_command(command_t c,int fd){
    //execute condition
    struct timespec end_real_time, start_m_time, end_m_time;
    if(clock_gettime(CLOCK_MONOTONIC, &start_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    switch (c->u.command[0]->type)
    {
        case IF_COMMAND: execute_if_command(c->u.command[0],fd); break;
        case PIPE_COMMAND: execute_pipe_command(c->u.command[0],fd); break;
        case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[0],fd); break;
        case SIMPLE_COMMAND: execute_simple_command(c->u.command[0],fd); break;
        case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[0],fd); break;
        case WHILE_COMMAND: execute_while_command(c->u.command[0],fd); break;
        case UNTIL_COMMAND: execute_until_command(c->u.command[0],fd); break;
    }
    //if condition executes successfully, execute the command[1]
    if (c->u.command[0]->status == EXIT_SUCCESS)
    {
        switch (c->u.command[1]->type)
        {
            case IF_COMMAND: execute_if_command(c->u.command[1],fd); break;
            case PIPE_COMMAND: execute_pipe_command(c->u.command[1],fd); break;
            case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[1],fd); break;
            case SIMPLE_COMMAND: execute_simple_command(c->u.command[1],fd); break;
            case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[1],fd); break;
            case WHILE_COMMAND: execute_while_command(c->u.command[1],fd); break;
            case UNTIL_COMMAND: execute_until_command(c->u.command[1],fd); break;
        }
    }
    //if condition executes unsuccessfully, execute the command[2] if it exits, else return 0 (successfully executes if command)
    else
    {
        if (c->u.command[2])
        {
            switch (c->u.command[2]->type)
            {
                case IF_COMMAND: execute_if_command(c->u.command[2],fd); break;
                case PIPE_COMMAND: execute_pipe_command(c->u.command[2],fd); break;
                case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[2],fd); break;
                case SIMPLE_COMMAND: execute_simple_command(c->u.command[2],fd); break;
                case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[2],fd); break;
                case WHILE_COMMAND: execute_while_command(c->u.command[2],fd); break;
                case UNTIL_COMMAND: execute_until_command(c->u.command[2],fd); break;
            }
        }
    }
    if(clock_gettime(CLOCK_REALTIME, &end_real_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    if(clock_gettime(CLOCK_MONOTONIC, &end_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    struct rusage usage;
    //struct rusage c_usage;
    if (getrusage(RUSAGE_SELF, &usage) < 0) {
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    /*
     if (getrusage(RUSAGE_CHILDREN, &c_usage) < 0) {
     fprintf(stderr, "Get Clock Time Error!\n");
     exit(1);
     }
     */
    double end_sec = end_real_time.tv_sec;
    double end_nsec = end_real_time.tv_nsec;
    double end_total = calctime(end_sec, end_nsec);
    double end_m_sec = end_m_time.tv_sec;
    double end_m_nsec = end_m_time.tv_nsec;
    double end_m_total = calctime(end_m_sec, end_m_nsec);
    double start_m_sec = start_m_time.tv_sec;
    double start_m_nsec = start_m_time.tv_nsec;
    double start_m_total = calctime(start_m_sec, start_m_nsec);
    double duration = end_m_total - start_m_total;
    
    double user_cpu_sec = usage.ru_utime.tv_sec;
    double user_cpu_usec = usage.ru_utime.tv_usec;
    double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
    double sys_cpu_sec = usage.ru_stime.tv_sec;
    double sys_cpu_usec = usage.ru_stime.tv_usec;
    double sys_cpu = calctime2(sys_cpu_sec, sys_cpu_usec);
    
    /*printf("%f %f %f %f \n", user_cpu_sec,user_cpu_usec,sys_cpu_sec,sys_cpu_usec);
     
     double user_cpu_secc = c_usage.ru_utime.tv_sec;
     double user_cpu_usecc = c_usage.ru_utime.tv_usec;
     double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
     double sys_cpu_secc = c_usage.ru_stime.tv_sec;
     double sys_cpu_usecc = c_usage.ru_stime.tv_usec;
     
     printf("%f %f %f %f \n", user_cpu_secc,user_cpu_usecc,sys_cpu_secc,sys_cpu_usecc);*/
    
    
    char str[1023];
    sprintf(str, "%.2f ", end_total);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", duration);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", user_cpu);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", sys_cpu);
    write(fd, str, strlen(str));
    printcmd(fd,c);
    write(fd, "[", 1);
    int pid=getpid();
    sprintf(str, "%d", pid);
    write(fd, str, strlen(str));
    write(fd, "]", 1);
    write(fd, "\n", 1);
}

/*void
execute_pipe_command(command_t c)
{
    int status;
    int fd[2];
    if (pipe(fd) < 0)
        error(1, 0, "Pipe failed!\n");
    pid_t pid = fork();
    pid_t pid2 =0 ;
    if (pid < 0)
        error(1, 0, "Fork failed!\n");
    if (pid == 0){
        waitpid(pid+1, &status, 0);
        printf("child1\n");
        close(fd[1]);
        if(dup2(fd[0], 0)<0) error(1,0,"dup2 failed!\n");
        switch (c->u.command[1]->type)
        {
            case IF_COMMAND: execute_if_command(c->u.command[1],fd); break;
            case PIPE_COMMAND: execute_pipe_command(c->u.command[1],fd); break;
            case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[1],fd); break;
            case SIMPLE_COMMAND: execute_simple_command(c->u.command[1],fd); break;
            case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[1],fd); break;
            case WHILE_COMMAND: execute_while_command(c->u.command[1],fd); break;
            case UNTIL_COMMAND: execute_until_command(c->u.command[1],fd); break;
        }
        printf("#child1\n");
    }
    else{
        printf("parent1\n");
        pid2 = fork();
        if (pid2 < 0) error(1, 0, "Fork failed!\n");
        if (pid2 == 0){
            printf("child2\n");
            close(fd[0]);
            if(dup2(fd[1], 1)<0) error(1,0,"dup2 failed!\n");
            switch (c->u.command[0]->type)
            {
                case IF_COMMAND: execute_if_command(c->u.command[0],fd); break;
                case PIPE_COMMAND: execute_pipe_command(c->u.command[0],fd); break;
                case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[0],fd); break;
                case SIMPLE_COMMAND: execute_simple_command(c->u.command[0],fd); break;
                case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[0],fd); break;
                case WHILE_COMMAND: execute_while_command(c->u.command[0],fd); break;
                case UNTIL_COMMAND: execute_until_command(c->u.command[0],fd); break;
            }
            printf("#child2\n");
        }
        else{
            printf("parent2\n");
            close(fd[0]);
            close(fd[1]);
            waitpid(pid2, &status, 0);
            waitpid(pid, &status, 0);
            printf("#parent2\n");
        }
        printf("#parent1\n");
    }
}*/

void
execute_pipe_command(command_t c, int fd2)
{
    struct timespec end_real_time, start_m_time, end_m_time;
    if(clock_gettime(CLOCK_MONOTONIC, &start_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    int status;
    int fd[2];
    if (pipe(fd) < 0)
        error(1, 0, "Pipe failed!\n");
    pid_t pid = fork();
    if (pid < 0)
        error(1, 0, "Fork failed!\n");
    if (pid == 0){
        pid_t pid2 = fork();
        if (pid2 < 0) error(1, 0, "Fork failed!\n");
        if (pid2==0){
            close(fd[0]);
            if(dup2(fd[1], 1)<0) error(1,0,"dup2 failed!\n");
            switch (c->u.command[0]->type)
            {
                    case IF_COMMAND: execute_if_command(c->u.command[0],fd2); break;
                    case PIPE_COMMAND: execute_pipe_command(c->u.command[0],fd2); break;
                    case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[0],fd2); break;
                    case SIMPLE_COMMAND: execute_simple_command(c->u.command[0],fd2); break;
                    case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[0],fd2); break;
                    case WHILE_COMMAND: execute_while_command(c->u.command[0],fd2); break;
                    case UNTIL_COMMAND: execute_until_command(c->u.command[0],fd2); break;
            }
            _exit(c->u.command[0]->status);
        }else{
            waitpid(pid2, &status, 0);
            close(fd[1]);
            if(dup2(fd[0], 0)<0) error(1,0,"dup2 failed!\n");
            switch (c->u.command[1]->type)
            {
                case IF_COMMAND: execute_if_command(c->u.command[1],fd2); break;
                case PIPE_COMMAND: execute_pipe_command(c->u.command[1],fd2); break;
                case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[1],fd2); break;
                case SIMPLE_COMMAND: execute_simple_command(c->u.command[1],fd2); break;
                case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[1],fd2); break;
                case WHILE_COMMAND: execute_while_command(c->u.command[1],fd2); break;
                case UNTIL_COMMAND: execute_until_command(c->u.command[1],fd2); break;
            }
            _exit(c->u.command[1]->status);
        }
    }
    else{
        close(fd[0]);
        close(fd[1]);
        waitpid(pid, &status, 0);
        if(clock_gettime(CLOCK_REALTIME, &end_real_time) < 0){
            fprintf(stderr, "Get Clock Time Error!\n");
            exit(1);
        }
        if(clock_gettime(CLOCK_MONOTONIC, &end_m_time) < 0){
            fprintf(stderr, "Get Clock Time Error!\n");
            exit(1);
        }
        struct rusage usage;
        //struct rusage c_usage;
        if (getrusage(RUSAGE_SELF, &usage) < 0) {
            fprintf(stderr, "Get Clock Time Error!\n");
            exit(1);
        }
        /*
         if (getrusage(RUSAGE_CHILDREN, &c_usage) < 0) {
         fprintf(stderr, "Get Clock Time Error!\n");
         exit(1);
         }
         */
        double end_sec = end_real_time.tv_sec;
        double end_nsec = end_real_time.tv_nsec;
        double end_total = calctime(end_sec, end_nsec);
        double end_m_sec = end_m_time.tv_sec;
        double end_m_nsec = end_m_time.tv_nsec;
        double end_m_total = calctime(end_m_sec, end_m_nsec);
        double start_m_sec = start_m_time.tv_sec;
        double start_m_nsec = start_m_time.tv_nsec;
        double start_m_total = calctime(start_m_sec, start_m_nsec);
        double duration = end_m_total - start_m_total;
        
        double user_cpu_sec = usage.ru_utime.tv_sec;
        double user_cpu_usec = usage.ru_utime.tv_usec;
        double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
        double sys_cpu_sec = usage.ru_stime.tv_sec;
        double sys_cpu_usec = usage.ru_stime.tv_usec;
        double sys_cpu = calctime2(sys_cpu_sec, sys_cpu_usec);
        
        /*printf("%f %f %f %f \n", user_cpu_sec,user_cpu_usec,sys_cpu_sec,sys_cpu_usec);
         
         double user_cpu_secc = c_usage.ru_utime.tv_sec;
         double user_cpu_usecc = c_usage.ru_utime.tv_usec;
         double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
         double sys_cpu_secc = c_usage.ru_stime.tv_sec;
         double sys_cpu_usecc = c_usage.ru_stime.tv_usec;
         
         printf("%f %f %f %f \n", user_cpu_secc,user_cpu_usecc,sys_cpu_secc,sys_cpu_usecc);*/
        
        
        char str[1023];
        sprintf(str, "%.2f ", end_total);
        write(fd2, str, strlen(str));
        sprintf(str, "%.3f ", duration);
        write(fd2, str, strlen(str));
        sprintf(str, "%.3f ", user_cpu);
        write(fd2, str, strlen(str));
        sprintf(str, "%.3f ", sys_cpu);
        write(fd2, str, strlen(str));
        printcmd(fd2,c);
        write(fd2, "[", 1);
        int pid=getpid();
        sprintf(str, "%d", pid);
        write(fd2, str, strlen(str));
        write(fd2, "]", 1);
        write(fd2, "\n", 1);
    }
}



void
execute_sequence_command(command_t c,int fd)
{
    struct timespec end_real_time, start_m_time, end_m_time;
    if(clock_gettime(CLOCK_MONOTONIC, &start_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    switch (c->u.command[0]->type)
    {
        case IF_COMMAND: execute_if_command(c->u.command[0],fd); break;
        case PIPE_COMMAND: execute_pipe_command(c->u.command[0],fd); break;
        case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[0],fd); break;
        case SIMPLE_COMMAND: execute_simple_command(c->u.command[0],fd); break;
        case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[0],fd); break;
        case WHILE_COMMAND: execute_while_command(c->u.command[0],fd); break;
        case UNTIL_COMMAND: execute_until_command(c->u.command[0],fd); break;
    }
    switch (c->u.command[1]->type)
    {
        case IF_COMMAND: execute_if_command(c->u.command[1],fd); break;
        case PIPE_COMMAND: execute_pipe_command(c->u.command[1],fd); break;
        case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[1],fd); break;
        case SIMPLE_COMMAND: execute_simple_command(c->u.command[1],fd); break;
        case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[1],fd); break;
        case WHILE_COMMAND: execute_while_command(c->u.command[1],fd); break;
        case UNTIL_COMMAND: execute_until_command(c->u.command[1],fd); break;
    }
    if(clock_gettime(CLOCK_REALTIME, &end_real_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    if(clock_gettime(CLOCK_MONOTONIC, &end_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    struct rusage usage;
    //struct rusage c_usage;
    if (getrusage(RUSAGE_SELF, &usage) < 0) {
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    /*
     if (getrusage(RUSAGE_CHILDREN, &c_usage) < 0) {
     fprintf(stderr, "Get Clock Time Error!\n");
     exit(1);
     }
     */
    double end_sec = end_real_time.tv_sec;
    double end_nsec = end_real_time.tv_nsec;
    double end_total = calctime(end_sec, end_nsec);
    double end_m_sec = end_m_time.tv_sec;
    double end_m_nsec = end_m_time.tv_nsec;
    double end_m_total = calctime(end_m_sec, end_m_nsec);
    double start_m_sec = start_m_time.tv_sec;
    double start_m_nsec = start_m_time.tv_nsec;
    double start_m_total = calctime(start_m_sec, start_m_nsec);
    double duration = end_m_total - start_m_total;
    
    double user_cpu_sec = usage.ru_utime.tv_sec;
    double user_cpu_usec = usage.ru_utime.tv_usec;
    double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
    double sys_cpu_sec = usage.ru_stime.tv_sec;
    double sys_cpu_usec = usage.ru_stime.tv_usec;
    double sys_cpu = calctime2(sys_cpu_sec, sys_cpu_usec);
    
    /*printf("%f %f %f %f \n", user_cpu_sec,user_cpu_usec,sys_cpu_sec,sys_cpu_usec);
     
     double user_cpu_secc = c_usage.ru_utime.tv_sec;
     double user_cpu_usecc = c_usage.ru_utime.tv_usec;
     double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
     double sys_cpu_secc = c_usage.ru_stime.tv_sec;
     double sys_cpu_usecc = c_usage.ru_stime.tv_usec;
     
     printf("%f %f %f %f \n", user_cpu_secc,user_cpu_usecc,sys_cpu_secc,sys_cpu_usecc);*/
    
    
    char str[1023];
    sprintf(str, "%.2f ", end_total);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", duration);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", user_cpu);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", sys_cpu);
    write(fd, str, strlen(str));
    printcmd(fd,c);
    write(fd, "[", 1);
    int pid=getpid();
    sprintf(str, "%d", pid);
    write(fd, str, strlen(str));
    write(fd, "]", 1);
    write(fd, "\n", 1);
}

void
execute_simple_command(command_t c,int fd)
{
    struct timespec end_real_time, start_m_time, end_m_time;
    if(clock_gettime(CLOCK_MONOTONIC, &start_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    pid_t p = fork();
    int status;
    if (p == 0) {
        if (c->input) {
            int infile;
            if ((infile = open(c->input, O_RDONLY, 0666)) == -1) error(1,0,"Cannnot open file!\n");
            if (dup2(infile, 0) < 0) error(1,0,"I/O redirecting failed!\n");
            close(infile);
        }
        if (c->output) {
            int outfile;
            if ((outfile = open(c->output, O_WRONLY|O_CREAT|O_TRUNC,0666)) == -1) error(1,0,"Cannnot open file!\n");
            if (dup2(outfile, 1) < 0) error(1,0,"I/O redirecting failed!\n");
            close(outfile);
        }
        if (strcmp(c->u.word[0], "exec")!= 0) execvp(c->u.word[0], c->u.word);
        else execvp(c->u.word[1], c->u.word + 1);
    }
    else{
        waitpid(p, &status, 0);
        c->status = WEXITSTATUS(status);
        if(clock_gettime(CLOCK_REALTIME, &end_real_time) < 0){
            fprintf(stderr, "Get Clock Time Error!\n");
            exit(1);
        }
        if(clock_gettime(CLOCK_MONOTONIC, &end_m_time) < 0){
            fprintf(stderr, "Get Clock Time Error!\n");
            exit(1);
        }
        struct rusage usage;
        struct rusage c_usage;
        if (getrusage(RUSAGE_SELF, &usage) < 0) {
            fprintf(stderr, "Get Clock Time Error!\n");
            exit(1);
        }
        if (getrusage(RUSAGE_CHILDREN, &c_usage) < 0) {
            fprintf(stderr, "Get Clock Time Error!\n");
            exit(1);
        }
        double end_sec = end_real_time.tv_sec;
        double end_nsec = end_real_time.tv_nsec;
        double end_total = calctime(end_sec, end_nsec);
        double end_m_sec = end_m_time.tv_sec;
        double end_m_nsec = end_m_time.tv_nsec;
        double end_m_total = calctime(end_m_sec, end_m_nsec);
        double start_m_sec = start_m_time.tv_sec;
        double start_m_nsec = start_m_time.tv_nsec;
        double start_m_total = calctime(start_m_sec, start_m_nsec);
        double duration = end_m_total - start_m_total;
        
        double user_cpu_sec = usage.ru_utime.tv_sec;
        double user_cpu_usec = usage.ru_utime.tv_usec;
        double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
        double sys_cpu_sec = usage.ru_stime.tv_sec;
        double sys_cpu_usec = usage.ru_stime.tv_usec;
        double sys_cpu = calctime2(sys_cpu_sec, sys_cpu_usec);
        
        //printf("parent:%f %f %f %f \n", user_cpu_sec,user_cpu_usec,sys_cpu_sec,sys_cpu_usec);
         
        double user_cpu_secc = c_usage.ru_utime.tv_sec;
        double user_cpu_usecc = c_usage.ru_utime.tv_usec;
         //double user_cpuc = calctime2(user_cpu_secc, user_cpu_usecc);
        double user_cpuc = calctime2(user_cpu_secc, user_cpu_usecc);
        double sys_cpu_secc = c_usage.ru_stime.tv_sec;
        double sys_cpu_usecc = c_usage.ru_stime.tv_usec;
        double sys_cpuc = calctime2(sys_cpu_secc, sys_cpu_usecc);
         //printf("parent:%f %f %f %f \n", user_cpu_secc,user_cpu_usecc,sys_cpu_secc,sys_cpu_usecc);
        
        user_cpu += user_cpuc;
        sys_cpu +=sys_cpuc;
        
        char str[1023];
        sprintf(str, "%.2f ", end_total);
        write(fd, str, strlen(str));
        sprintf(str, "%.3f ", duration);
        write(fd, str, strlen(str));
        sprintf(str, "%.3f ", user_cpu);
        write(fd, str, strlen(str));
        sprintf(str, "%.3f ", sys_cpu);
        write(fd, str, strlen(str));
        printcmd(fd,c);
        write(fd, "[", 1);
        int pid=getpid();
        sprintf(str, "%d", pid);
        write(fd, str, strlen(str));
        write(fd, "]", 1);
        write(fd, "\n", 1);
    }
}

void
execute_subshell_command(command_t c, int fd)
{
    struct timespec end_real_time, start_m_time, end_m_time;
    if(clock_gettime(CLOCK_MONOTONIC, &start_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    if(c->input){
        c->u.command[0]->input=c->input;
    }
    if(c->output){
        c->u.command[0]->output=c->output;
    }
    switch (c->u.command[0]->type)
    {
        case IF_COMMAND:  execute_if_command(c->u.command[0],fd); break;
        case PIPE_COMMAND:  execute_pipe_command(c->u.command[0],fd); break;
        case SEQUENCE_COMMAND:  execute_sequence_command(c->u.command[0],fd); break;
        case SIMPLE_COMMAND:  execute_simple_command(c->u.command[0],fd); break;
        case SUBSHELL_COMMAND:  execute_subshell_command(c->u.command[0],fd); break;
        case WHILE_COMMAND:  execute_while_command(c->u.command[0],fd); break;
        case UNTIL_COMMAND:  execute_until_command(c->u.command[0],fd); break;
    }
    if(clock_gettime(CLOCK_REALTIME, &end_real_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    if(clock_gettime(CLOCK_MONOTONIC, &end_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    struct rusage usage;
    //struct rusage c_usage;
    if (getrusage(RUSAGE_SELF, &usage) < 0) {
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    /*
     if (getrusage(RUSAGE_CHILDREN, &c_usage) < 0) {
     fprintf(stderr, "Get Clock Time Error!\n");
     exit(1);
     }
     */
    double end_sec = end_real_time.tv_sec;
    double end_nsec = end_real_time.tv_nsec;
    double end_total = calctime(end_sec, end_nsec);
    double end_m_sec = end_m_time.tv_sec;
    double end_m_nsec = end_m_time.tv_nsec;
    double end_m_total = calctime(end_m_sec, end_m_nsec);
    double start_m_sec = start_m_time.tv_sec;
    double start_m_nsec = start_m_time.tv_nsec;
    double start_m_total = calctime(start_m_sec, start_m_nsec);
    double duration = end_m_total - start_m_total;
    
    double user_cpu_sec = usage.ru_utime.tv_sec;
    double user_cpu_usec = usage.ru_utime.tv_usec;
    double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
    double sys_cpu_sec = usage.ru_stime.tv_sec;
    double sys_cpu_usec = usage.ru_stime.tv_usec;
    double sys_cpu = calctime2(sys_cpu_sec, sys_cpu_usec);
    
    /*printf("%f %f %f %f \n", user_cpu_sec,user_cpu_usec,sys_cpu_sec,sys_cpu_usec);
     
     double user_cpu_secc = c_usage.ru_utime.tv_sec;
     double user_cpu_usecc = c_usage.ru_utime.tv_usec;
     double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
     double sys_cpu_secc = c_usage.ru_stime.tv_sec;
     double sys_cpu_usecc = c_usage.ru_stime.tv_usec;
     
     printf("%f %f %f %f \n", user_cpu_secc,user_cpu_usecc,sys_cpu_secc,sys_cpu_usecc);*/
    
    
    char str[1023];
    sprintf(str, "%.2f ", end_total);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", duration);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", user_cpu);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", sys_cpu);
    write(fd, str, strlen(str));
    printcmd(fd,c);
    write(fd, "[", 1);
    int pid=getpid();
    sprintf(str, "%d", pid);
    write(fd, str, strlen(str));
    write(fd, "]", 1);
    write(fd, "\n", 1);
}

void
execute_until_command(command_t c, int fd)
{
    struct timespec end_real_time, start_m_time, end_m_time;
    if(clock_gettime(CLOCK_MONOTONIC, &start_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    //execute condition
    switch (c->u.command[0]->type)
    {
        case IF_COMMAND:  execute_if_command(c->u.command[0],fd); break;
        case PIPE_COMMAND:  execute_pipe_command(c->u.command[0],fd); break;
        case SEQUENCE_COMMAND:  execute_sequence_command(c->u.command[0],fd); break;
        case SIMPLE_COMMAND:  execute_simple_command(c->u.command[0],fd); break;
        case SUBSHELL_COMMAND:  execute_subshell_command(c->u.command[0],fd); break;
        case WHILE_COMMAND:  execute_while_command(c->u.command[0],fd); break;
        case UNTIL_COMMAND:  execute_until_command(c->u.command[0],fd); break;
    }
    while (c->u.command[0]->status != EXIT_SUCCESS)
    {
        switch (c->u.command[1]->type)
        {
            case IF_COMMAND:  execute_if_command(c->u.command[1],fd); break;
            case PIPE_COMMAND:  execute_pipe_command(c->u.command[1],fd); break;
            case SEQUENCE_COMMAND:  execute_sequence_command(c->u.command[1],fd); break;
            case SIMPLE_COMMAND:  execute_simple_command(c->u.command[1],fd); break;
            case SUBSHELL_COMMAND:  execute_subshell_command(c->u.command[1],fd); break;
            case WHILE_COMMAND:  execute_while_command(c->u.command[1],fd); break;
            case UNTIL_COMMAND:  execute_until_command(c->u.command[1],fd); break;
        }
        switch (c->u.command[0]->type)
        {
            case IF_COMMAND:  execute_if_command(c->u.command[0],fd); break;
            case PIPE_COMMAND:  execute_pipe_command(c->u.command[0],fd); break;
            case SEQUENCE_COMMAND:  execute_sequence_command(c->u.command[0],fd); break;
            case SIMPLE_COMMAND:  execute_simple_command(c->u.command[0],fd); break;
            case SUBSHELL_COMMAND:  execute_subshell_command(c->u.command[0],fd); break;
            case WHILE_COMMAND:  execute_while_command(c->u.command[0],fd); break;
            case UNTIL_COMMAND:  execute_until_command(c->u.command[0],fd); break;
        }
    }
    if(clock_gettime(CLOCK_REALTIME, &end_real_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    if(clock_gettime(CLOCK_MONOTONIC, &end_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    struct rusage usage;
    //struct rusage c_usage;
    if (getrusage(RUSAGE_SELF, &usage) < 0) {
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    /*
     if (getrusage(RUSAGE_CHILDREN, &c_usage) < 0) {
     fprintf(stderr, "Get Clock Time Error!\n");
     exit(1);
     }
     */
    double end_sec = end_real_time.tv_sec;
    double end_nsec = end_real_time.tv_nsec;
    double end_total = calctime(end_sec, end_nsec);
    double end_m_sec = end_m_time.tv_sec;
    double end_m_nsec = end_m_time.tv_nsec;
    double end_m_total = calctime(end_m_sec, end_m_nsec);
    double start_m_sec = start_m_time.tv_sec;
    double start_m_nsec = start_m_time.tv_nsec;
    double start_m_total = calctime(start_m_sec, start_m_nsec);
    double duration = end_m_total - start_m_total;
    
    double user_cpu_sec = usage.ru_utime.tv_sec;
    double user_cpu_usec = usage.ru_utime.tv_usec;
    double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
    double sys_cpu_sec = usage.ru_stime.tv_sec;
    double sys_cpu_usec = usage.ru_stime.tv_usec;
    double sys_cpu = calctime2(sys_cpu_sec, sys_cpu_usec);
    
    /*printf("%f %f %f %f \n", user_cpu_sec,user_cpu_usec,sys_cpu_sec,sys_cpu_usec);
     
     double user_cpu_secc = c_usage.ru_utime.tv_sec;
     double user_cpu_usecc = c_usage.ru_utime.tv_usec;
     double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
     double sys_cpu_secc = c_usage.ru_stime.tv_sec;
     double sys_cpu_usecc = c_usage.ru_stime.tv_usec;
     
     printf("%f %f %f %f \n", user_cpu_secc,user_cpu_usecc,sys_cpu_secc,sys_cpu_usecc);*/
    
    
    char str[1023];
    sprintf(str, "%.2f ", end_total);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", duration);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", user_cpu);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", sys_cpu);
    write(fd, str, strlen(str));
    printcmd(fd,c);
    write(fd, "[", 1);
    int pid=getpid();
    sprintf(str, "%d", pid);
    write(fd, str, strlen(str));
    write(fd, "]", 1);
    write(fd, "\n", 1);
}

void
execute_while_command(command_t c, int fd)
{
    struct timespec end_real_time, start_m_time, end_m_time;
    if(clock_gettime(CLOCK_MONOTONIC, &start_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    //execute condition
    switch (c->u.command[0]->type)
    {
        case IF_COMMAND:  execute_if_command(c->u.command[0],fd); break;
        case PIPE_COMMAND:  execute_pipe_command(c->u.command[0],fd); break;
        case SEQUENCE_COMMAND:  execute_sequence_command(c->u.command[0],fd); break;
        case SIMPLE_COMMAND:  execute_simple_command(c->u.command[0],fd); break;
        case SUBSHELL_COMMAND:  execute_subshell_command(c->u.command[0],fd); break;
        case UNTIL_COMMAND:  execute_until_command(c->u.command[0],fd); break;
        case WHILE_COMMAND:  execute_while_command(c->u.command[0],fd); break;
    }
    while (c->u.command[0]->status == EXIT_SUCCESS)
    {
        switch (c->u.command[1]->type)
        {
            case IF_COMMAND: execute_if_command(c->u.command[1],fd); break;
            case PIPE_COMMAND: execute_pipe_command(c->u.command[1],fd); break;
            case SEQUENCE_COMMAND: execute_sequence_command(c->u.command[1],fd); break;
            case SIMPLE_COMMAND: execute_simple_command(c->u.command[1],fd); break;
            case SUBSHELL_COMMAND: execute_subshell_command(c->u.command[1],fd); break;
            case UNTIL_COMMAND: execute_until_command(c->u.command[1],fd); break;
            case WHILE_COMMAND: execute_while_command(c->u.command[1],fd); break;
        }
        switch (c->u.command[0]->type)
        {
            case IF_COMMAND: execute_if_command(c->u.command[0],fd); break;
            case PIPE_COMMAND: execute_pipe_command(c->u.command[0],fd); break;
            case SEQUENCE_COMMAND:  execute_sequence_command(c->u.command[0],fd); break;
            case SIMPLE_COMMAND:  execute_simple_command(c->u.command[0],fd); break;
            case SUBSHELL_COMMAND:  execute_subshell_command(c->u.command[0],fd); break;
            case UNTIL_COMMAND:  execute_until_command(c->u.command[0],fd); break;
            case WHILE_COMMAND:  execute_while_command(c->u.command[0],fd); break;
        }
    }
    if(clock_gettime(CLOCK_REALTIME, &end_real_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    if(clock_gettime(CLOCK_MONOTONIC, &end_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    struct rusage usage;
    //struct rusage c_usage;
    if (getrusage(RUSAGE_SELF, &usage) < 0) {
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    /*
     if (getrusage(RUSAGE_CHILDREN, &c_usage) < 0) {
     fprintf(stderr, "Get Clock Time Error!\n");
     exit(1);
     }
     */
    double end_sec = end_real_time.tv_sec;
    double end_nsec = end_real_time.tv_nsec;
    double end_total = calctime(end_sec, end_nsec);
    double end_m_sec = end_m_time.tv_sec;
    double end_m_nsec = end_m_time.tv_nsec;
    double end_m_total = calctime(end_m_sec, end_m_nsec);
    double start_m_sec = start_m_time.tv_sec;
    double start_m_nsec = start_m_time.tv_nsec;
    double start_m_total = calctime(start_m_sec, start_m_nsec);
    double duration = end_m_total - start_m_total;
    
    double user_cpu_sec = usage.ru_utime.tv_sec;
    double user_cpu_usec = usage.ru_utime.tv_usec;
    double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
    double sys_cpu_sec = usage.ru_stime.tv_sec;
    double sys_cpu_usec = usage.ru_stime.tv_usec;
    double sys_cpu = calctime2(sys_cpu_sec, sys_cpu_usec);
    
    /*printf("%f %f %f %f \n", user_cpu_sec,user_cpu_usec,sys_cpu_sec,sys_cpu_usec);
     
     double user_cpu_secc = c_usage.ru_utime.tv_sec;
     double user_cpu_usecc = c_usage.ru_utime.tv_usec;
     double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
     double sys_cpu_secc = c_usage.ru_stime.tv_sec;
     double sys_cpu_usecc = c_usage.ru_stime.tv_usec;
     
     printf("%f %f %f %f \n", user_cpu_secc,user_cpu_usecc,sys_cpu_secc,sys_cpu_usecc);*/
    
    
    char str[1023];
    sprintf(str, "%.2f ", end_total);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", duration);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", user_cpu);
    write(fd, str, strlen(str));
    sprintf(str, "%.3f ", sys_cpu);
    write(fd, str, strlen(str));
    printcmd(fd,c);
    write(fd, "[", 1);
    int pid=getpid();
    sprintf(str, "%d", pid);
    write(fd, str, strlen(str));
    write(fd, "]", 1);
    write(fd, "\n", 1);
}


void
execute_command(command_t c, int profiling)
{
    //profiling = -1; //Verbose
    //int fd[2];
    //if (pipe(fd) < 0)
    //    error(1, 0, "Pipe failed!\n");
    /*struct timespec end_real_time, start_m_time, end_m_time;
    if(clock_gettime(CLOCK_MONOTONIC, &start_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }*/
    switch (c->type) {
        case IF_COMMAND:
            execute_if_command(c, profiling);
            break;
        case PIPE_COMMAND:
            execute_pipe_command(c, profiling);
            break;
        case SEQUENCE_COMMAND:
            execute_sequence_command(c, profiling);
            break;
        case SIMPLE_COMMAND:
            execute_simple_command(c, profiling);
            break;
        case SUBSHELL_COMMAND:
            execute_subshell_command(c, profiling);
            break;
        case WHILE_COMMAND:
            execute_while_command(c, profiling);
            break;
        case UNTIL_COMMAND:
            execute_until_command(c, profiling);
            break;
        default:
            error(1,0,"Undefined Command Type!\n");
            break;
    }
    /*if(clock_gettime(CLOCK_REALTIME, &end_real_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    if(clock_gettime(CLOCK_MONOTONIC, &end_m_time) < 0){
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    struct rusage usage;
    //struct rusage c_usage;
    if (getrusage(RUSAGE_SELF, &usage) < 0) {
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    if (getrusage(RUSAGE_CHILDREN, &c_usage) < 0) {
        fprintf(stderr, "Get Clock Time Error!\n");
        exit(1);
    }
    
    double end_sec = end_real_time.tv_sec;
    double end_nsec = end_real_time.tv_nsec;
    double end_total = calctime(end_sec, end_nsec);
    double end_m_sec = end_m_time.tv_sec;
    double end_m_nsec = end_m_time.tv_nsec;
    double end_m_total = calctime(end_m_sec, end_m_nsec);
    double start_m_sec = start_m_time.tv_sec;
    double start_m_nsec = start_m_time.tv_nsec;
    double start_m_total = calctime(start_m_sec, start_m_nsec);
    double duration = end_m_total - start_m_total;
    
    double user_cpu_sec = usage.ru_utime.tv_sec;
    double user_cpu_usec = usage.ru_utime.tv_usec;
    double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
    double sys_cpu_sec = usage.ru_stime.tv_sec;
    double sys_cpu_usec = usage.ru_stime.tv_usec;
    double sys_cpu = calctime2(sys_cpu_sec, sys_cpu_usec);
    
    printf("%f %f %f %f \n", user_cpu_sec,user_cpu_usec,sys_cpu_sec,sys_cpu_usec);
    
    double user_cpu_secc = c_usage.ru_utime.tv_sec;
    double user_cpu_usecc = c_usage.ru_utime.tv_usec;
    double user_cpu = calctime2(user_cpu_sec, user_cpu_usec);
    double sys_cpu_secc = c_usage.ru_stime.tv_sec;
    double sys_cpu_usecc = c_usage.ru_stime.tv_usec;
    
    printf("%f %f %f %f \n", user_cpu_secc,user_cpu_usecc,sys_cpu_secc,sys_cpu_usecc);
    
    
    char str[1023];
    sprintf(str, "%.2f ", end_total);
    write(profiling, str, strlen(str));
    sprintf(str, "%.3f ", duration);
    write(profiling, str, strlen(str));
    sprintf(str, "%.3f ", user_cpu);
    write(profiling, str, strlen(str));
    sprintf(str, "%.3f ", sys_cpu);
    write(profiling, str, strlen(str));
    printcmd(profiling,c);
    write(profiling, "[", 1);
    int pid=getpid();
    sprintf(str, "%d", pid);
    write(profiling, str, strlen(str));
    write(profiling, "]", 1);
    write(profiling, "\n", 1);*/
    //execute_simple_command(c);
    //c->status=-1;
    //profiling=0;
    /* FIXME: Replace this with your implementation, like 'prepare_profiling'.  */
    //error (1, 0, "command execution not yet implemented");
}