#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<wait.h>

#define CHUNK_SIZ 10
#define INPUT_SIZ 100
#define TOKEN_SIZ 20
#define DIR_SIZ 100
#define HOME "/home/"

#ifndef USER
#define USER getlogin()
#endif

// v1 -> no pipes and no additional params
// struct for command?

char* get_params(char* input, char* params, int start_index);
char* get_newd(char* current_dir, char* token);

int main(int argc, char* argv[]){
    char* current_dir = (char*) malloc(DIR_SIZ * sizeof(char));
    char* input = (char*) malloc(INPUT_SIZ * sizeof(char));
    char* token = (char*) malloc(TOKEN_SIZ * sizeof(char));
    char* command = (char*) malloc(TOKEN_SIZ * sizeof(char));
    char* params = (char*) malloc(INPUT_SIZ * sizeof(char));

    while(1){
        current_dir = getcwd(current_dir, DIR_SIZ);
        if(current_dir == NULL){
            perror("Error in fetching current directory : ");
            exit(1);
        }
        printf("user@shell: ");
        fgets(input, INPUT_SIZ, stdin);

        command = strtok(input, " \n");
        
        if(strcmp(command, "exit") == 0){
            exit(0);
        }
        else if(strcmp(command, "cd") == 0){
            token = strtok(NULL, " \n");
            printf("NEWD : %s\n", get_newd(current_dir, token));
            if(chdir(get_newd(current_dir, token)) == -1){
                perror("Error in cd ");
                exit(-1);
            }

            current_dir = getcwd(current_dir, DIR_SIZ);
        }
        else if(strcmp(command, "pwd") == 0){
            printf("%s\n", current_dir);
        }
        else if(strcmp(command, "source") == 0){
            // work on this
        }
        else if(strcmp(command, "echo") == 0){
            params = get_params(input, params, strlen(command) + 1);
            if(fork() > 0){
                int status;
                wait(&status);
                if(status != 0){
                    // error handling
                }
            }
            else{
                execl("/bin/echo", "echo", params, (char*) NULL);
            }
        }
        else{
            command = strtok(input, "\n");
            printf("%s : command not found\n", command);
        }
    }
}

char* get_params(char* input, char* params, int start_index){
    int index = start_index, size = INPUT_SIZ, break_flag = 0;
    char c;
    while(break_flag == 0){
        c = input[index];
        if(c == EOF || c == '\n'){
            c = '\0';
            break_flag = 1;
        }

        /* need to expand */
        if(size <= index){
            size += CHUNK_SIZ;
            char* tmp = (char*) realloc(params, size + CHUNK_SIZ);
            /* unable to allocate memory */
            if(tmp == NULL){
                free(params);
                params = NULL;
                break;
            }
            params = tmp;
            tmp = NULL;
        }
        params[index - start_index] = c;
        ++index;
    }
    
    return params;
}

char* get_newd(char* current_dir, char* token){
    if(token == NULL){
        // way to get home location -> getlogin()
        char* dir = (char*) malloc(DIR_SIZ * sizeof(char));
        dir = strcpy(dir, HOME);
        return strcat(dir, USER);
    }
    if(strcmp(token, "") == 0 || strcmp(token, "..")){
        return token;
    }
    
    return strcat(strcat(current_dir, "/"), token);
}

/* headers and their used functions :
    --> stdio : perror, printf
    --> stdlib : malloc, 
    --> unistd : getcwd, getlogin()
*/