#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<wait.h>
#include<fcntl.h>
#include<limits.h>
#include<errno.h>
#include<stdbool.h>
#include<signal.h>
#include<setjmp.h>

#define CHUNK_SIZ 10
#define INPUT_SIZ 1025
#define MAX_PARAMS 100
#define HOME "/home/"
#define USER "yatn"

#ifndef USER
#define USER getlogin()
#endif

char* get_newd(char* current_dir, char* token);
void ctrl_c_handler(int sig);
bool is_active(int* fd);

jmp_buf env_buffer;

int main(int argc, char* argv[]){
    char* current_dir = (char*) malloc(PATH_MAX * sizeof(char));
    char* input = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* token = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* command = (char*) malloc(INPUT_SIZ * sizeof(char));
    int fd_in = 0, fd_out = 1;
    int pipe_fd[2];

    bool bckgnd_exec = false;

    current_dir = getcwd(current_dir, PATH_MAX);
    if(current_dir == NULL){
        perror("Error in fetching current directory : ");
        exit(1);
    }

    signal(SIGINT, ctrl_c_handler);

    while(1){
        if(argc < 2 || strcmp(argv[1], "source") != 0){
            // avoid printing when executing source
            printf("user@shell: ");
        }
        fd_in = 0;
        fd_out = 1;
        pipe_fd[0] = pipe_fd[1] = -1;

        fgets(input, INPUT_SIZ, stdin);
        command = strtok(input, " \n");
        if(command == NULL && strlen(input) != 0){
            // user pressed enter
            continue;
        }
        do{
            bckgnd_exec = false;

            if(strcmp(command, "exit") == 0){
                exit(0);
            }
            else if(strcmp(command, ";") == 0){
                fd_in = 0;
                fd_out = 1;
                // reset pipe
                pipe_fd[0] = pipe_fd[1] = -1;
                continue;
            }
            else if(strcmp(command, "cd") == 0){
                token = strtok(NULL, " \n");
                
                if(token == NULL){
                    token = (char*) malloc(PATH_MAX * sizeof(char));
                    token = strcat(strcat(token, HOME), USER);
                }

                if(strcmp(token, ";") == 0){
                    token = NULL;
                }
                
                if(chdir(token) == -1){
                    perror("Error in cd ");
                    continue;
                }

                current_dir = getcwd(current_dir, PATH_MAX);
            }
            else if(strcmp(command, "pwd") == 0){
                printf("%s\n", current_dir);
            }
            else if(strcmp(command, "source") == 0){
                token = strtok(NULL, " \n");
                
                if(token == NULL){
                    fprintf(stderr, "bash: source: filename argument required\n");
                    continue;
                }
                if(strlen(token) == 1 && token[0] == '<'){
                    token = strtok(NULL, " \n");
                    if(token == NULL || (strlen(token) == 1 && token[0] == ';')){
                        fprintf(stderr, "source: syntax error near <\n");
                        continue;
                    }
                    fd_in = open(token, O_RDONLY);
                    token = strtok(NULL, " \n");
                }
                else{
                    fd_in = open(token, O_RDONLY);
                    token = strtok(NULL, " \n");
                }
                if(token != NULL && strlen(token) == 1 && token[0] == '>'){
                    token = strtok(NULL, " \n");
                    if(token == NULL || (strlen(token) == 1 && token[0] == ';')){
                        fprintf(stderr, "source: syntax error near >\n");
                        continue;
                    }
                    fd_out = creat(token, (1 << 9) - 1);
                    token = strtok(NULL, " \n");
                }
                
                if(token != NULL && strlen(token) == 1 && strcmp(token, "&") == 0){
                    bckgnd_exec = true;
                }
                if(fd_in == -1 || fd_out == -1){
                    perror("Error in opening file ");
                    continue;
                }
                if(fork() > 0){
                    if(!bckgnd_exec){
                        int status;
                        wait(&status);
                    }
                }
                else{
                    if(fd_in != 0){
                        dup2(fd_in, 0);
                        close(fd_in);
                    }
                    if(fd_out != 1){
                        dup2(fd_out, 1);
                        close(fd_out);
                    }
                    execl("shell", "shell", "source", NULL);
                }
            }
            else if(strcmp(command, "echo") == 0){
                // no support for I/O redirection
                command = strtok(NULL, ";\n");
                printf("%s", command);
                printf("\n");
                
            }
            else{
                // run in background
                char *params[MAX_PARAMS];
                int count = 0;
                token = command;
                do{
                    //printf("TOKEN : %s\n", token);
                    if(strlen(token) == 1 && token[0] == ';'){
                        break;
                    }
                    else if(strlen(token) == 1 && token[0] == '|'){
                        if(is_active(pipe_fd)){
                            // read from old pipe
                            fd_in = pipe_fd[0];
                        }
                        if(pipe(pipe_fd) == -1){
                            perror("error in piping ");
                            break;
                        }
                        // always write to new pipe
                        fd_out = pipe_fd[1];
                        break;
                    }
                    else if(strlen(token) == 1 && token[0] == '&'){
                        bckgnd_exec = true;
                        break;
                    }
                    else if(strlen(token) == 1 && token[0] == '<'){
                        token = strtok(NULL, " \n");
                        if(token == NULL || (strlen(token) == 1 && token[0] == ';')){
                            break;
                        }
                        fd_in = open(token, O_RDONLY);
                    }
                    else if(strlen(token) == 1 && token[0] == '>'){
                        token = strtok(NULL, " \n");
                        if(token == NULL || (strlen(token) == 1 && token[0] == ';')){
                            break;
                        }
                        fd_out = creat(token, (1 << 9) - 1);
                    }
                    else{
                        params[count] = (char*) malloc(INPUT_SIZ * sizeof(char));
                        params[count++] = token;
                    }
                }
                while((token = strtok(NULL, " \n")) != NULL);
                params[count] = NULL;
                if(fork() > 0){
                    if(!bckgnd_exec){
                        int status;
                        wait(&status);
                    }
                }
                else{
                    if(fd_in != 0){
                        dup2(fd_in, 0);
                        close(fd_in);
                        if(pipe_fd[0] != -1){
                            close(pipe_fd[0]);
                        }
                    }
                    if(fd_out != 1){
                        dup2(fd_out, 1);
                        close(fd_out);
                        if(pipe_fd[1] != -1){
                            close(pipe_fd[1]);
                        }
                    }
                    int ret = execvp(params[0], params);
                    if(ret == -1){
                        perror("Error ");
                        kill(getpid(), SIGTERM);
                    }
                }
            }
            if(is_active(pipe_fd)){
                fd_in = pipe_fd[0];
                close(pipe_fd[1]);
            }
            fflush(stdout);
        }
        while((command = strtok(NULL, " \n")) != NULL);
        
        // to avoid going into infinite loop when executing source. Method is slow tho?
        memset(input, '\0', INPUT_SIZ);
    }
    free(current_dir);
    free(token);
    free(input);
    free(command);

    return 0;
}

void ctrl_c_handler(int sig){
    signal(SIGINT, ctrl_c_handler);
    signal(SIGUSR1, SIG_IGN);
    kill(-getpid(), SIGUSR1);
    fflush(stdout);
    fflush(stdin);
    write(0, "\nuser@shell: ", 14);
}

bool is_active(int* fd){
    if(fd[0] != -1 || fd[1] != -1){
        return true;
    }
    return false;
}

/* headers and their used functions :
    --> stdio : perror, printf, fgets
    --> stdlib : malloc, free
    --> unistd : getcwd, getlogin(), execl, fork, chdir, getlogin
    --> string : strcmp, strcat, strcpy, strtok
    --> wait : wait
    --> limits : PATH_MAX
    --> errno : errno
    --> stdbool : boolean data type
    --> signal : signal
*/
