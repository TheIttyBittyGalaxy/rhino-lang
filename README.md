# Badger 🦡
A programming language I am creating for my own enjoyment and, hopefully, for use on future projects! :D

## Rough Roadmap


### Phase 1: Front-end
Create an interpreted version of the language. The goal is to design the language; hone in the front-end of the compiler (syntax, semantics, error reporting); get the language into a stage where I can use it an try it out. The back-end implementation will be "simple and discardable", just a 'good enough' AST walker. This will include a simple but non-final version of the core library.
   
- Parser
- Analyiser
- Interpreter
- Crude core library for debug, file IO, console IO, HTTP IO


### Phase 2: Back-end 
Remove the interpreter and make Badger a compiled language. This will be the primary goal. That said, insights from phase 1 and from trying to properly implment the language may inform changes to the language's design. I think I have no choice but to cross this bridge when I get to it?

- Lowering pass
- Optimisation pass
- "Build for platform" pass 


### Phase 3: Core library
This is what I expect I will be worse at, and it will be the most labour intensive given the changing needs of different platforms. At this point, however, I will want to flesh out the core library in more detail so that the language can be more feature rich.

- Core library
- Ability to call from DLLs
<!-- 
    - IO
    - HTTP IO
    - WebSockets IO
    - File IO
    - Networking IO
- Text handling
    - CSV
    - Markdown
    - HTML?? (and CSS??)
    - XML
    - JSON
    - String formatting
- Image handling
    - PNG, JPG, etc
- Memory
    - Allocators
    - Sorrting
- Graphics / rendering
    - 2D
    - 3D
- Math
- Random
- UUID
- Date/time
- Debug tools 
-->


### Phase 4: The tail end
By this point the language should be feature complete. But, there will still be a need to address technical debt, open issues, bugs, etc, etc. 
