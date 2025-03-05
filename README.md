# HY-340 Project 2024-2025 Phase 1: Lexical Analyzer

# Team Members:
    csd5171 - Fytaki Maria
    csd5310 - Rafail Drakakis
    csd5082 - Theologos Kokkinellis

# Project Overview
    Lexical analyzer implemented using Flex for the HY-340 course (C).
    It identifies tokens, and handles comments and strings and 
    detects lexical errors.

# Features
**Token identification** : identifies 
    - keywords, 
    - operators, 
    - integer constants, 
    - real constants,
    - strings,
    - punctuation,
    - identifiers and
    - comments.

**String Handling** : 
    Supports strings ("...") and detects unclosed strings.
    Supports escape sequences such as \n, \t, \", \\ .

**Comment Handling** : Supports (using stacks)
    - single-line (//) comments,
    - multi-line (/* ... */) comments, 
    - nested comments (/* /* ... */ */) comments. 
    Detects unclosed block comments.

**Error Handling**: Detects 
    - undefined characters,
    - unclosed strings, 
    - unclosed comments,
    ==> prints according error messages each time.

**Token List** : Stores all tokens in a linked list and prints formatted 
    output according to FAQ.

# Compilation
    We provide a makefile. We then run the lexer on a source test file 
    and the output is written to output.txt file. Then "cat output.txt"
    to obtain.
