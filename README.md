# simple-unix-shell

## Overview
This project is a simple UNIX shell that mimics basic functionalities of a standard command-line interface. The shell accepts commands from the user and executes them in separate processes. It provides features such as running commands in the background, maintaining a history of commands, and handling built-in commands like cd, pwd, exit, and help.

## Features
- Command Execution: Executes user commands in a separate child process using fork() and exec() family of functions.
- Background Execution: Allows commands to be run in the background by appending an ampersand (&) at the end of the command.
- Command History: Stores the last 10 commands issued and allows the user to re-execute commands from history using ! followed by the command index or !! for the most recent command.
- Built-in Commands:
	exit: Exits the shell.
	pwd: Prints the current working directory.
	cd: Changes the current working directory.
	help: Displays help information for built-in commands.
	history: Displays the list of last 10 commands executed.

## Usage
1) Compile the shell program using Makefile: make
2) Run the shell: ./shell
3) Type commands at the prompt and press Enter

# Contributing
If you want to contribute to this project, please fork this repository and create a pull request, or drop me an email at kha112@sfu.ca
