#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

typedef enum {dm, fa} cache_map_t;
typedef enum {uc, sc} cache_org_t;
typedef enum {instruction, data} access_t;

typedef struct {
  uint32_t address;
  access_t accesstype;
} mem_access_t;

typedef struct {
  uint64_t accesses;
  uint64_t hits;
} cache_stat_t;

// custome fifo queue struct
typedef struct {
  uint32_t *store; 
  size_t size;
  size_t count;
  size_t head;
  size_t tail;
} queue;

struct cache{
  int valid;
  char *tag;
  char *block;
};
// DECLARE CACHES AND COUNTERS FOR THE STATS HERE


uint32_t cache_size; 
uint32_t block_size = 64;
uint32_t bit_offset = 6;        // log2(64) = 6 
cache_map_t cache_mapping;
cache_org_t cache_org;

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;


/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE *ptr_file) {
  char buf[1000];
  char* token;
  char* string = buf;
  mem_access_t access;
  if (fgets(buf,1000, ptr_file)!=NULL) {

    /* Get the access type */
    token = strsep(&string, " \n");        
    if (strcmp(token,"I") == 0) {
      access.accesstype = instruction;
    } else if (strcmp(token,"D") == 0) {
      access.accesstype = data;
    } else {
      printf("Unkown access type\n");
      exit(0);
    }  

    token = strsep(&string, " \n");
    access.address = (uint32_t)strtol(token, NULL, 16);

    return access;
  }

  /* If there are no more entries in the file,  
    * return an address 0 that will terminate the infinite loop in main
    */
  access.address = 0;
  return access;
}

/*
* Helper method to print various types
*/
const char* access_type_to_string(access_t access_type) 
{
  switch (access_type)
  {
    case instruction: return "Instruction";
    case data:        return "Data";
  }
}

const char* mapping_type_to_string(cache_map_t mapping_type) 
{
  switch(mapping_type) 
  {
    case fa: return "fully associative";
    case dm: return "direct mapped";
  }
}

const char* organization_type_to_string(cache_org_t org_type) 
{
  switch (org_type) 
  {
    case uc: return "unified cache";
    case sc: return "split cache";
  }
}

// struct cache *unified_cache_config(uint32_t cache_size) {
//   uint32_t block_number = cache_size / block_size;
//   uint32_t bit_index = log2(block_number);
//   uint32_t tag = 32 - (block_number + bit_index);
//   struct cache unified_cache[block_number];  
//   uint32_t sets = block_number;

//   intialize_cache(unified_cache, block_number, tag);
//   print_cache_configuration(cache_size, block_number, bit_index, tag);

//   return unified_cache;
// }

/*
 * Intitalize cache structs with tag memory, value and valid flags 
*/
void intialize_cache(struct cache cache[], uint32_t block_number, uint32_t tag)
{
  for (int i = 0; i < block_number; i++) {
    // set buffer size for each tag with size of char * tag size 
    cache[i].tag = (char *)malloc(sizeof (char) * tag); 
    for (int j = 0; j < tag; j++) {
      // set cache tag a row of 0's
      cache[i].tag[j] = '0';
    }
    //  set cache valid flag to false
    cache[i].valid = 0;
  }
}

void print_cache_configuration(uint32_t cache_size, uint32_t block_number, uint32_t bit_index, uint32_t tag) 
{
  printf("\nCache Configurations\n");
  printf("--------------------\n\n");
  printf("Size: %24d / 2048 bytes\n", cache_size);
  printf("Mapping: %30s\n", mapping_type_to_string(cache_mapping));
  printf("Organization: %23s\n", organization_type_to_string(cache_org));
  printf("Block size: %16d bytes\n", block_size);
  printf("Num of blocks: %13d blocks\n", block_number);
  printf("Num bits for offset: %6d bits\n", bit_offset);
  printf("Num bits for index: %7d bits\n", bit_index);
  printf("Num bits for tag: %10d bits\n\n", tag);
}

void main(int argc, char** argv)
{
  // Reset statistics:
  memset(&cache_statistics, 0, sizeof(cache_stat_t));

  /* Read command-line parameters and initialize:
  * cache_size, cache_mapping and cache_org variables
  */

  if ( argc != 4 ) { /* argc should be 2 for correct execution */
    printf("Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] [cache organization: uc|sc]\n");
    exit(0);
  } 
  /* argv[0] is program name, parameters start with argv[1] */

  /* Set cache size */
  cache_size = atoi(argv[1]);

  /* Set Cache Mapping */
  if (strcmp(argv[2], "dm") == 0) {
    cache_mapping = dm;
  } else if (strcmp(argv[2], "fa") == 0) {
    cache_mapping = fa;
  } else {
    printf("Unknown cache mapping\n");
    exit(0);
  }

  /* Set Cache Organization */
  if (strcmp(argv[3], "uc") == 0) {
    cache_org = uc;
  } else if (strcmp(argv[3], "sc") == 0) {
    cache_org = sc;
  } else {
    printf("Unknown cache organization\n");
    exit(0);
  }
    


    /* Open the file mem_trace.txt to read memory accesses */
  FILE *ptr_file = fopen("mem_trace2.txt","r");
  if (!ptr_file) {
    printf("Unable to open the trace file\n");
    exit(1);
  }

  
  

  struct cache data_cache;
  struct cache instruction_cache;
  mem_access_t access;

  uint32_t block_number = cache_size / block_size;
  uint32_t bit_index = log2(block_number);
  uint32_t tag = 32 - (bit_offset + bit_index);
  struct cache unified_cache[block_number];  
  uint32_t sets = block_number;
  

  /* Loop until whole trace file has been read */
  intialize_cache(unified_cache, block_number, tag);
  while(1) {
    access = read_transaction(ptr_file);
    //If no transactions left, break out of loop
    if (access.address == 0) {
      break;
    }
    //printf("%d %x\n",access.accesstype, access.address);
    //printf("%-11s %2x\n", access_type_to_string(access.accesstype), access.address);

    // split address uo into offset bits, index bits and tag bits. 
    uint32_t offset_bits = (access.address >> 0) & ((1 << bit_offset) -1);
    uint32_t index_bits = (access.address >> bit_offset) & ((1 << bit_index) -1);
    uint32_t tag_bits = (access.address >> bit_offset+bit_index) & ((1 << tag) -1);

    


    char tag_str[32];
    sprintf(tag_str, "%x", tag_bits);
    //printf("%s\n", tag_str);
    if (unified_cache[index_bits].valid == 0) 
    {
      unified_cache[index_bits].valid = 1;
      strcpy(unified_cache[index_bits].tag, tag_str);
    }

    if (unified_cache[index_bits].valid == 1 && (strcmp(unified_cache[index_bits].tag, tag_str) == 0)) {

      cache_statistics.hits++;
      cache_statistics.accesses++;
    } else {
       
      cache_statistics.accesses++;
      
      strcpy(unified_cache[index_bits].tag, tag_str);

      
    }

  }
  /* Print cache configurations */
  print_cache_configuration(cache_size, block_number, bit_index, tag);
  /* Print the statistics */
  // DO NOT CHANGE THE FOLLOWING LINES!
  printf("\nCache Statistics\n");
  printf("--------------------\n\n");
  printf("Accesses: %ld\n", cache_statistics.accesses);
  printf("Hits:     %ld\n", cache_statistics.hits);
  printf("Hit Rate: %.4f\n", (double) cache_statistics.hits / cache_statistics.accesses);
  // You can extend the memory statistic printing if you like!

  /* Close the trace file */
  fclose(ptr_file);

}