#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>
#include <string>
#include <cstring>

#include <stdlib.h>
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
    // create copies of stdin and stdout; dup(); DONE
    int stdout = dup(0);
    int stdin = dup(1);
    // bg processes
    vector<pid_t> bg_processes;
    // previous working directory
    string pwd;
    for (;;) {
        // implement iteration over vector of bg pid (vector also declared outside loop)
        // waitpid() - using flag for non-blocking
        for (unsigned int i = 0; i < bg_processes.size(); i++) {
            if (waitpid(bg_processes[i], 0, WNOHANG) > 0) {
                cout << "Process: " << bg_processes[i] << "ended" << endl;
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
        char cwd[MAX_PATH];
        // need date/time, username, and absolute path to current dir
        cout << YELLOW << date << " " << time << " " << username << ':' << getcwd(cwd, sizeof(cwd)) << "$ " << NC;
        
        // get user inputted command
        string input;
        getline(cin, input);


        if (transform(input) == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }
        if (input.empty()) {
            continue;
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        // implement cd with chdir(); DONE
        // if dir (cd <dir>) is "-", then go to previous working directory; DONE
        // variable to store previous working directory (needs to be declare outside loop); DONE

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
        // for loop to go through commands
        for (unsigned int i = 0; i < tknr.commands.size(); i++) {
            // argument list
            char** args = new char*[tknr.commands[i]->args.size() + 1];
            for (unsigned int j = 0; j < tknr.commands[i]->args.size(); j++) {
                args[j] = const_cast<char*>(tknr.commands[i]->args[j].c_str());
            }
            args[tknr.commands[i]->args.size()] = nullptr;
            // process non-piped command
            if (tknr.commands.size() < 2) {
                // implement cd with chdir()
                // if it's not cd, then we can process commands normally
                if (string(args[0]).find("cd") == 0) {
                    // check to see if we need to go to previous directory
                    if (string(args[1]).find("-") == 0) {
                        // if there is no previous working directory store current directory
                        if (pwd.empty()) {
                            pwd = getcwd(cwd, sizeof(cwd));
                        }
                        // if pwd isn't empty we just need to change directory
                        chdir(pwd.c_str());
                        // update previous working directory 
                        pwd = getcwd(cwd, sizeof(cwd));
                    } else {
                        // if we don't need to go to previous working directory
                        // update pwd with the current directory
                        pwd = cwd;
                        // change directory to specified directory
                        chdir(args[1]);
                    }
                }
                pid_t pid = fork();
                if (pid < 0) { // error check
                    perror("FAILED TO CREATE CHILD");
                    exit(1);
                }
                // if current command is redirected, then open file and dup2 std(in/out) that's being redirected; DONE
                // implement it safely for both at the same time; DONE
                if (pid == 0) {
                    // file descriptor
                    int fd;
                    // check for I/O redirection
                    // check for input redirection
                    if (tknr.commands[i]->hasInput()) {
                        // read only
                        fd = open(tknr.commands[i]->in_file.c_str(), O_RDONLY);
                        // if file failed to open
                        if (fd < 0) {
                            perror("FAILED TO OPEN FILE");
                            exit(1);
                        }
                        // redirect fd to input
                        dup2(fd, STDIN_FILENO);
                    }
                    // check for output redirection
                    if (tknr.commands[i]->hasOutput()) {
                        // write / create file
                        fd = open(tknr.commands[i]->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                        // if file failed to open
                        if (fd < 0) {
                            perror("FAILED TO OPEN FILE");
                            exit(1);
                        }
                        // redirect fd to output
                        dup2(fd, STDOUT_FILENO);
                    }
                    // execute command
                    if(execvp(args[0], args) < 0) {
                        perror("FAILED TO EXECUTE");
                        exit(1);
                    }
                } else if (pid > 0) { // in parent process
                    // add check for bg process - add pid to vector if bg and don't waitpid() in parent; DONE
                    if (tknr.commands[i]->isBackground()) {
                        bg_processes.push_back(pid);
                    } else {
                        // if not a bg process then we can wait for child
                        int status;
                        waitpid(pid, &status, 0);
                    }
                }
            } else {
                // for piping commands
                int fd[2];
                // create pipe
                pipe(fd);
                pid_t pid = fork();
                if (pid < 0) { // error check
                    perror("FAILED TO CREATE CHILD");
                    exit(1);
                }
                if (pid == 0) {
                    // last command
                    if (i < tknr.commands.size() - 1) {
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[0]);
                    }
                    execvp(args[0], args);
                } else {
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[1]);
                    if (i == tknr.commands.size() - 1) {
                        wait(0);
                    }
                }
            }
        }
        dup2(stdin, 0);
        dup2(stdin, 1);

        // for piping
        // for cmd : commands
        //      call pipe() to make pipe
        //      fork() - in child, redirect stdout; in parent, redirect stdin
        //      ^ is already written (pog)
        // add checks for first/last command
        
        /*
        // fork to create child
        pid_t pid = fork();
        if (pid < 0) {  // error check
            perror("fork");
            exit(2);
        }

        // add check for bg process - add pid to vector if bg and don't waitpid() in parent

        if (pid == 0) {  // if child, exec to run command
            // implement multiple arguments - iterate over args of current command to make
            // char* array
            char* args[] = {(char*) tknr.commands.at(0)->args.at(0).c_str(), nullptr};

            // if current command is redirected, then open file and dup2 std(in/out) that's being redirected
            // implement it safely for both at the same time

            if (execvp(args[0], args) < 0) {  // error check
                perror("execvp");
                exit(2);
            }
        }
        else {  // if parent, wait for child to finish
            int status = 0;
            waitpid(pid, &status, 0);
            if (status > 1) {  // exit if child didn't exec properly
                exit(status);
            }
        }
        // restore stdin/stdout (variable woudl be outside loop)
        */
    }
}
