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
    const std::string outputFileName {"outputredir.txt"};
    std::string userIn {};
    std::cout << "Enter cmd: ";
    std::getline(std::cin, userIn);
    //Spaces are replaced with '\0'
    //The string is then sent to the child proc and used to construct the argv array
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
        //Buffer is one smaller here so we can terminate
        ssize_t bytesRead = read(pipeFd[0], buf, sizeof(buf)-1);
        close(pipeFd[0]);
        if(bytesRead < 0){
            std::cout << "Child had problem reading\n";
            return 3;
        }
        buf[bytesRead] = '\0';

        //Construct argv array
        const int argvSize = 10;
        char *newArgv[argvSize];
        int argvIdx = 1;
        newArgv[0] = buf; //First arg is the command
        for(int i = 1; i < bytesRead; ++i){
            if(buf[i] == '\0'){ //Tokens are seperated with \0
                if(argvIdx >= argvSize-1){ //Added elem would override space for null term
                    std::cout << "argv buffer too small\n";
                    return 5;
                }
                newArgv[argvIdx++] = &buf[i+1]; //This is fine because we tell read that the buffer is one smaller
            }
        }
        newArgv[argvIdx] = nullptr;
        int stdinFd = dup(STDOUT_FILENO);
        //Make file if not found, with owner read/write, group and others have read
        int fileOutFd = open(outputFileName.c_str(), O_APPEND | O_WRONLY | O_CREAT, 644);
        if(fileOutFd < 0){
            std::cout << "Error opening file for output redir\n";
            return 4;
        }
        //Set stdout to the file
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