#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <vector>
#include <string>
#include <cstring>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"
#define MAX_PATH 256

using namespace std;
// function to transform a string to lowercase
string transform(string input) {
    string output;
    for (unsigned int i = 0; i < input.length(); i++) {
        char letter = tolower(input.at(i));
        output += letter;
    }
    return output;
}
// function to remove whitespaces from a string
string trim_space(string input) {
    string output;
    for (unsigned int i = 0; i < input.length(); i++) {
        char letter = input.at(i);
        if (letter == ' ') {
            continue;
        } else {
            output += input.at(i);
        }
    }
    return output;
}

int main () {
    // create copies of stdin and stidout using dup; DONE
    int stdout = dup(0);
    int stdin = dup(1);
    vector<pid_t> bg_processes;
    // previous working directory
    char pwd[MAX_PATH];
    for (;;) {
        // implement iteration over vector of bg pid (vector also declared outside loop); DONE
        // waitpid() - using flag for non-blocking; DONE
        for (size_t i = 0; i < bg_processes.size(); i++) {
            if ((waitpid(bg_processes[i], 0, WNOHANG)) > 0 ) {
                cout << "Process: " << bg_processes[i] << " ended" << endl;
                bg_processes.erase(bg_processes.begin() + i);
                i--;
            }
        }
        // Implement date/time with TODO; DONE
        time_t curTime;
        time(&curTime);
        string localtime = trim_space(ctime(&curTime));
        string date = localtime.substr(3,3) + ' ' + localtime.at(6);
        string time = localtime.substr(7,8);
        // Implement username with getenv("USER"); DONE
        char* username = getenv("USER");
        // Implement curdir with getcwd(); DONE
        char curr_dir[MAX_PATH];
        getcwd(curr_dir,MAX_PATH);

        cout << BLUE << date << " " << time << " " << username << ':' << curr_dir << NC << "$ ";
        

        // get user inputted command
        string input;
        getline(cin, input);

        if (transform(input) == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }       

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }
        // print out every command token-by-token on individual lines
        // prints to cerr to avoid influencing autograder
        for (auto cmd : tknr.commands) {
            for (auto str : cmd->args) {
                cerr << "|" << str << "| ";
            }
            if (cmd->hasInput()) {
                cerr << "in< " << cmd->in_file << " ";
            }
            if (cmd->hasOutput()) {
                cerr << "out> " << cmd->out_file << " ";
            }
            cerr << endl;
        }

        // implement cd with chdir(); DONE
        // if dir (cd <dir>) is "-", then go to previous working directory; DONE
        // variable to store previous working directory (needs to be declare outside loop); DONE

        // check if cd is the first argument
        if (tknr.commands.at(0)->args[0] == "cd") {
            // check for second argument is "-" to change to previous working directory
            if (tknr.commands.at(0)->args[1] == "-") {
               chdir(pwd);
            }
            // update store current working directory into previous working directory
            getcwd(pwd,MAX_PATH);
            // change directory to user-specified directory
            chdir(tknr.commands.at(0)->args[1].c_str());
            // go to next line
            continue;
        }

        // for piping
        // for cmd : commands
        //      call pipe() to make pipe
        //      fork() - in child, redirect stdout; in parent, redirect stdin
        //      ^ is already written (pog)
        // add checks for first/last command
        
        // file descriptor array
        int fd[2];
        // reinitializing stdin and stdout, resetting
        stdin = dup(0);
        stdout = dup(1);

        for(unsigned int i = 0; i < tknr.commands.size(); i++) {
            // create the pipe
            pipe(fd);
            // fork to create child
            pid_t pid = fork();
            if (pid < 0) {  // error check
                perror("FAILED TO CREATE CHILD");
                exit(2);
            }
            char** args = new char*[tknr.commands[i]->args.size() + 1];
            for (unsigned int j = 0; j < tknr.commands[i]->args.size(); j++) {
                args[j] = (char*)tknr.commands[i]->args[j].c_str();
            }
            args[tknr.commands[i]->args.size()] = nullptr;
            if (pid == 0) {  // if child, exec to run command
                // if current command is redirected, then open file and dup2 std(in/out) that's being redirected; DONE
                // implement it safely for both at the same time; DONE
                // check for I/O redirection
                // for final commands/one line commands
                if(i < tknr.commands.size() - 1) {
                    // copy stdout to write end of pipe
                    // In Child, redirect output to write end of the pipe
                    dup2(fd[1], 1);
                    // Close the read end of the pipe on the child side.
                    close(fd[0]);
                }
                // check for input redirection
                if (tknr.commands[i]->hasInput()) {
                    // open input file
                    int fd = open(tknr.commands[i]->in_file.c_str(), O_RDONLY);
                    // if file failed to open
                    if (fd < 0) {
                        perror("open");
                        exit(1);
                    }
                    // redirect fd to input
                    dup2(fd,STDIN_FILENO);
                }  
                // check for output redirection
                if (tknr.commands[i]->hasOutput()) {
                    // open output file
                    int fd = open(tknr.commands[i]->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                    // if file failed to open
                    if (fd < 0) {
                        perror("FAILED TO OPEN FILE");
                        exit(1);
                    }
                    // redirect fd to output
                    dup2(fd, STDOUT_FILENO);
                }        
                // In child, execute the command
                if (execvp(args[0], args) < 0) {  // error check
                    perror("execvp");
                    exit(2);
                }
            }
            else {  // if parent, wait for child to finish
                // Redirect the SHELL(PARENT)'s input to the read end of the pipe
                dup2(fd[0], STDIN_FILENO);
                // Close the write end of the pipe
                close(fd[1]);
                int status = 0;
                // add check for bg process - add pid to vector if bg and don't waitpid() in parent; DONE
                if(tknr.commands.at(i)->isBackground()) {
                    bg_processes.push_back(pid);
                }
                // wait until the last command finishes
                if((i == (tknr.commands.size() - 1)) && !(tknr.commands[i]->isBackground())) {
                    // deallocate the argument array afterwards
                    delete[] args;
                    waitpid(pid, &status, 0);
                }
                if (status > 1) {  // exit if child didn't exec properly
                    exit(status);
                }
            }
        }
        // restore stdin/stdout (variable would be outside loop);DONE;
        dup2(stdin, 0);
        dup2(stdout,1);
    }
}
