#include "cachesim.h"

#define TRUE 1
#define FALSE 0

/**
 * The stuct that you may use to store the metadata for each block in the L1 and L2 caches
 */
typedef struct block_t {
    uint64_t tag; // The tag stored in that block
    uint8_t valid; // Valid bit
    uint8_t dirty; // Dirty bit
    uint64_t lru;
} block;


/**
 * A struct for storing the configuration of both the L1 and L2 caches as passed in the
 * cache_init function. All values represent number of bits. You may add any parameters
 * here, however I strongly suggest not removing anything from the config struct
 */
typedef struct config_t {
    uint64_t C1; /* Size of cache L1 */
    uint64_t C2; /* Size of cache L2 */
    uint64_t S; /* Set associativity of L2 */
    uint64_t B; /* Block size of both caches */
} config;

typedef struct set {
    block *block;
}set;

config* configlah;
int counter;
set* cache_l1;
set* cache_l2;


/****** Do not modify the below function headers ******/
static uint64_t get_tag(uint64_t address, uint64_t C, uint64_t B, uint64_t S);
static uint64_t get_index(uint64_t address, uint64_t C, uint64_t B, uint64_t S);
static uint64_t convert_tag(uint64_t tag, uint64_t index, uint64_t C1, uint64_t C2, uint64_t B, uint64_t S);
static uint64_t convert_index(uint64_t tag, uint64_t index, uint64_t C1, uint64_t C2, uint64_t B, uint64_t S);
static uint64_t convert_tag_l1(uint64_t l2_tag, uint64_t l2_index, uint64_t C1, uint64_t C2, uint64_t B, uint64_t S);
static uint64_t convert_index_l1(uint64_t l2_tag, uint64_t l2_index, uint64_t C1, uint64_t C2, uint64_t B, uint64_t S);
int replace (uint64_t index, uint64_t S);
/****** You may add Globals and other function headers that you may need below this line ******/





/**
 * Subroutine for initializing your cache with the passed in arguments.
 * You may initialize any globals you might need in this subroutine
 *
 * @param C1 The total size of your L1 cache is 2^C1 bytes
 * @param C2 The tatal size of your L2 cache is 2^C2 bytes
 * @param S The total number of blocks in a line/set of your L2 cache are 2^S
 * @param B The size of your blocks is 2^B bytes
 */
void cache_init(uint64_t C1, uint64_t C2, uint64_t S, uint64_t B){
    counter = 0;
    int blocks_per_set_l1 = 1<<0;
    int blocks_per_set_l2 = 1<<S;
    int set_num_1 = 1 << (C1 - 0 - B);
    int set_num_2 = 1 << (C2 - S - B);
    cache_l1 = (set*) (malloc(set_num_1 * sizeof(set)));//set up l1 cache
    cache_l2 = (set*) (malloc(blocks_per_set_l2* sizeof(set)));// set up l2 cache
    configlah = (config*) malloc(sizeof(config));
    configlah -> C1 = C1;
    configlah -> C2 = C2;
    configlah -> S = S;
    configlah -> B = B;
    for (int i = 0; i < blocks_per_set_l1; i++) {
        cache_l1[i].block = (block*) (malloc(set_num_1 * sizeof(block)));
        for (int j = 0; j < set_num_1; j++) {
            cache_l1[i].block[j].dirty = 0;
            cache_l1[i].block[j].tag = 0;
            cache_l1[i].block[j].valid = 0;
        }
    }

    //initiate l2
    for (int i = 0; i < blocks_per_set_l2; i++) {
        cache_l2[i].block = (block*) (malloc(set_num_2 * sizeof(block)));
        for (int j = 0; j < set_num_2; j++) {
            cache_l2[i].block[j].dirty = 0;
            cache_l2[i].block[j].tag = 0;
            cache_l2[i].block[j].valid = 0;
            cache_l2[i].block[j].lru = 0;
        }
    }
}

/**
 * Subroutine that simulates one cache event at a time.
 * @param rw The type of access, READ or WRITE
 * @param address The address that is being accessed
 * @param stats The struct that you are supposed to store the stats in
 */
void cache_access (char rw, uint64_t address, struct cache_stats_t *stats)
{
    counter++;
    stats -> accesses++;
    if (rw == 'w') {
        stats -> writes++;
    } else {
        stats -> reads++;
    }
    uint64_t C1 = configlah -> C1;
    uint64_t C2 = configlah -> C2;
    uint64_t S = configlah -> S;   
    uint64_t B = configlah -> B;
    uint64_t index = get_index(address, C1, B, 0);
    uint64_t tag = get_tag(address, C1, B, 0);
    uint64_t tag2 = get_tag(address,C2,B,S);
    uint64_t index2 = get_index(address,C2,B,S);

    int l2_is_empty= 0;
    int l1_hit = 0;
    if (cache_l1[0].block[index].valid && cache_l1[0].block[index].tag == tag) {
        l1_hit = 1;
        for (int i = 0; i < 1<< S; i++) {
            int reached = 0;
            if (!reached) {
                int walid = cache_l2[i].block[index2].valid;
                int tag_equal_l2 = cache_l2[i].block[index2].tag == tag2;
                if(walid && tag_equal_l2) {
                    cache_l2[i].block[index2].lru = counter;
                    reached =1;
                }
            }
        } // update correspoding l2's lru
    }
    if (l1_hit) {//if l1 hits
        if (rw == 'w') {
            cache_l1[0].block[index].tag = tag;
            cache_l1[0].block[index].dirty = 1;
            cache_l1[0].block[index].valid = 1;
        } else {
            cache_l1[0].block[index].tag = tag;
            cache_l1[0].block[index].valid = 1;
        }
    } else { //else if l1 misses
        if (rw == 'w'){
            stats->l1_write_misses++;
        } else {
            stats->l1_read_misses++;
        }

        int l2_hit = 0;
        int l2_empty_index;
        //check if l1 is dirty, if dirty, then update corresponding l2
        if(cache_l1[0].block[index].dirty){
            uint64_t newtag1 = cache_l1[0].block[index].tag;
            uint64_t newtag2 = convert_tag(newtag1, index,C1,C2,B,S);
            uint64_t newindex2 = convert_index(newtag1,index,C1,C2,B,S);
            for(int k = 0; k < 1<<S; k++) {
                int reached = 0;
                if (!reached) {
                    if (cache_l2[k].block[newindex2].valid && cache_l2[k].block[newindex2].tag == newtag2) {
                        cache_l2[k].block[newindex2].dirty = 1;
                        reached = 1;
                    }
                }
            }
        }
        cache_l1[0].block[index].dirty = 0;
        int i2;
        for(i2 = 0; i2 < 1<<S; i2++) {
            
            if(cache_l2[i2].block[index2].valid==0) {
                if(!l2_is_empty) {
                    l2_empty_index = i2;
                }
                l2_is_empty = 1; //l2 has empty spot
            }
            //simultaneously check whether l2 hits
            if (cache_l2[i2].block[index2].valid && cache_l2[i2].block[index2].tag == tag2) {
                l2_hit = 1;
                cache_l2[i2].block[index2].lru = counter; //check l2 lru
                break;
            }
        }

        if (l2_hit) {
            //uint64_t new_l1_index = convert_index_l1(tag2, index2, C1,C2,B,S);
            if (rw == 'w') {
                cache_l2[i2].block[index2].dirty = 1;
            }
        } else { //if l2 doesnt hit
            if (rw == 'w') {
                stats->l2_write_misses++;
            } else {
                stats->l2_read_misses++;
            }

            if(l2_is_empty) { //if has empty spot, we dont have to use lru
                cache_l2[l2_empty_index].block[index2].tag = tag2;
                cache_l2[l2_empty_index].block[index2].valid = 1;
                if (rw == 'w') {
                    cache_l2[l2_empty_index].block[index2].dirty = 1;
                } else {
                    cache_l2[l2_empty_index].block[index2].dirty = 0;
                }
                cache_l2[l2_empty_index].block[index2].lru = counter;
            } else { // if has no empty spot, we use lru
                int new_index = replace(index2, configlah ->S);
                uint64_t newtag2 = cache_l2[new_index].block[index2].tag;
                uint64_t tag11 = convert_tag_l1(newtag2, index2, C1, C2, B, S);
                uint64_t index11 = convert_index_l1(newtag2,index2,C1,C2,B,S);
                if(cache_l2[new_index].block[index2].dirty) {
                    stats->write_backs++;

                }
                if (tag11 == cache_l1[0].block[index11].tag ) {
                    cache_l1[0].block[index11].valid = 0;
                    cache_l1[0].block[index11].dirty = 0;

                } //evict the corresponding l1
                if (rw == 'w') {
                    cache_l2[new_index].block[index2].dirty = 1;
                } else {
                    cache_l2[new_index].block[index2].dirty = 0;
                }
                cache_l2[new_index].block[index2].lru= counter;
                cache_l2[new_index].block[index2].valid = 1;
                cache_l2[new_index].block[index2].tag = tag2;
            }
        } 


    }
    cache_l1[0].block[index].valid = 1;
    cache_l1[0].block[index].tag = tag;

}
        
    




int replace (uint64_t index, uint64_t S) {
    int new_lru = cache_l2[0].block[index].lru;
    int new_index = 0;
    for(int i = 0; i < 1<<S; i++) {
        if (new_lru > cache_l2[i].block[index].lru) {
            new_lru = cache_l2[i].block[index].lru;
            new_index = i;
        }
    }
    return new_index;
}

/**
 * Subroutine for freeing up memory, and performing any final calculations before the statistics
 * are outputed by the driver
 */
void cache_cleanup (struct cache_stats_t *stats)
{

    stats->read_misses = stats->l1_read_misses + stats->l2_read_misses;
    stats->write_misses = stats->l1_write_misses + stats->l2_write_misses;
    stats->misses = stats->read_misses + stats->write_misses;
    stats->l1_miss_rate = (stats->l1_read_misses + stats->l1_write_misses)/(stats->accesses*1.0);
    stats->l2_miss_rate = (stats->l2_read_misses + stats->l2_write_misses)/((stats->l1_read_misses + stats->l1_write_misses)*1.0);
    stats->miss_rate = (stats->misses)/(1.0 * stats->accesses);
    stats->l2_avg_access_time = stats->l2_access_time + stats->l2_miss_rate*stats->memory_access_time;
    stats->avg_access_time = stats->l1_access_time + stats->l1_miss_rate * stats->l2_avg_access_time;
    free(cache_l1);
    uint64_t S = configlah -> S;
    for(int i = 0; i < 1<< S; i++) {
         free(cache_l2[i].block);
    }
    free(cache_l2);
    free(configlah);


   
}

/**
 * Subroutine to compute the Tag of a given address based on the parameters passed in
 *
 * @param address The address whose tag is to be computed
 * @param C The size of the cache in bits (i.e. Size of cache is 2^C)
 * @param B The size of the cache block in bits (i.e. Size of block is 2^B)
 * @param S The set associativity of the cache in bits (i.e. Set-Associativity is 2^S)
 * 
 * @return The computed tag
 */
static uint64_t get_tag(uint64_t address, uint64_t C, uint64_t B, uint64_t S)
{
    /**************** TODO ******************/
  //   return C-B;
 
    uint64_t mask = ~(1<< (64 - C +S));
    return (address >> (C - S)) & mask;
}

/**
 * Subroutine to compute the Index of a given address based on the parameters passed in
 *
 * @param address The address whose tag is to be computed
 * @param C The size of the cache in bits (i.e. Size of cache is 2^C)
 * @param B The size of the cache block in bits (i.e. Size of block is 2^B)
 * @param S The set associativity of the cache in bits (i.e. Set-Associativity is 2^S)
 *
 * @return The computed index
 */
static uint64_t get_index(uint64_t address, uint64_t C, uint64_t B, uint64_t S)
{
    /**************** TODO ******************/
    return (address>>B)&((1<<(C-B-S))-1);
}


/**** DO NOT MODIFY CODE BELOW THIS LINE UNLESS YOU ARE ABSOLUTELY SURE OF WHAT YOU ARE DOING ****/

/*
    Note:   The below functions will be useful in converting the L1 tag and index into corresponding L2
            tag and index. These should be used when you are evicitng a block from the L1 cache, and
            you need to update the block in L2 cache that corresponds to the evicted block.

            The newly added functions will be useful for converting L2 indecies ang tags into the corresponding
            L1 index and tags. Make sure to understand how they are working.
*/

/**
 * This function converts the tag stored in an L1 block and the index of that L1 block into corresponding
 * tag of the L2 block
 *
 * @param tag The tag that needs to be converted (i.e. L1 tag)
 * @param index The index of the L1 cache (i.e. The index from which the tag was found)
 * @param C1 The size of the L1 cache in bits
 * @param C2 The size of the l2 cache in bits
 * @param B The size of the block in bits
 * @param S The set associativity of the L2 cache
 */
static uint64_t convert_tag(uint64_t tag, uint64_t index, uint64_t C1, uint64_t C2, uint64_t B, uint64_t S)
{
    uint64_t reconstructed_address = (tag << (C1 - B)) | index;
    return reconstructed_address >> (C2 - B - S);
}

/**
 * This function converts the tag stored in an L1 block and the index of that L1 block into corresponding
 * index of the L2 block
 *
 * @param tag The tag stored in the L1 index
 * @param index The index of the L1 cache (i.e. The index from which the tag was found)
 * @param C1 The size of the L1 cache in bits
 * @param C2 The size of the l2 cache in bits
 * @param B The size of the block in bits
 * @param S The set associativity of the L2 cache
 */
static uint64_t convert_index(uint64_t tag, uint64_t index, uint64_t C1, uint64_t C2, uint64_t B,  uint64_t S)
{
    // Reconstructed address without the block offset bits
    uint64_t reconstructed_address = (tag << (C1 - B)) | index;
    // Create index mask for L2 without including the block offset bits
    return reconstructed_address & ((1 << (C2 - S - B)) - 1);
}

/**
 * This function converts the tag stored in an L2 block and the index of that L2 block into corresponding
 * tag of the L1 cache
 *
 * @param l2_tag The L2 tag
 * @param l2_index The index of the L2 block
 * @param C1 The size of the L1 cache in bits
 * @param C2 The size of the l2 cache in bits
 * @param B The size of the block in bits
 * @param S The set associativity of the L2 cache
 * @return The L1 tag linked to the L2 index and tag
 */
static uint64_t convert_tag_l1(uint64_t l2_tag, uint64_t l2_index, uint64_t C1, uint64_t C2, uint64_t B, uint64_t S) {
    uint64_t reconstructed_address = (l2_tag << (C2 - B - S)) | l2_index;
    return reconstructed_address >> (C1 - B);
}

/**
 * This function converts the tag stored in an L2 block and the index of that L2 block into corresponding
 * index of the L1 block
 *
 * @param l2_tag The L2 tag
 * @param l2_index The index of the L2 block
 * @param C1 The size of the L1 cache in bits
 * @param C2 The size of the l2 cache in bits
 * @param B The size of the block in bits
 * @param S The set associativity of the L2 cache
 * @return The L1 index of the L2 block
 */
static uint64_t convert_index_l1(uint64_t l2_tag, uint64_t l2_index, uint64_t C1, uint64_t C2, uint64_t B, uint64_t S) {
    uint64_t reconstructed_address = (l2_tag << (C2 - B - S)) | l2_index;
    return reconstructed_address & ((1 << (C1 - B)) - 1);
}
