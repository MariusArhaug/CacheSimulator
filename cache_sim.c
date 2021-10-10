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

  // Additional stats for split cache
  uint64_t instruction_accesses;
  uint64_t instruction_hits;

  uint64_t data_accesses;
  uint64_t data_hits; 
} cache_stat_t;

struct cache{
  int valid;
  char *tag;
  char *block;
};

typedef struct {
  int *arr;       // where T is the data type you're working with
  size_t length;  // the physical array length
  size_t head;    // location to pop from
  size_t tail;    // location to push to
  size_t count;   // number of elements in queue
} queue;

queue q;

uint32_t cache_size; 
uint32_t block_size = 64;
uint32_t bit_offset = 6;        // log2(64) = 6 
cache_map_t cache_mapping;
cache_org_t cache_org; 

uint32_t block_number;
uint32_t bit_index;
uint32_t tag;

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;


void enqueue(queue q, int new_value)
{
  q.arr[q.tail] = new_value;
  q.tail = ( q.tail + 1 ) % q.length;
  q.count++;
}

int dequeue(queue q) 
{
  int value = q.arr[q.head];
  q.count--;
  q.head = ( q.head + 1 ) % q.length;
  return value;
}
  
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

/*
 * Intitalize cache structs with tag memory, value and valid flags 
*/
void intialize_cache(struct cache cache[], uint32_t tag_value)
{
  for (int i = 0; i < block_number; i++) {
    // set buffer size for each tag with size of char * tag size 
    cache[i].tag = (char *)malloc(sizeof (char) * tag_value); 
    for (int j = 0; j < tag_value; j++) {
      // set cache tag a row of 0's
      cache[i].tag[j] = '0';
    }
    //  set cache valid flag to false
    cache[i].valid = 0;
  }
}

/*
 * Update cache acccesses depending on access type
*/
void update_cache_accesses(mem_access_t access) {
  cache_statistics.accesses++;
  switch(access.accesstype) {
    case instruction: cache_statistics.instruction_accesses++; break;
    case data: cache_statistics.data_accesses++; break;
  }
}

/* 
 * Update cache hits depending on access type
*/
void update_cache_hits(mem_access_t access) {
  cache_statistics.hits++; 
  switch (access.accesstype) {
    case instruction: cache_statistics.instruction_hits++; break;
    case data: cache_statistics.data_hits++; break;
  }
}

void update_cache_statistics(struct cache cache[], mem_access_t access) 
{
  uint32_t offset_bits = (access.address >> 0) & ((1 << bit_offset) -1);
  uint32_t index_bits = (access.address >> bit_offset) & ((1 << bit_index) -1);
  uint32_t tag_bits = (access.address >> bit_offset+bit_index) & ((1 << tag) -1);

  char tag_str[32];
  update_cache_accesses(access);
  switch (cache_mapping) {
    case dm: {
      sprintf(tag_str, "%x", tag_bits);
      if (cache[index_bits].valid) {  
        if (strcmp(cache[index_bits].tag, tag_str) == 0) {
          update_cache_hits(access);
        } else {
          strcpy(cache[index_bits].tag, tag_str);
        }
      } 
      else {
        strcpy(cache[index_bits].tag, tag_str);
        cache[index_bits].valid = 1;
      }
    } break;

    case fa: {
      sprintf(tag_str, "%x", (tag_bits + index_bits));

      // check for hits in whole cache arr
      for (int i = 0; i < block_number; i++) {
        if (cache[i].valid && (strcmp(cache[i].tag, tag_str) == 0)) {
          update_cache_hits(access);
          return;
        }
      }
      
      // update empty cache blocks 
      for (int i = 0; i < block_number; i++) {
        if (!cache[i].valid) {
         strcpy(cache[i].tag, tag_str);
         cache[i].valid = 1;
         enqueue(q, i);
         return;
        }
      }

      int j = dequeue(q);
      strcpy(cache[j].tag, tag_str);
      cache[j].valid = 1;
      enqueue(q, j);
    }
  }
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
  block_number = cache_size / block_size;  
  
  q.arr = malloc( sizeof *q.arr * block_number );

  if (q.arr) {
    q.length = block_number;
    q.head = 0;
    q.tail = 0;
  }

  struct cache unified_cache[block_number];
  struct cache instruction_cache[block_number];
  struct cache data_cache[block_number];
 
  switch (cache_mapping) {
    case dm: {
      bit_index = log2(block_number);
      tag = 32 - (bit_offset + bit_index);
    } break;

    case fa: {
      bit_index = 0;
      tag = 32 - bit_offset;
    }
  }

  switch(cache_org) {
    case uc: {
      intialize_cache(unified_cache, tag);
    } break;

    case sc: {
      intialize_cache(instruction_cache, tag);
      intialize_cache(data_cache, tag);
    }
  }
  
  mem_access_t access;
  
  while(1) {
    access = read_transaction(ptr_file);
    
    if (access.address == 0) {
      break;
    }

    switch (cache_org) {
      case uc: update_cache_statistics(unified_cache, access); break;
      case sc: {
        switch (access.accesstype) {
          case instruction: update_cache_statistics(instruction_cache, access); break;
          case data: update_cache_statistics(data_cache, access); break;
        }
      } break;

    }
  }

  printf("\nCache Configurations\n\n");
  printf("------------------------\n");
  printf("Size:                 %d bytes\n", cache_size);
  printf("Mapping:              %s\n", mapping_type_to_string(cache_mapping));
  printf("Organization:         %s\n", organization_type_to_string(cache_org));
  printf("Policy:               %s\n", "FIFO");
  printf("Block size:           %d bytes\n", block_size);
  printf("Num of blocks:        %d blocks\n", block_number);
  printf("Num bits for offset:  %d bits\n", bit_offset);
  printf("Num bits for index:   %d bits\n", bit_index);
  printf("Num bits for tag:     %d bits\n\n", tag);
  printf("------------------------\n");

  printf("\nCache Statistics\n");
  printf("------------------------\n\n");
  printf("Accesses: %ld\n", cache_statistics.accesses);
  printf("Hits:     %ld\n", cache_statistics.hits);
  printf("Hit Rate: %.4f\n\n", (double) cache_statistics.hits / cache_statistics.accesses);

  /* If split cache, print their stats */
  switch (cache_org) {
    case sc: {
      printf("DCache Accesses: %ld\n", cache_statistics.data_accesses);
      printf("DCache Hits:     %ld\n", cache_statistics.data_hits);
      printf("DCache Hit rate: %.4f\n\n", (double) cache_statistics.data_hits / cache_statistics.data_accesses);

      printf("ICache Accesses: %ld\n", cache_statistics.instruction_accesses);
      printf("ICache Hits:     %ld\n", cache_statistics.instruction_hits);
      printf("ICache Hit rate: %.4f\n", (double) cache_statistics.instruction_hits / cache_statistics.instruction_accesses);
    } break;
  }

  /* Close the trace file */
  fclose(ptr_file);

}