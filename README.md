# CacheSimulator
A C program which reads a trace of memory references and reports 3 statistics (accesses, hits and hit rate). The cache configuration is determined by parameters that are passed as command line arguments. The command-line parameters are cache size, cache mapping (DM/FA), and cache organization (Unified/Split).

# Running program

## VSCode

You can run this code in vscode using the <a href="https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools"><strong>C/C++</strong></a> extension along with the <a href="https://marketplace.visualstudio.com/items?itemName=formulahendry.code-runner"><strong>CodeRunner</strong></a> extension and using these workspace [**settings**](./.vscode)

<br />
Or you can run it in the termianl with these commands for compiling and executing.

## Compile 
`gcc -o cache_sim cache_sim.c`

## Run

`./cache_sim 1024 dm uc`

### CL arguments 

- 1024 is the cache size in bytes
- dm tells that the cache is directly mappend 
- uc signifies a unified cache

### The parameters can take the following values:

|   	|   	|
|---	|---	|
|  cache size 	        | 128 - 4096 (power of 2)     	              |  
|  cache mapping 	      | dm (Direct Mapped or fa (Fully associative) | 
|  cache organization 	| uc (Unified cache) or sc (Split Cache)  	  | 
