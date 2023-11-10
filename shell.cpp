#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>
#include <string>

#include <fcntl.h>
#include <ctime>

#include <cstring>
#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

void replaceSubstring(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}

std::vector<std::pair<std::string, std::string>> getSignExpansions(const std::string input){
    // do we have a sign expansion in the input string?
    std::vector<std::pair<std::string, std::string>> cmds;
    for(long unsigned int i = 0; i < input.length()-1; i++){
        if(input[i] == '$' && input[i+1] == '('){
            // find the closing bracket
            std::vector<char> stack = {'('};
            long unsigned int j = i+2;
            while(stack.size() && j < input.length()){
                // search for a closing character
                if(input[j] == '('){
                    stack.push_back('(');
                }else if(input[j] == ')'){
                    stack.pop_back();
                }
                j++;
            }
            cmds.push_back(std::make_pair(input.substr(i, j-i), input.substr(i+2, j - (i+2+1))));
        }
    }
    return cmds;
}

bool hasSignExpansion (const std::string input){
    // do we have a sign expansion in the input string?
    for(long unsigned int i = 0; i < input.length()-1;i++){
        if(input[i] == '$' && input[i+1] == '('){
            // find the closing bracket
            std::vector<char> stack = {'('};
            long unsigned int j = i+2;
            while(stack.size() && j < input.length()){
                // search for a closing character
                if(input[j] == '('){
                    stack.push_back('(');
                }else if(input[j] == ')'){
                    stack.pop_back();
                }
                j++;
            }
            if(!stack.size()){
                return true;
            }
            return false;
        }
    }
    return false;
}

void logic(std::string& cwd, std::string& prevCwd, Tokenizer &tknr, int outRedir = -1){
        // Step 1: Get the current command char * []
        int savedReadFd = 0;
        for(long unsigned int i = 0; i < tknr.commands.size(); i++){

            std::vector<std::string> cmds = tknr.commands[i]->args;
            std::vector<char *> c_args;
            for(long unsigned int j = 0; j < cmds.size(); j++){
                const char * temp = cmds[j].c_str();
                c_args.push_back((char *)temp);
                //std::cout<<cmds[j]<<std::endl;
            }
            c_args.push_back(NULL);

            // check if we need to change cwd
            if(c_args[0] == std::string("cd")){
                if(c_args.size() > 1 && c_args[1] != NULL){
                    string temp = prevCwd;
                    prevCwd = cwd;
                    if(c_args[1] == std::string("-")){
                        if(chdir(temp.c_str())){
                            perror("chdir");
                            exit(2);
                        }
                        
                    }else if(chdir(c_args[1]) == -1){
                        perror("chdir");
                        exit(2);
                    }
                }else if(chdir("/") == -1){ // move back to root
                    perror("chdir");
                    exit(2);
                }
                continue;
            }


            // fd for last dir
            int fd[2];
            
            if(pipe(fd)){
                perror("fork");
                exit(2);
            }
            // fork to create child
            pid_t pid = fork();
            if (pid < 0) {  // error check
                perror("fork");
                exit(2);
            }

            if (pid == 0) {  // if child, exec to run command
                // run single commands with no arguments
                //char* args[] = {(char*) tknr.commands.at(0)->args.at(0).c_str(), nullptr};
                
                close(fd[0]); // we dont want to read anything right now
                if(i < tknr.commands.size() - 1){
                    dup2(fd[1], 1); // set stdout to be fd[1] else we want to keep it in the stdout
                }else if(outRedir != -1){
                    dup2(outRedir, 1); // we explicitly want to send the stdout to another file disc
                }
                dup2(savedReadFd, 0); // set stdin to be savedReadFd
                
                if(tknr.commands[i]->hasInput() && tknr.commands[i]->hasOutput()){
                    /*
                    
                    1. open a fdin for the input.
                    2. make the fdin for input the stdin for process
                    3. open the fdout
                    4. make the fdout the stdout for process
                    */

                    int fdin;
                    fdin = open(tknr.commands[i]->in_file.c_str(), O_RDONLY);
                    if(fdin == -1){
                        perror("open");
                        exit(2);
                    }
                    dup2(fdin, 0); // set the stdin in to be from input file
                    close(fdin);

                    int fdout;
                    fdout = open(tknr.commands[i]->out_file.c_str(), O_CREAT | O_WRONLY, 0644);
                    if(fdout == -1){
                        perror("open");
                        exit(2);
                    }
                    dup2(fdout, 1); // set the stdout out to be sent to fdout.
                    close(fdout);
                    
                }else if(tknr.commands[i]->hasInput()){
                    int fdin;
                    fdin = open(tknr.commands[i]->in_file.c_str(), O_RDONLY);
                    if(fdin == -1){
                        perror("open");
                        exit(2);
                    }
                    dup2(fdin, 0); // set the stdin in to be from input file
                    close(fdin);

                }else if(tknr.commands[i]->hasOutput()){
                    int fdout;
                    fdout = open(tknr.commands[i]->out_file.c_str(), O_CREAT | O_WRONLY, 0644);
                    if(fdout == -1){
                        perror("open");
                        exit(2);
                    }
                    dup2(fdout, 1); // set the stdout to be sent to fdout.
                    close(fdout);
                }
                if (execvp(c_args[0], c_args.data()) < 0) {  // error check
                    perror("execvp");
                    exit(2);
                }
            }
            else {  // if parent, wait for child to finish
                int status = 0;
                if(tknr.commands[i]->isBackground()){
                    signal(SIGCHLD, SIG_IGN); // kernel doesnt need to wait for parents decision for child anymore
                }else{
                    waitpid(pid, &status, 0); // NOTE: we only want to wait for the child process if it is not background process
                }
                
                if (status > 1) {  // exit if child didn't exec properly
                    //exit(status);
                }
                close(fd[1]);
                savedReadFd = fd[0];
            }
        }
}

int main () {
    std::string prevCwd = "..";
    while(1) {
        std::time_t now;
        std::time(&now);
        // Convert the time to a structure
        struct std::tm timeinfo;
        localtime_r(&now, &timeinfo);

        // Format the date and time as a string
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%B %d %H:%M:%S", &timeinfo);


        // need date/time, username, and absolute path to current dir
        char *username = getenv("USER");
        char buf[1024];
        std::string cwd = getcwd(buf, 1024);

        cout << WHITE << buffer <<" "<< RED <<username << WHITE << ":" << BLUE <<cwd << "$" << NC << " ";
        
        // get user inputted command
        string input;
        getline(cin, input);
        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }
        if(hasSignExpansion(input)){
            std::vector<std::pair<std::string, std::string>> cmds = getSignExpansions(input);
            for(long unsigned int i = 0; i < cmds.size(); i++){
                int fd[2];
                if(pipe(fd) == -1){
                    perror("fork");
                    exit(2);
                }
                int pid = fork();
                if (pid < 0){
                    perror("fork");
                    exit(2);
                }else if(pid >= 1){
                    // parent
                    wait(NULL); // wait for the child to execute command
                    close(fd[1]); // done writing
                    char buff[MAX_OUTPUT];
                    read(fd[0], buff, sizeof(buff));
                    close(fd[0]); // we are done reading
                    // remove any newlines
                    for (long unsigned int i = 0; i < std::strlen(buff); ++i) {
                        if (buff[i] == '\n') {
                            buff[i] = '\0';
                            break; // Exit the loop once the first newline is found and replaced
                        }
                    }
                    cmds[i].second = std::string(buff);
                    replaceSubstring(input, cmds[i].first, cmds[i].second);
                }else{
                    // child
                    Tokenizer signExpandedTknr(cmds[i].second);
                    close(fd[0]); // dont want to read anything rn 
                    logic(cwd, prevCwd, signExpandedTknr, fd[1]);
                    close(fd[1]); // done writing
                    exit(0); // gracefully done.
                }
            }
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            std::cout<<"Error in command."<<std::endl;
            continue;
        }
        logic(cwd, prevCwd, tknr);
    }
}
