## README

### Brief
My experiment of a more expression-oriented, terse, and dynamic scripting language. Written in C23 with extensions dialect and might be used by myself if it matures enough.

### Requirements
 - Recent Clang / GCC compiler version
 - CMake 3.31
 - Ninja build tool (CMake presets need this)

#### To Do's
 1. Test parser. **WIP**
 2. Add bytecode types.
    - Register-based opcodes
    - NaN boxed value??
    - Create Shapes and ICs.
 3. Add generic "pools" for objects.
 4. Make bytecode compiler & test it.
 5. Make VM using Shapes and ICs.
 6. Refactor things to use a unified "driver" interface.
 7. Add GC.
 8. Test further on larger scripts.

### Other Notes
The first few commits are larger since I forgot to create the repo initially & I had to compensate with retroactive commits. No AI was used.
