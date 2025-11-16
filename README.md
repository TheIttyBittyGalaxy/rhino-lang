# 🦏 Rhino

Rhino is a work in progress programming language. Contributions welcome! :)

## Build

[`/build_program/build.py`](/build_program/build.py) is a Python script which will generate compiler source code and place this in [`/compiler/include`](/compiler/include). The `bbuild` batch script will execute this script using Python.

After that, the C program can be built. The `build` batch script will compile the program using g++.

```
> bbuild
> build
```

## Run

```
> rhino <file-path>
```

## Scripts

### Compiler

| Script  | Description                |
| ------- | -------------------------- |
| `build` | Build the interpreter      |
| `comp`  | Run the interpreter        |
| `debug` | Run the interpreter in gdb |
| `run`   | Run the rhino interpreter  |

### Test program

| Script  | Description            |
| ------- | ---------------------- |
| `btest` | Build the test program |
| `rtest` | Run the test program   |
