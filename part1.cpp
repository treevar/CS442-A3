#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>

#define PASSKEY_LENGTH 20

std::string promptForKey(unsigned int length){
    std::string userIn;
    do{
        std::cout << "Enter " << length << " char long passkey: ";
        std::cin >> userIn;
    } while(userIn.length() != length);
    return userIn;
}

//Returns whether the key is in the file
//Returns -1 on file open error
int keyInFile(const std::string &key, const std::string &fileName){
    std::ifstream inFile {fileName};
    if(!inFile.is_open()){
        std::cout << "Couldnt open file\n";
        return -1;
    }
    std::string line {};
    while(inFile.good()){
        if(std::getline(inFile, line)){
            if(key == line){ return 1; }
        }
    }
    return 0;
}

int main(){
    const std::string passkeyFileName {"../passkeys.txt"};
    std::string userIn {promptForKey(PASSKEY_LENGTH)};
    int childFd[2], parentFd[2];
    if(pipe(parentFd) != 0){
        std::cout << "Pipe error\n";
        return 1;
    }
    if(pipe(childFd) != 0){
        std::cout << "Pipe error\n";
        return 2;
    }

    int childPid = fork();
    if(childPid < 0){
        std::cout << "Fork error\n";
        return 3;
    }
    else if(childPid == 0){ //Inside child
        char buf[PASSKEY_LENGTH+1]; //+1 for null term
        close(parentFd[1]);
        close(childFd[0]);
        //Read from parent
        int bytesRead = read(parentFd[0], buf, sizeof(buf)-1); //Dont allow to overwrite space for null term
        close(parentFd[0]);

        if(bytesRead < 0){
            std::cout << "Error reading\n";
            exit(4);
        }
        buf[bytesRead] = '\0';
        //Send to parent
        if(keyInFile(buf, passkeyFileName) == 1){
            write(childFd[1], "FOUND", 6);
        }
        else{
            write(childFd[1], "NOT FOUND", 10);
        }
        close(childFd[1]);
    }
    else{ //Inside parent
        close(parentFd[0]);
        close(childFd[1]);
        //Write passkey to child
        write(parentFd[1], userIn.c_str(), userIn.length());
        close(parentFd[1]);
        
        //Read response
        char buf[10];
        int bytesRead = read(childFd[0], buf, sizeof(buf));
        close(childFd[0]);
        if(bytesRead < 0){
            std::cout << "Error reading from child\n";
            return 5;
        }
        std::cout << buf << '\n';
        wait(nullptr); //Wait for child to finish
    }
    return 0;
}