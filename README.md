# Netcp

This is a simple linux c++ program that sends text files through sockets, its made using multithreading so you can have multiple
files being processed in parallel.

The program consists of a menu with a couple of commands: 
- quit - quits the program.
- receive [ PathToSaveFile ] - starts a process to receive files that are being sent and saves them at PathToSaveFile (just directory path, no filename).
- send [ filename ] - starts a new send process with the given filename.
- pause [ processNumber ] - pauses the send process with the given processNumber.
- abort [processNumber || \"receive\"] - aborts the process with given processNumber or the receive process if its enabled.
- resume [ processNumber ] - resumes the send process with the given ProcessNumber.


## Install 
Simply download the repo, cd to the folder and type `make` on the terminal to build the executable, 
to run type `./Netcp`.

