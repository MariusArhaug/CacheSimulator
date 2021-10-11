#include "cache.h"

uint32_t cache_size; 
uint32_t block_size = 64;
uint32_t bit_offset = 6;        // log2(64) = 6 
cache_map_t cache_mapping;
cache_org_t cache_org; 

uint32_t block_number;
uint32_t bit_index;
uint32_t tag;

uint32_t q_index;

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;

/*
 * Intitalize cache structs with tag memory, value and valid flags 
*/
void intialize_cache(cache cache[], uint32_t tag_value, uint32_t block_number)
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
void update_cache_accesses(mem_access_t access) 
{
  cache_statistics.accesses++;
  switch(access.accesstype) {
    case instruction: cache_statistics.instruction_accesses++; break;
    case data: cache_statistics.data_accesses++; break;
  }
}

/* 
 * Update cache hits depending on access type
*/
void update_cache_hits(mem_access_t access) 
{
  cache_statistics.hits++; 
  switch (access.accesstype) {
    case instruction: cache_statistics.instruction_hits++; break;
    case data: cache_statistics.data_hits++; break;
  }
}

void update_cache_statistics(cache cache[], mem_access_t access) 
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
     
      int is_in_cache = 0;
      for (int i = 0; i < block_number; i++) {
        if (cache[i].valid && (strcmp(cache[i].tag, tag_str) == 0)) {
          update_cache_hits(access);
          is_in_cache = 1;
          break;
        }
      }

      if (is_in_cache == 0) {
        int has_updated_cache = 0;
        for (int i = 0; i < block_number; i++) {
          if (cache[i].valid == 0) {
            strcpy(cache[i].tag, tag_str); 
            cache[i].valid = 1;
            has_updated_cache = 1;
            break;
          }
        }
        if (has_updated_cache == 0) {
          strcpy(cache[q_index].tag, tag_str);
          cache[q_index].valid = 1;
          q_index = (q_index + 1) % block_number;
        }
      }
    }
  }
}

void start(char** argv) {
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
  FILE *ptr_file = fopen("mem_trace.txt","r");
  if (!ptr_file) {
    printf("Unable to open the trace file\n");
    exit(1);
  }
  block_number = cache_size / block_size;  

  q_index = 0;

  cache unified_cache[block_number];
  cache instruction_cache[block_number];
  cache data_cache[block_number];
 
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
  
  
  while(1) {
    mem_access_t access = read_transaction(ptr_file);
    if (access.address == 0) {
      break;
    }
    // printf("%x\n", access.address);

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