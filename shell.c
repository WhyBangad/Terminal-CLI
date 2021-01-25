#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<wait.h>
#include<fcntl.h>
#include<limits.h>
#include<errno.h>

#define CHUNK_SIZ 10
#define INPUT_SIZ 1025
#define HOME "/home/"

#ifndef USER
#define USER getlogin()
#endif

// v1 -> no pipes and no additional params
// find macros for command and input size

char* get_newd(char* current_dir, char* token);

int main(int argc, char* argv[]){
    char* current_dir = (char*) malloc(PATH_MAX * sizeof(char));
    char* input = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* token = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* command = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* params = (char*) malloc(INPUT_SIZ * sizeof(char));

    while(1){
        current_dir = getcwd(current_dir, PATH_MAX);
        if(current_dir == NULL){
            perror("Error in fetching current directory : ");
            exit(1);
        }
        
        if(argc < 2 || strcmp(argv[1], "source") != 0){
            // avoid printing when executing source
            printf("user@shell: ");
        }

        fgets(input, INPUT_SIZ, stdin);
        command = strtok(input, " \n");
        
        if(strcmp(command, "exit") == 0){
            exit(0);
        }
        else if(strcmp(command, "cd") == 0){
            token = strtok(NULL, " \n");
            if(chdir(get_newd(current_dir, token)) == -1){
                perror("Error in cd ");
                exit(-1);
            }

            current_dir = getcwd(current_dir, PATH_MAX);
        }
        else if(strcmp(command, "pwd") == 0){
            printf("%s\n", current_dir);
        }
        else if(strcmp(command, "source") == 0){
            token = strtok(NULL, "\n");
            int fd_in = open(token, O_RDONLY);
            if(fd_in == -1){
                perror("Error in opening the file ");
                continue;
            }
            if(fork() > 0){
                int status;
                wait(&status);
            }
            else{
                dup2(fd_in, 0);
                execl("shell", "shell", "source", NULL);
            }
        }
        else if(strcmp(command, "echo") == 0){
            params = strtok(NULL, "\n");
            printf("%s\n", params);
        }
        else{
            command = strtok(input, "\n");
            printf("%s : command not found\n", command);
        }
        // to avoid going into infinite loop when executing source. Method is slow tho?
        memset(input, '\0', INPUT_SIZ);
    }
    free(current_dir);
    free(token);
    free(params);
    free(input);
    free(command);

    return 0;
}

char* get_newd(char* current_dir, char* token){
    if(token == NULL){
        // way to get home location -> getlogin()
        char* dir = (char*) malloc(PATH_MAX * sizeof(char));
        dir = strcpy(dir, HOME);
        return strcat(dir, USER);
    }
    if(strcmp(token, "") == 0 || strcmp(token, "..")){
        return token;
    }
    
    return strcat(strcat(current_dir, "/"), token);
}


/* headers and their used functions :
    --> stdio : perror, printf, fgets
    --> stdlib : malloc, free
    --> unistd : getcwd, getlogin(), execl, fork, chdir, getlogin
    --> string : strcmp, strcat, strcpy, strtok
    --> wait : wait
    --> limits : PATH_MAX
    --> errno : errno
*/