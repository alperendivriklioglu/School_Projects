#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>


/* 
This function gets the name of the user of the current session,
and returns it by storing it in the "user_name" parameter.
*/
void get_user_name(char *user_name)
{
	// 'user_name_file' will store the name of the file where the output of 'whoami' will be directed to.	
	char *user_name_file = "/tmp/user_name.txt";

	// To be able execute 'whoami' without switching to that process, this line forks the current process.
	pid_t pid = fork();
	
	// This part checks whether the fork has failed or not.
	// If yes, an error message gets printed and execution ends.
	// If not, 'whoami' gets executed in the child process, and the parent process waits for the child process to end.
	if (pid == -1) {
		perror("fork failed");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {	// The child process	
		// Creates the file where the output of 'whoami' will be directed to,
		// and the returned file descriptor is stored at 'filedesc' to be able to access the file when needed.
		int filedesc = open(user_name_file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
		
		// Redirects the STDOUT to 'filedesc', to write the output of 'whoami' to that file.
		dup2(filedesc, 1);
		
		// 'whoami' gets executed.
		char *cmd = "whoami";			
		char *argv[] = {"whoami", NULL};
		execvp(cmd, argv);
		
		// Child process gets terminated.
		_exit(EXIT_SUCCESS);
	}
	else {	// The parent process
		// Waits for the child process to end.
		int status;
		(void)waitpid(pid, &status, 0);

		// Opens the file where the name of the user of the current session is stored.
		int filedesc = open(user_name_file, O_RDONLY);
		
		// Reads the user name from the file and returns the length of the user name, then the length is stored at "user_name_length".
		// (This line reads 33 bytes; because, the length of the name of the user is not known in advance,
		// but a user name can be 32 characters long at most.) 
		int user_name_length = read(filedesc, user_name, 33);

		// The character after the last character of the user name gets modified with null character,
		// in case the user name is shorter than 32 characters long.
		user_name[user_name_length - 1] = '\0';
	}
}


/* 
This function reads the user input, and returns it.
*/
char* read_line()
{
	char *line = NULL;
	size_t len = 0;

	// The input line is read into "line".	
	getline(&line, &len, stdin);

	// "getline" is not able to handle the newline characters in the end of the input.
	// Therefore, the newline character gets replaced with a null character.
	line[strlen(line)-1] = '\0';
	
	// "line" is returned.
	return line;
}


/* 
This function adds the latest input line to the shell history.
The line is passed to this function via 'line' parameter.
*/
void add_line_to_history(char *line)
{
	// Shell history is stored in the file with the path '/tmp/shell_history'.
	char *shell_history_file = "/tmp/shell_history";

	// 'filedesc' is used to access the shell history file.
	// If the file does not exist, it gets created first.
	int filedesc = open(shell_history_file, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);

	// 'line' gets written to the shell history file.
	write(filedesc, line, strlen(line));
	write(filedesc, "\n", 1);
}


/* 
This function parses the latest input line and separates the line into the executable and its arguments,
and runs the command (if possible).
*/
void parse_and_execute(char *line)
{
	// Checks whether the "exit" command is inputted. If yes, the execution of the shell ends.
	if(strcmp(line, "exit") == 0) {
		exit(EXIT_SUCCESS);
	}
	
	// To be able execute the inputted command without switching to that process, this line forks the current process.
	pid_t pid = fork();
	
	// This part checks whether the fork has failed or not.
	// If yes, an error message gets printed and execution ends.
	// If not, the inputted command gets executed in the child process, and the parent process waits for the child process to end.
	if (pid == -1) {
		perror("fork failed");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {	// The child process, namely the execution of the inputted command (if possible).
		// "cmd" keeps the name of the Linux executable corresponding to the inputted command.
		char *cmd;

		if(strcmp(line, "listdir") == 0) {	// Checks whether the inputted command is "listdir".
			// "ls" gets executed.
			cmd = "ls";
			char *argv[] = {cmd, NULL};
			execvp(cmd, argv);
		} else if(strcmp(line, "listdir -a") == 0) {	// Checks whether the inputted command is "listdir -a".
			// "ls -a" gets executed.
			char *cmd = "ls";
			char *argv[] = {cmd, "-a", NULL};
			execvp(cmd, argv);
		} else if(strcmp(line, "currentpath") == 0) {	// Checks whether the inputted command is "currentpath".
			// "pwd" gets executed.
			char *cmd = "pwd";
			char *argv[] = {cmd, NULL};
			execvp(cmd, argv);
		} else if(strcmp(line, "footprint") == 0) {	// Checks whether the inputted command is "footprint".
			// 'footprint' is actually the equivalent of 'tail -15 /tmp/shell_history | nl'.
			// So, to pipe the output of 'tail -15 /tmp/shell_history' to the input of 'nl', a pipe gets created.
			int pipefd[2];
			pipe(pipefd);
			
			// To be able execute 'tail -15 /tmp/shell_history' command without switching to that process, 
			// this line forks the current process.
			pid_t history_pid = fork();
			
			// This part checks whether the fork has failed or not.
			// If yes, an error message gets printed and the execution of the command ends.
			// If not, 'tail -15 /tmp/shell_history' gets executed in the child process, 
			// and the parent process waits for the child process to end.
			if(history_pid == -1) {
				perror("fork failed");
				exit(EXIT_FAILURE);
			} else if(history_pid == 0) {	// Child process, namely, 'tail -15 /tmp/shell_history'.
				// STDOUT is directed to the write end of the pipe.
				dup2(pipefd[1], 1);
				close(pipefd[0]);
				close(pipefd[1]);

				// 'tail -15 /tmp/shell_history' gets executed.
				char *cmd = "tail";
				char *argv[]={cmd, "-15", "/tmp/shell_history", NULL};
				execvp(cmd, argv);
				_exit(EXIT_SUCCESS);
			} else {	// Parent process, namely, 'nl'
				// Parent process waits for 'tail -15 /tmp/shell_history' to finish.
				int status;
				(void)waitpid(pid, &status, 0);

				// STDIN is directed to the read end of the pipe.
				dup2(pipefd[0],0);
				close(pipefd[1]);
				close(pipefd[0]);

				// 'nl' -with the output of 'tail -15 /tmp/shell_history' as its input- gets executed.
				char *cmd = "nl";
				char *argv[2];
				read(0, argv[0], 1024); // The output of 'tail -15 /tmp/shell_history' is read into argv[0] to be used as an argument to 'nl'.
										// '0' is the file descriptor number of STDIN. As STDIN was redirected to the read end of the pipe,
										// the input is read from the read end of the pipe. 
				argv[1] = NULL;
				execvp(cmd, argv);
			}
		} else if(strchr(line, 62)) {	// Checks whether the inputted command is 'printfile <fileName> > <newFileName>'.
										// 62 is the ASCII code of '>', and strchr checks for '>' inside the inputted command.
			
			// The file with the name <newFileName> gets created.
			// (strrchr(line, 32)+1 returns the <newFileName> argument).
			int new_file_fildes = open(strrchr(line, 32)+1, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
			
			// STDOUT is redirected to the file with the name <newFileName>.
			dup2(new_file_fildes, 1);
			
			// 'cat <fileName> > <newFileName>' gets executed.
			char *cmd = "cat";
			char *argv[]={cmd, strtok(strchr(line, 32)+1, " "), NULL}; // 'strtok(strchr(line, 32)+1, " ")' returns the <fileName> argument. 
			execvp(cmd, argv);
		} else if(strchr(line, 45) && strchr(line, 124)) {	// Checks whether the inputted command is 'listdir -a | grep "<argument>"'.
															// 124 is the ASCII code of '|' and 45 is the ASCII code of '-'
															// strchr checks for '|' and '-' inside the inputted command.
			
			// 'listdir -a | grep "<argument>"' is actually the equivalent of 'ls -a | grep "<argument>"'.
			// So, to pipe the output of 'ls -a' to the input of 'grep <argument>', a pipe gets created.
			int pipefd[2];
			pipe(pipefd);
			
			// To be able execute 'ls -a' command without switching to that process, 
			// this line forks the current process.
			pid_t listdir_pid = fork();
			
			// This part checks whether the fork has failed or not.
			// If yes, an error message gets printed and execution ends.
			// If not, 'ls -a' gets executed in the child process, 
			// and the parent process waits for the child process to end.
			if(listdir_pid == -1) {
				perror("fork failed");
				exit(EXIT_FAILURE);
			} else if(listdir_pid == 0) {	// Child process, namely, 'ls -a'.
				// STDOUT is directed to the write end of the pipe.
				dup2(pipefd[1], 1);
				close(pipefd[0]);
				close(pipefd[1]);

				// 'ls -a' gets executed.
				char *cmd = "ls";
				char *argv[]={cmd, "-a", NULL};
				execvp(cmd, argv);
				_exit(EXIT_SUCCESS);
			} else {	// Parent process, namely, 'grep <argument>'.
				// Parent process waits for 'ls -a' to finish.
				int status;
				(void)waitpid(pid, &status, 0);

				// STDOUT is directed to the read end of the pipe.
				dup2(pipefd[0],0);
				close(pipefd[1]);
				close(pipefd[0]);

				// 'grep <argument>' -with the output of 'ls -a' as its input- gets executed.
				char *cmd = "grep";
				char *argv[3];
				read(0, argv[0], 1024); // The output of 'ls -a' is read into argv[0] to be used as an argument to 'grep <argument>'.
										// '0' is the file descriptor number of STDIN. As STDIN was redirected to the read end of the pipe,
										// the input is read from the read end of the pipe.
				argv[1] = strtok(strchr(line, 34)+1, "\""); // 'strtok(strchr(line, 34)+1, "\"")' returns the <argument>.
															// (i.e. the argument between the quotes)
				argv[2] = NULL;
				execvp(cmd, argv);
			}
		} else if(strchr(line, 124)) {	// Checks whether the inputted command is 'listdir | grep "<argument>"'.
										// 124 is the ASCII code of "|", and strchr checks for "|" inside the inputted command.
			// 'listdir | grep "<argument>"' is actually the equivalent of 'ls | grep "<argument>"'.
			// So, to pipe the output of 'ls' to the input of 'grep <argument>', a pipe gets created.
			int pipefd[2];
			pipe(pipefd);
			
			// To be able execute 'ls' command without switching to that process, 
			// this line forks the current process.
			pid_t listdir_pid = fork();
			
			// This part checks whether the fork has failed or not.
			// If yes, an error message gets printed and execution ends.
			// If not, 'ls' gets executed in the child process, 
			// and the parent process waits for the child process to end.
			if(listdir_pid == -1) {
				perror("fork failed");
				exit(EXIT_FAILURE);
			} else if(listdir_pid == 0) {	// Child process, namely, 'ls'.
				// STDOUT is directed to the write end of the pipe.
				dup2(pipefd[1], 1);
				close(pipefd[0]);
				close(pipefd[1]);

				// 'ls' gets executed.
				char *cmd = "ls";
				char *argv[]={cmd, NULL};
				execvp(cmd, argv);
				_exit(EXIT_SUCCESS);
			} else {	// Parent process, namely, 'grep <argument>'.
				// Parent process waits for 'ls -a' to finish.
				int status;
				(void)waitpid(pid, &status, 0);

				// STDOUT is directed to the read end of the pipe.
				dup2(pipefd[0],0);
				close(pipefd[1]);
				close(pipefd[0]);

				// 'grep <argument>' -with the output of 'ls' as its input- gets executed.
				char *cmd = "grep";
				char *argv[3];
				read(0, argv[0], 1024); // The output of 'ls' is read into argv[0] to be used as an argument to 'grep <argument>'.
										// '0' is the file descriptor number of STDIN. As STDIN was redirected to the read end of the pipe,
										// the input is read from the read end of the pipe.
				argv[1] = strtok(strchr(line, 34)+1, "\""); // 'strtok(strchr(line, 34)+1, "\"")' returns the <argument>.
															// (i.e. argument between the quotes)
				argv[2] = NULL;
				execvp(cmd, argv);
			}
		} else {	// Checks whether the inputted command is 'printfile <fileName>'.
			// 'cat <fileName>' gets executed.
			char *cmd = "cat";
			char *argv[] = {cmd, strrchr(line, 32) + 1, NULL}; // 'strrchr(line, 32) + 1' returns the <fileName> argument. 
			execvp(cmd, argv);
		}
		_exit(EXIT_SUCCESS);
	}
	else {	// Parent process, namely, the shell
		// Parent process, namely, the shell, waits for the execution of the current command to end.
		int status;
		(void)waitpid(pid, &status, 0);
	}
}


int main(void)
{
	// Name of the user of the current session is received from the system,
	// and stored in the 'user_name' variable.
	char user_name[33];
	get_user_name(user_name);
	while(1)
	{
		// User is prompted to enter a command.
		printf("%s >>> ", user_name);

		// Inputted line gets read and stored in 'line'.
		char *line = read_line();

		// 'line' is passed to 'add_line_to_history()' function to be added to shell history.
		add_line_to_history(line);

		// 'line' is passed to 'parse_and_execute()' function to be parsed and executed (if possible).
		parse_and_execute(line);
	}
}
