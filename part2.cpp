#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> //open

//replace a with b
void replace(std::string &str, const char a, const char b){
    for(auto &c : str){
        if(c == a){ c = b; }
    }
}

int main(){
    std::string userIn {};
    std::cout << "Enter cmd: ";
    std::getline(std::cin, userIn);
    replace(userIn, ' ', '\0');
    int pipeFd[2];
    if(pipe(pipeFd) < 0){
        std::cout << "Error creating pipe\n";
        return 1;
    }
    int childPid = fork();
    if(childPid < 0){
        std::cout << "Error forking\n";
        return 2;
    }
    else if(childPid == 0){ //Child
        close(pipeFd[1]);
        char buf[128];
        ssize_t bytesRead = read(pipeFd[0], buf, sizeof(buf)-1);
        close(pipeFd[0]);
        if(bytesRead < 0){
            std::cout << "Child had problem reading\n";
            return 3;
        }
        buf[bytesRead] = '\0';

        const int argvSize = 10;
        char *newArgv[argvSize];
        int argvIdx = 1;
        newArgv[0] = buf; //First arg is the command
        for(int i = 1; i < bytesRead; ++i){
            if(buf[i] == '\0'){
                newArgv[argvIdx++] = &buf[i+1]; //This is fine because we tell read that the buffer is one smaller
                if(argvIdx >= argvSize-1){ break; }
            }
        }
        newArgv[argvIdx] = nullptr;
        int stdinFd = dup(STDOUT_FILENO);
        int fileOutFd = open("outputredir.txt", O_APPEND | O_WRONLY);
        if(fileOutFd < 0){
            std::cout << "Error opening file for output redir\n";
            return 4;
        }
        dup2(fileOutFd, STDOUT_FILENO);
        close(fileOutFd);
        execvp(buf, newArgv);
    }
    else{ //Parent
        close(pipeFd[0]);
        write(pipeFd[1], userIn.c_str(), userIn.length());
        close(pipeFd[1]);
        wait(nullptr);
    }
    return 0;
}