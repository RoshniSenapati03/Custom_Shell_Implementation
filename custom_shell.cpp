#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <map>
using namespace std;

struct Job {
    pid_t pid;
    string command;
    bool running;
};

map<int, Job> jobs;
int jobCounter = 0;

// Parse input into tokens
vector<string> parseInput(const string &input) {
    vector<string> args;
    string arg;
    bool inQuotes = false;
    string temp;
    for (char c : input) {
        if (c == '"') { inQuotes = !inQuotes; continue; }
        if (c == ' ' && !inQuotes) {
            if (!temp.empty()) { args.push_back(temp); temp.clear(); }
        } else {
            temp += c;
        }
    }
    if (!temp.empty()) args.push_back(temp);
    return args;
}

// Convert vector<string> to vector<char*>
vector<char*> vecToCharPtr(vector<string> &tokens) {
    vector<char*> args;
    for (auto &s : tokens) args.push_back(&s[0]);
    args.push_back(nullptr);
    return args;
}

// Execute a single command
void executeCommand(vector<string> tokens) {
    if (tokens.empty()) return;
    bool appendMode = false;

    // Built-in: cd
    if (tokens[0] == "cd") {
        if (tokens.size() < 2) cerr << "cd: missing operand\n";
        else if (chdir(tokens[1].c_str()) != 0) perror("cd failed");
        return;
    }

    // Built-in: pwd
    if (tokens[0] == "pwd") {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr)
            cout << cwd << endl;
        else perror("pwd failed");
        return;
    }

    string inputFile, outputFile;
    vector<string> cmdArgs;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "<") inputFile = tokens[++i];
        else if (tokens[i] == ">") { outputFile = tokens[++i]; appendMode = false; }
        else if (tokens[i] == ">>") { outputFile = tokens[++i]; appendMode = true; }
        else cmdArgs.push_back(tokens[i]);
    }

    vector<char*> args = vecToCharPtr(cmdArgs);
    pid_t pid = fork();
    if (pid == 0) {
        if (!inputFile.empty()) {
            int fd = open(inputFile.c_str(), O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (!outputFile.empty()) {
            int fd = (appendMode) ? open(outputFile.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644)
                                  : open(outputFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(args[0], args.data());
        perror("execvp failed");
        exit(1);
    } else {
        waitpid(pid, nullptr, 0);
    }
}

// Execute piped commands
pid_t executePiped(vector<string> pipeCmds) {
    int n = pipeCmds.size();
    int pipefd[2*(n-1)];
    pid_t lastPID = -1;

    for (int i = 0; i < n-1; i++)
        if (pipe(pipefd + i*2) < 0) perror("pipe failed");

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            if (i != 0) dup2(pipefd[(i-1)*2], STDIN_FILENO);
            if (i != n-1) dup2(pipefd[i*2 + 1], STDOUT_FILENO);
            for (int j = 0; j < 2*(n-1); j++) close(pipefd[j]);

            vector<string> tokens = parseInput(pipeCmds[i]);
            string inputFile, outputFile;
            bool appendMode = false;
            vector<string> cmdArgs;

            for (size_t k = 0; k < tokens.size(); k++) {
                if (tokens[k] == "<") inputFile = tokens[++k];
                else if (tokens[k] == ">") { outputFile = tokens[++k]; appendMode = false; }
                else if (tokens[k] == ">>") { outputFile = tokens[++k]; appendMode = true; }
                else cmdArgs.push_back(tokens[k]);
            }

            if (!inputFile.empty()) {
                int fd = open(inputFile.c_str(), O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (!outputFile.empty()) {
                int fd = (appendMode) ? open(outputFile.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644)
                                      : open(outputFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            vector<char*> args = vecToCharPtr(cmdArgs);
            execvp(args[0], args.data());
            perror("execvp failed");
            exit(1);
        } else {
            lastPID = pid;
        }
    }

    for (int i = 0; i < 2*(n-1); i++) close(pipefd[i]);
    return lastPID; // Only store parent fork PID for bg jobs
}

// Bring job to foreground
void bringToForeground(int jobId) {
    if (jobs.find(jobId) == jobs.end()) {
        cout << "No such job\n";
        return;
    }
    Job job = jobs[jobId];
    cout << "Bringing job [" << jobId << "] to foreground: " << job.command << endl;
    waitpid(job.pid, nullptr, 0);
    jobs.erase(jobId);
}

// Resume job in background
void bringToBackground(int jobId) {
    if (jobs.find(jobId) == jobs.end()) {
        cout << "No such job\n";
        return;
    }
    Job job = jobs[jobId];
    cout << "Resuming job [" << jobId << "] in background: " << job.command << endl;
    kill(job.pid, SIGCONT);
}

// List jobs
void listJobs() {
    // Cleanup finished jobs first
    vector<int> toRemove;
    for (auto &j : jobs) {
        int status;
        pid_t result = waitpid(j.second.pid, &status, WNOHANG);
        if (result == j.second.pid) toRemove.push_back(j.first);
    }
    for (int id : toRemove) jobs.erase(id);

    cout << "\nActive Jobs:\n";
    for (auto &j : jobs)
        cout << "[" << j.first << "] PID: " << j.second.pid
             << "  Command: " << j.second.command
             << (j.second.running ? " (Running)" : " (Stopped)") << endl;
}

int main() {
    cout << "=====================================\n";
    cout << "         Custom Shell       \n";
    cout << "   Developed by K Roshni Senapati\n";
    cout << "=====================================\n";
    cout << "Supports: Piping, Redirection, Background & Job Control\n";
    cout << "Type 'exit' to quit the shell.\n\n";

    string input;
    while (true) {
        cout << "myShell> ";
        getline(cin, input);
        if (input.empty()) continue;
        if (input == "exit") break;

        bool background = false;
        if (input.back() == '&') { background = true; input.pop_back(); }

        // Job control built-ins
        if (input == "jobs") { listJobs(); continue; }
        if (input.rfind("fg", 0) == 0) { stringstream ss(input); string cmd; int id; ss >> cmd >> id; bringToForeground(id); continue; }
        if (input.rfind("bg", 0) == 0) { stringstream ss(input); string cmd; int id; ss >> cmd >> id; bringToBackground(id); continue; }

        vector<string> pipeCmds;
        stringstream ss(input);
        string cmdPart;
        while (getline(ss, cmdPart, '|')) pipeCmds.push_back(cmdPart);

        pid_t pid = fork();
        if (pid == 0) {
            if (pipeCmds.size() > 1){
                pid_t childPID = executePiped(pipeCmds);
                exit(0); // Don't wait inside child
            } else {
                vector<string> tokens = parseInput(pipeCmds[0]);
                executeCommand(tokens);
                exit(0);
            }
        } else {
            if (background) {
                jobCounter++;
                jobs[jobCounter] = {pid, input, true};
                cout << "Background job started (PID: " << pid << ")\n";
            } else {
                waitpid(pid, nullptr, 0);
            }
        }
    }

    cout << "Exiting shell... Goodbye!\n";
    return 0;
}
