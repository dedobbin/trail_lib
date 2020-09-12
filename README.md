# trail_lib
Text adventure engine, work in progress. Quite some bugs, memory leaks, and unfinished parts. 
This is not the development repo but code mirror.

## TODO
- Code examples on how to use the library.
- There is something wrong with text rendering. When drawing character for character, it looks very wobbly. It's charming though, so maybe make optional to toggle to current behaviour.
- Valgrind to find and fix memory leaks. Probably good moment to introduce smart pointers.
- Player class. Currently the client has to take care of implementing this.
- Items default behaviour should be with player class. Currently the client has to take of implementing item behaviour.
