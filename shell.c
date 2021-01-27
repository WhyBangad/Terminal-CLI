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

//no pipes
/*issues :
    1) ctrl + c interrupt is only caught once
    2) write all errors to the error stream not standard output
*/

char* get_newd(char* current_dir, char* token);
void ctrl_c_handler(int sig);

jmp_buf env_buffer;

int main(int argc, char* argv[]){
    char* current_dir = (char*) malloc(PATH_MAX * sizeof(char));
    char* input = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* token = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* command = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* args = (char*) malloc(INPUT_SIZ * sizeof(char));
    
    signal(SIGINT, ctrl_c_handler);
    bool bckgnd_exec = false;

    current_dir = getcwd(current_dir, PATH_MAX);
    if(current_dir == NULL){
        perror("Error in fetching current directory : ");
        exit(1);
    }

    if(setjmp(env_buffer) != 0){
        signal(SIGINT, ctrl_c_handler);
        printf("\n");
    }

    while(1){
        if(argc < 2 || strcmp(argv[1], "source") != 0){
            // avoid printing when executing source
            printf("user@shell: ");
        }

        fgets(input, INPUT_SIZ, stdin);
        command = strtok(input, " \n");
        if(command == NULL && strlen(input) != 0){
            // user pressed enter
            continue;
        }
        do{
            //printf("COMMAND : %s\n", command);
            bckgnd_exec = false;

            if(strcmp(command, "exit") == 0){
                exit(0);
            }
            else if(strcmp(command, ";") == 0){
                continue;
            }
            else if(strcmp(command, "cd") == 0){
                token = strtok(NULL, " \n");

                if(strcmp(token, ";") == 0){
                    token = NULL;
                }
                if(token == NULL){
                    token = (char*) malloc(PATH_MAX * sizeof(char));
                    token = strcat(strcat(token, HOME), USER);
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
            else if(strcmp(command, "cat") == 0){
                int fd_in = 0, fd_out = 1;
                token = strtok(NULL, " ;\n");
                if(token == NULL){
                    continue;
                }
                if(strlen(token) == 1 && token[0] == '<'){
                    token = strtok(NULL, " \n");
                    if(token == NULL || (strlen(token) == 1 && token[0] == ';')){
                        printf("shell: syntax error near <\n");
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
                        printf("shell: syntax error near >\n");
                        continue;
                    }
                    fd_out = creat(token, (1 << 9) - 1);
                    token = strtok(NULL, " \n");
                }
                char* buffer = (char*) malloc(INPUT_SIZ * sizeof(char));
                while(1){
                    int n_read = read(fd_in, buffer, INPUT_SIZ);
                    if(fd_out != 1){
                        write(fd_out, buffer, n_read);
                    }
                    else{
                        printf("%s", buffer);
                    }

                    if(n_read < INPUT_SIZ){
                        break;
                    }
                }
            }
            else if(strcmp(command, "source") == 0){
                int fd_in = 0, fd_out = 1;
                token = strtok(NULL, " \n");
                
                if(token == NULL){
                    printf("bash: source: filename argument required\n");
                    continue;
                }
                if(strlen(token) == 1 && token[0] == '<'){
                    token = strtok(NULL, " \n");
                    if(token == NULL || (strlen(token) == 1 && token[0] == ';')){
                        printf("source: syntax error near <\n");
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
                        printf("source: syntax error near >\n");
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
                    params[count] = (char*) malloc(INPUT_SIZ * sizeof(char));
                    if(strlen(token) == 1 && strcmp(token, ";") == 0){
                        break;
                    }
                    if(strlen(token) == 1 && strcmp(token, "&") == 0){
                        bckgnd_exec = true;
                        break;
                    }
                    params[count++] = token;
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
                    int ret = execvp(params[0], params);
                    if(ret == -1){
                        perror("Error ");
                        kill(getpid(), SIGTERM);
                    }
                }
            }
            fflush(stdout);
        }
        while((command = strtok(NULL, " \n")) != NULL);
        
        // to avoid going into infinite loop when executing source. Method is slow tho?
        memset(input, '\0', INPUT_SIZ);
    }
    free(current_dir);
    free(token);
    free(args);
    free(input);
    free(command);

    return 0;
}

char* get_newd(char* current_dir, char* token){
    if(token == NULL){
        char* dir = (char*) malloc(PATH_MAX * sizeof(char));
        dir = strcpy(dir, HOME);
        return strcat(dir, USER);
    }
    if(strcmp(token, "") == 0 || strcmp(token, "..") == 0 || strcmp(token, "-") == 0){
        return token;
    }
    
    return strcat(strcat(current_dir, "/"), token);
}

void ctrl_c_handler(int sig){
    signal(SIGINT, ctrl_c_handler);
    longjmp(env_buffer, 1);
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
