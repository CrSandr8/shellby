Sure thing, bro! Here is the revised `README.md` in English with the license updated to Apache 2.0.

***

# 🐚 Shellby 

> A didactic file system inspired by the FAT architecture, entirely mapped in RAM and equipped with a dedicated interactive shell.

**Shellby** is a project developed for academic purposes to put into practice the theoretical concepts of operating systems and mass storage management. The system simulates the behavior of a FAT (File Allocation Table) formatted disk, autonomously managing block allocation, hierarchical navigation, and data persistence through the use of memory mapping (`mmap`).

## ✨ Key Features

* **Memory Mapping (mmap):** The file system resides in a binary file acting as a virtual "disk," mapped directly into the process's virtual address space to eliminate the overhead of `read()` and `write()` calls.
* **Lazy Allocation:** Newly created empty files do not consume data space; no physical block is allocated until an actual write operation occurs.
* **Bottom-Up Propagation (Directory Sizing):** Directories update their size dynamically by traversing up the hierarchical tree using the `..` parent directory entry.
* **Separation of Concerns:** The architecture is clearly divided into two layers: the low-level File System APIs (`/fat`) and the Shell state machine (`/shell`).

## 🏗 Project Architecture

The source code is organized to strictly separate disk logic from the user interface:

* `src/fat/`: Contains the definitions of data structures (Superblock, FAT, FCB) and the core APIs for virtual disk manipulation. No user interaction occurs here.
* `src/shell/`: Contains the interactive wrapper. It parses user input and invokes the appropriate system APIs.
* `src/main.c`: The application entry point.

## 🚀 Getting Started

### Prerequisites
The project is written in C and designed for Unix-like environments (Linux/macOS). You will need:
* A C compiler (e.g., `gcc` or `clang`)
* `make`

### Build and Run

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/shellby.git
   cd shellby
   ```

2. Compile the project using the Makefile:
   ```bash
   make
   ```

3. Launch the Shellby shell:
   ```bash
   ./shellby
   ```

## 🛠 Available Commands

Once inside the Shellby shell, you have a set of commands to interact with the virtual file system.

*Note: You must format or mount a virtual disk before performing any operations.*

* `format <size>`: Creates and initializes a new file system of the specified size. It generates the Superblock, clears the FAT, and creates the root directory.
* `mount`: Mounts the previously formatted file system into memory.
* `ls`: Lists the contents of the current directory.
* `mkdir <name>`: Creates a new directory.
* `touch <name>`: Creates a new empty file (using lazy allocation).
* `rm <name>`: Removes a file or an empty directory.
* `cd <path>`: Changes the current working directory.

## ⚠️ Known Limitations (Disclaimer)

Shellby is a **didactic project** (Proof of Concept) created to explore basic FAT architectures. Currently:
* It operates in a single-process environment (no concurrency management).
* It does not implement user permission systems.
* It lacks advanced data recovery mechanisms in case of system file corruption (no journaling).

## 📄 License

This project is licensed under the **Apache License 2.0**. See the `LICENSE` file for more details.
