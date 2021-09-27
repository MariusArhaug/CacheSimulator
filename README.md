# CacheSimulator
A C program which reads a trace of memory references and reports 3 statistics (accesses, hits and hit rate). The cache configuration is determined by parameters that are passed as command line arguments. The command-line parameters are cache size, cache mapping (DM/FA), and cache organization (Unified/Split).

## Compile 
`gcc -o cache_sim cache_sim.c`

## Run

`./cache_sim 1024 dm uc`

### CL arguments 

- 1024 is the cache size in bytes
- dm tells that the cache is directly mappend 
- uc signifies a unified code

### The parameters can take the following values:

|   	|   	|
|---	|---	|
|  cache size 	        | 128 - 4096 (power of 2)     	              |  
|  cache mapping 	      | dm (Direct Mapped or fa (Fully associative) | 
|  cache organization 	| uc (Unified cache) or se (Split Cache)  	  | 
