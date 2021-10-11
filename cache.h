#include <inttypes.h>

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

typedef struct {
  int valid;
  char *tag;
  char *block;
} cache;

void intialize_cache(cache cache[], uint32_t tag_value, uint32_t block_number);
void update_cache_accesses(mem_access_t access);
void update_cache_hits(mem_access_t access);
void update_cache_statistics(cache cache[], mem_access_t access);