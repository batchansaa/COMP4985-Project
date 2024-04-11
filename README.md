# COMP 4985 Project Guide

This server manager project was built for COMP 4985 @ BCIT.

## Authors:
Batchansaa Batzorig

Alexander Gibbison

## **Table of Contents**

1. [Cloning the Repository](#cloning-the-repository)
2. [Prerequisites](#Prerequisites)
3. [Running the `generate-cmakelists.sh` Script](#running-the-generate-cmakelistssh-script)
4. [Running the `change-compiler.sh` Script](#running-the-change-compilersh-script)
5. [Running the `build.sh` Script](#running-the-buildsh-script)
5. [Running the `build-all.sh` Script](#running-the-build-allsh-script)
6. [Copy the template to start a new project](#copy-the-template-to-start-a-new-project)

## **Cloning the Repository**

Clone the repository using the following command:

```bash
git clone https://github.com/batchansaa/COMP4985-Project
```

Ensure the scripts are executable:

```bash
chmod +x *.sh
```

## ! Note ! - You can run without building the project

If you want to run the server manager program without building the project, you can do the following steps instead.

``` bash
cd src
```

``` bash
gcc main.c -o main -lncurses
```

``` bash
./main
```

## **Prerequisites**

- to ensure you have all of the required tools installed, run:
```bash
./check-env.sh
```

If you are missing tools follow these [instructions](https://docs.google.com/document/d/1ZPqlPD1mie5iwJ2XAcNGz7WeA86dTLerFXs9sAuwCco/edit?usp=drive_link).

## **Running the generate-cmakelists.sh Script**

You will need to create the CMakeLists.txt file:

```bash
./generate-cmakelists.sh
```

## **Running the change-compiler.sh Script**

Tell CMake which compiler you want to use:

```bash
./change-compiler.sh -c <compiler>
```

To the see the list of possible compilers:

```bash
cat supported_cxx_compilers.txt
```

## **Running the build.sh Script**

To build the program run:

```bash
./build.sh
```

## **Running the build-all.sh Script**

To build the program with all compilers run:

```bash
./build-all.sh
```

## **Running the program**

To start running the program, write this command in terminal: 

```bash
./build/main
```
