# Alpha Compiler
This repository contains a multi-phase compiler for the Alpha programming language, implemented for the HY-340 course at the University of Crete (2024–2025).
    
## Project structure
    
- `Phase_1` &ndash; **Lexical Analysis** implemented in Flex.
- `Phase_2` &ndash; **Syntax Analysis** using Bison with a symbol table.
- `Phase_3` &ndash; **Intermediate Code Generation** producing quads.
- `Phases_4_5` &ndash; **Final Code Generation** targeting a stack-based
 virtual machine.
    
    Each phase directory includes source code under `code/`, a Makefile, and
 tests.
    
## Building
To build a specific phase, move into its `code/` directory and run `make
`:
```bash
cd Phase_1/code
make
```
Executables are produced in the same directory. Cleaning is done with `make clean`.
    
## Team
    
- **csd5171** &ndash; Fytaki Maria
- **csd5310** &ndash; Rafail Drakakis
- **csd5082** &ndash; Theologos Kokkinellis
