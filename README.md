# Artificial Haiku Project

## Purpose
The Artificial Haiku project is a Linux-based C++ system that generates and stores "Artificial Haiku" poems, consisting of three lines with 5, 7, and 5 words. It uses three programs:

* haiku_master: Sends words to clients via TCP sockets.
* haiku_student: Collects words, forms haikus, and prints them to the terminal.
* librarian: Logs haikus to a file with timestamps.

## How to Run
### Prerequisites

- Linux system (e.g., Ubuntu, Debian).
- GCC with C++11 support (g++).
- Project files in ~/Projects/haiku_project: haiku_master.cpp, haiku_student.cpp, librarian.cpp.

### Steps

1. Navigate to the project directory:

```
cd ~/Projects/haiku_project
```
2. Compile the programs:

```
g++ -o haiku_master haiku_master.cpp
g++ -o haiku_student haiku_student.cpp
g++ -o librarian librarian.cpp
```

This compiles librarian, haiku_master, and haiku_student.

3. Run the programs in separate terminals:
- **Terminal 1**: Start the librarian:

```
./librarian
```

- **Terminal 2**: Start the haiku_master:

```
./haiku_master
```

- **Terminal 3**: Start the haiku_student:

```
./haiku_student
```

Output:
Haiku lines appear in the haiku_student terminal (if run manually).

Example:
[2025-05-22 11:41:23] student_12345: <br/>
the quick brown fox jumps <br/> 
over lazy dog sun rises slowly in <br/>
morning sky the quick brown fox


### Stop the system:

Press **Ctrl+C** in the script’s terminal to stop all processes.
Or manually:killall haiku_master haiku_student librarian
rm -f /tmp/haiku_librarian

## Notes

> Ensure master_config.txt and words.txt are in the project directory (created by run_haiku.sh if missing).
To run manually, start in order: ./librarian, ./haiku_master, ./haiku_student 127.0.0.1 5000 5001.

