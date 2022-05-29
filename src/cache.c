//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"

//
// TODO:Student Information
//
const char *studentName = "Harsh Gondaliya";
const char *studentID   = "A9001613";
const char *email       = "hgondali@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//TODO: Add your Cache data structures here
//
uint32_t icacheTagBits;
uint32_t dcacheTagBits;
uint32_t l2cacheTagBits;
uint32_t *icache;
uint32_t *dcache;
uint32_t *l2cache;
uint8_t *icacheLRU;
uint8_t *dcacheLRU;
uint8_t *l2cacheLRU;


//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //
  icache = (uint32_t*)malloc(icacheSets * icacheAssoc * sizeof(uint32_t));
  dcache = (uint32_t*)malloc(dcacheSets * dcacheAssoc * sizeof(uint32_t));
  l2cache = (uint32_t*)malloc(l2cacheSets * l2cacheAssoc * sizeof(uint32_t));

  icacheLRU = (uint8_t*)malloc(icacheSets * icacheAssoc * sizeof(uint8_t));
  dcacheLRU = (uint8_t*)malloc(dcacheSets * dcacheAssoc * sizeof(uint8_t));
  l2cacheLRU = (uint8_t*)malloc(l2cacheSets * l2cacheAssoc * sizeof(uint8_t));
  for(int i=0; i<(icacheSets * icacheAssoc); i++){
    icache[i] = 0;
    icacheLRU[i] = 9; // 9 represents empty
  }
  for(int i=0; i<(dcacheSets * dcacheAssoc); i++){
    dcache[i] = 0;
    dcacheLRU[i] = 9;
  }
  for(int i=0; i<(l2cacheSets * l2cacheAssoc); i++){
    l2cache[i] = 0;
    l2cacheLRU[i] = 9;
  }
  // printf("dcacheLRU[(64*4)-1]: %d\n",dcacheLRU[((dcacheSets-1) * dcacheAssoc)+1]);
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  // printf("address: %d\n",addr);
  uint32_t penalty = 0;
  // compute tag and index of I-Cache
  uint32_t iIndexBits = log2(icacheSets);
  uint32_t iBlockOffsetBits = log2(blocksize);
  uint32_t iTagBits = 32 - iIndexBits - iBlockOffsetBits;

  uint32_t iIndexMask = (1 << iIndexBits) - 1;
  iIndexMask = iIndexMask << iBlockOffsetBits;

  uint32_t iTagMask = (1 << iTagBits) - 1;
  iTagMask = iTagMask << (iBlockOffsetBits + iIndexBits);

  uint32_t iTag = (addr & iTagMask);
  iTag = iTag >> (iBlockOffsetBits + iIndexBits);
  uint32_t iIndex = (addr & iIndexMask); 
  iIndex = iIndex >> iBlockOffsetBits;
  // printf("iTag: %d\n", iTag);
  // printf("iIndex: %d\n", iIndex);
  uint32_t iSuccess = 0;
  // attempt access in I-Cache. If hit, then return I-cache hit time.
  if(icacheSets){
    icacheRefs++;
    for(int i=0; i<icacheAssoc; i++){
      // printf("icacheLRU[(iIndex*icacheAssoc)+i]: %d\n", icacheLRU[(iIndex*icacheAssoc)+i]);
      if(icacheLRU[(iIndex*icacheAssoc)+i] < 9){
        if((icache[(iIndex*icacheAssoc)+i] ) == iTag){
          iSuccess = 1;
          // printf("tag found\n");
          if(!(icacheLRU[(iIndex*icacheAssoc)+i] == icacheAssoc-1)){
            icacheLRU[(iIndex*icacheAssoc)+i] = icacheAssoc-1;
            for(int k=0; k<icacheAssoc; k++){
              if((k != i) && (icacheLRU[(iIndex*icacheAssoc)+k]!=9)){
                if(icacheLRU[(iIndex*icacheAssoc)+k]!=0){
                  icacheLRU[(iIndex*icacheAssoc)+k] = icacheLRU[(iIndex*icacheAssoc)+k]-1;
                }
              }
            }
          }
          break;
        }
      }
    }
    if(!iSuccess){
      // printf("iTag miss\n");
      icacheMisses++;
      penalty += l2cache_access(addr);
      uint32_t l1ReplacementSuccess = 0;
      for(int j=0; j<icacheAssoc; j++){
        // printf("icacheLRU[(iIndex*icacheAssoc)+j]: %d\n", icacheLRU[(iIndex*icacheAssoc)+j]);
        if(icacheLRU[(iIndex*icacheAssoc)+j] == 9){ // free space available in L1
          icache[(iIndex*icacheAssoc)+j] = iTag;
          if(!(icacheLRU[(iIndex*icacheAssoc)+j] == icacheAssoc-1)){
            icacheLRU[(iIndex*icacheAssoc)+j] = icacheAssoc-1;
            for(int k=0; k<icacheAssoc; k++){
              if((k != j) && (icacheLRU[(iIndex*icacheAssoc)+k]!=9)){
                if(icacheLRU[(iIndex*icacheAssoc)+k]!=0){
                  icacheLRU[(iIndex*icacheAssoc)+k] = icacheLRU[(iIndex*icacheAssoc)+k]-1;
                }
              }
            }
          }
          l1ReplacementSuccess = 1;
          // printf("icache line filled\n");
          break;
        }
      }
      if(!l1ReplacementSuccess){
        for(int j=0; j<icacheAssoc; j++){
          // printf("icacheLRU[(iIndex*icacheAssoc)+j]: %d\n", icacheLRU[(iIndex*icacheAssoc)+j]);
          if(icacheLRU[(iIndex*icacheAssoc)+j] == 0){ // replacement in L1
            icache[(iIndex*icacheAssoc)+j] = iTag;
            if(!(icacheLRU[(iIndex*icacheAssoc)+j] == icacheAssoc-1)){
              icacheLRU[(iIndex*icacheAssoc)+j] = icacheAssoc-1;
              for(int k=0; k<icacheAssoc; k++){
                if((k != j) && (icacheLRU[(iIndex*icacheAssoc)+k]!=9)){
                  if(icacheLRU[(iIndex*icacheAssoc)+k]!=0){
                    icacheLRU[(iIndex*icacheAssoc)+k]=icacheLRU[(iIndex*icacheAssoc)+k]-1;
                  }
                }
              }
            }
            break;
          }
        }
        // printf("icache line replaced\n");
      }
    }
    icachePenalties += penalty;
    return penalty + icacheHitTime;
  }
  // if no l1 cache and need to directly move to l2 cache; if l2 doesnt exist no need to update its metadata
  else{
    return l2cache_access(addr);
  }
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  // printf("address: %d\n", addr);
  uint32_t penalty = 0;
  // compute tag and index of I-Cache
  uint32_t dIndexBits = log2(dcacheSets);
  uint32_t dBlockOffsetBits = log2(blocksize);
  uint32_t dTagBits = 32 - dIndexBits - dBlockOffsetBits;

  uint32_t dIndexMask = (1 << dIndexBits) - 1;
  dIndexMask = dIndexMask << dBlockOffsetBits;

  uint32_t dTagMask = (1 << dTagBits) - 1;
  dTagMask = dTagMask << (dBlockOffsetBits + dIndexBits);

  uint32_t dTag = (addr & dTagMask);
  dTag = dTag >> (dBlockOffsetBits + dIndexBits);
  uint32_t dIndex = (addr & dIndexMask); 
  dIndex = dIndex >> dBlockOffsetBits;
  
  // printf("dTag: %d\n", dTag);
  // printf("dIndex: %d\n",dIndex);
  uint32_t dSuccess = 0;
  // attempt access in I-Cache. If hit, then return I-cache hit time.
  if(dcacheSets){
    // printf("hi\n");
    dcacheRefs++;
    for(int i=0; i<dcacheAssoc; i++){
      // printf("Checking for hit\n");
      // printf("dcacheLRU[(dIndex*dcacheAssoc)+i]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+i]);
      if(dcacheLRU[(dIndex*dcacheAssoc)+i] < 9){
        if((dcache[(dIndex*dcacheAssoc)+i] ) == dTag){
          dSuccess = 1;
          if(!(dcacheLRU[(dIndex*dcacheAssoc)+i] == dcacheAssoc-1)){
            dcacheLRU[(dIndex*dcacheAssoc)+i] = dcacheAssoc-1;
            for(int k=0; k<dcacheAssoc; k++){
              if((k != i) && (dcacheLRU[(dIndex*dcacheAssoc)+k]!=9)){
                // printf("dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                if(dcacheLRU[(dIndex*dcacheAssoc)+k]!=0){
                  // printf("Before: dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                  dcacheLRU[(dIndex*dcacheAssoc)+k]=dcacheLRU[(dIndex*dcacheAssoc)+k]-1;
                  // printf("After: dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                }
              }
            }
          }
          // printf("tag found at index %d\n", i);
          break;
        }
      }
    }
    if(!dSuccess){
      // printf("tag miss\n");
      dcacheMisses++;
      penalty += l2cache_access(addr);
      uint32_t l1ReplacementSuccess = 0;
      for(int j=0; j<dcacheAssoc; j++){
        // printf("Search for empty slot\n");
        // printf("dcacheLRU[(dIndex*dcacheAssoc)+j]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+j]);
        if(dcacheLRU[(dIndex*dcacheAssoc)+j] == 9){ // free space available in L1
          dcache[(dIndex*dcacheAssoc)+j] = dTag;
          if(!(dcacheLRU[(dIndex*dcacheAssoc)+j] == dcacheAssoc-1)){
            dcacheLRU[(dIndex*dcacheAssoc)+j] = dcacheAssoc-1;
            for(int k=0; k<dcacheAssoc; k++){
              if((k != j) && (dcacheLRU[(dIndex*dcacheAssoc)+k]!=9)){
                // printf("dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                if(dcacheLRU[(dIndex*dcacheAssoc)+k]!=0){
                  // printf("Before: dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                  dcacheLRU[(dIndex*dcacheAssoc)+k]=dcacheLRU[(dIndex*dcacheAssoc)+k]-1;
                  // printf("After: dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                }
              }
            }
          }
          // printf("tag filled at index %d\n",j);
          l1ReplacementSuccess = 1;
          break;
        }
      }
      if(!l1ReplacementSuccess){
        for(int j=0; j<dcacheAssoc; j++){
          // printf("Search for replacement\n");
          // printf("dcacheLRU[(dIndex*dcacheAssoc)+j]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+j]);
          if(dcacheLRU[(dIndex*dcacheAssoc)+j] == 0){ // replacement in L1
            dcache[(dIndex*dcacheAssoc)+j] = dTag;
            if(!(dcacheLRU[(dIndex*dcacheAssoc)+j] == dcacheAssoc-1)){
              dcacheLRU[(dIndex*dcacheAssoc)+j] = dcacheAssoc-1;
              for(int k=0; k<dcacheAssoc; k++){
                if((k != j) && (dcacheLRU[(dIndex*dcacheAssoc)+k]!=9)){
                  // printf("dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                  if(dcacheLRU[(dIndex*dcacheAssoc)+k]!=0){
                    // printf("Before: dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                    dcacheLRU[(dIndex*dcacheAssoc)+k]=dcacheLRU[(dIndex*dcacheAssoc)+k]-1;
                    // printf("After: dcacheLRU[(dIndex*dcacheAssoc)+k]: %d\n", dcacheLRU[(dIndex*dcacheAssoc)+k]);
                  }
                }
              }
            }
            // printf("tag replaced at index %d\n", j);
            break;
          }
        }
      }
    }
    dcachePenalties += penalty;
    return penalty + dcacheHitTime;
  }
  // if no l1 cache and need to directly move to l2 cache; if l2 doesnt exist no need to update its metadata
  else{
    return l2cache_access(addr);
  }
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  // printf("address: %d\n", addr);
  uint32_t penalty = 0;

  // compute tag and index of L2-Cache
  uint32_t l2IndexBits = log2(l2cacheSets);
  uint32_t l2BlockOffsetBits = log2(blocksize);
  uint32_t l2TagBits = 32 - l2IndexBits - l2BlockOffsetBits;

  uint32_t l2IndexMask = (1 << l2IndexBits) - 1;
  l2IndexMask = l2IndexMask << l2BlockOffsetBits;

  uint32_t l2TagMask = (1 << l2TagBits) - 1;
  l2TagMask = l2TagMask << (l2BlockOffsetBits + l2IndexBits);

  uint32_t l2Tag = (addr & l2TagMask);
  l2Tag = l2Tag >> (l2BlockOffsetBits + l2IndexBits);
  uint32_t l2Index = (addr & l2IndexMask); 
  l2Index = l2Index >> l2BlockOffsetBits;

  uint32_t l2Success = 0;
  // printf("tag: %d\n", l2Tag);
  // printf("index: %d\n", l2Index);

  if(l2cacheSets){
    l2cacheRefs++;
    for(int i=0; i<l2cacheAssoc; i++){
      if(l2cacheLRU[(l2Index*l2cacheAssoc)+i] < 9){
        if((l2cache[(l2Index*l2cacheAssoc)+i] ) == l2Tag){
          l2Success = 1;
          if(!(l2cacheLRU[(l2Index*l2cacheAssoc)+i] == l2cacheAssoc-1)){
            l2cacheLRU[(l2Index*l2cacheAssoc)+i] = l2cacheAssoc-1;
            for(int k=0; k<l2cacheAssoc; k++){
              if((k != i) && (l2cacheLRU[(l2Index*l2cacheAssoc)+k]!=9)){
                if(l2cacheLRU[(l2Index*l2cacheAssoc)+k]!=0){
                  l2cacheLRU[(l2Index*l2cacheAssoc)+k]=l2cacheLRU[(l2Index*l2cacheAssoc)+k]-1;
                }
              }
            }
          }
          break;
        }
      }
    }
    if(!l2Success){
      l2cacheMisses++;
      penalty += memspeed;
      uint32_t l2ReplacementSuccess = 0;
      for(int j=0; j<l2cacheAssoc; j++){
        if(l2cacheLRU[(l2Index*l2cacheAssoc)+j] == 9){ // free space available in L1
          l2cache[(l2Index*l2cacheAssoc)+j] = l2Tag;
          if(!(l2cacheLRU[(l2Index*l2cacheAssoc)+j] == l2cacheAssoc-1)){
            l2cacheLRU[(l2Index*l2cacheAssoc)+j] = l2cacheAssoc-1;
            for(int k=0; k<l2cacheAssoc; k++){
              if((k != j) && (l2cacheLRU[(l2Index*l2cacheAssoc)+k]!=9)){
                if(l2cacheLRU[(l2Index*l2cacheAssoc)+k]!=0){
                  l2cacheLRU[(l2Index*l2cacheAssoc)+k]=l2cacheLRU[(l2Index*l2cacheAssoc)+k]-1;
                }
              }
            }
          }
          l2ReplacementSuccess = 1;
          break;
        }
      }
      if(!l2ReplacementSuccess){
        for(int j=0; j<l2cacheAssoc; j++){
          if(l2cacheLRU[(l2Index*l2cacheAssoc)+j] == 0){ // replacement in L1
            l2cache[(l2Index*l2cacheAssoc)+j] = l2Tag;
            if(!(l2cacheLRU[(l2Index*l2cacheAssoc)+j] == l2cacheAssoc-1)){
              l2cacheLRU[(l2Index*l2cacheAssoc)+j] = l2cacheAssoc-1;
              for(int k=0; k<l2cacheAssoc; k++){
                if((k != j) && (l2cacheLRU[(l2Index*l2cacheAssoc)+k]!=9)){
                  if(l2cacheLRU[(l2Index*l2cacheAssoc)+k]!=0){
                    l2cacheLRU[(l2Index*l2cacheAssoc)+k]=l2cacheLRU[(l2Index*l2cacheAssoc)+k]-1;
                  }
                }
              }
            }
            break;
          }
        }
      }
    }
    l2cachePenalties += penalty;
    return penalty + l2cacheHitTime;
  }
  // if no l1 cache and need to directly move to l2 cache; if l2 doesnt exist no need to update its metadata
  else{
    return memspeed;
  }
}