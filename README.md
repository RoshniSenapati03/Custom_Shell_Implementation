# Custom_Shell_Implementation


1. **Objective**  
Build a simple shell in C++ that can execute commands, manage processes, and handle redirection and piping. This project helps understand process management, system calls, and how real-world shells operate.

2. **Features**  
   2.1 Execute **basic commands** like `ls`, `pwd`, `cd`, `dir`.  
   2.2 **Input/Output Redirection**:  
       - `>` : Redirect output to a file (overwrite)  
       - `>>` : Redirect output to a file (append)  
       - `<` : Redirect input from a file  
   2.3 **Piping**: Run multiple commands connected by `|` (e.g., `ls | grep ".cpp"`)  
   2.4 **Background Jobs**: Run commands in the background using `&` (e.g., `sleep 10 &`)  
   2.5 **Job Control**:  
       - `jobs` : List active background jobs  
       - `fg <jobID>` : Bring a job to the foreground  
       - `bg <jobID>` : Resume a stopped job in the background  

3. **Instructions to Run**  
   3.1 Clone the repository:  
   ```bash
   git clone <your-repo-link>
   ```
   3.2 Navigate to the project folder:
   ```bash
   cd Custom-Shell-Implementation
   ```
   3.3 Compile the code:
   ```bash
   g++ custom_shell.cpp -o custom_shell
   ```
   3.4 Run the shell:
   ```bash
   ./custom_shell
   ```
   3.5 Use commands as you would in a normal shell:
   - Basic commands: ls, pwd, cd ..
   - Background process: sleep 10 &
   - Redirection: ls > output.txt, cat < output.txt
   - Piping: ls | grep ".cpp"
   - Job control: jobs, fg 1, bg 1

4. **Demonstration Commands**
   - Basic commands:
     ```text
     pwd
     ls
     cd ..
     ```
   - Redirection:
     ```text
     ls > output.txt
     cat < output.txt
     ls >> output.txt
     ```
   - Piping:
     ```text
     ls | grep ".cpp"
     ```
   - Background Jobs & Job Control:
     ```text
     sleep 10 &
     jobs
     fg 1
     bg 1
     ```
---
**Conclusion**

This Custom Shell in C++ can run basic commands, handle redirection, support piping, and manage background jobs. The project helps understand process handling, I/O control, and job management. It can be extended in the future with features like command history, auto-completion, and advanced job handling.
