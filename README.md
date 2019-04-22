# Practice Application Coding Exercise

There are 2 applications in this folder. One (messageParser.cpp) is the entry point deliverable for the coding exercise, and the other (messageGenerator.cpp) is for help with generating binary files to test with.
All other source code files are used by both applications.

## Building the applications

The "make" command will build both applications and store them in the build folder as messageParser.exe and messageGenerator.exe.

## Running the applications

messageParser.exe takes a single command line arguement, which will must be a path to a binary file to load and parse as if it were data being received over a communication interface.

messageGenerator.exe takes a variable amout of arguments based on the the value of the third argument. See the source code for more details.