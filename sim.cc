#include <iostream>
#include <stdlib.h>
#include<stdio.h>
#include <bitset>
#include <inttypes.h>
#include <fstream>
#include <bits/stdc++.h>
#include <vector>
#include <math.h>
#include <sstream>
#include <map>
#include <string>
#include "sim.h"

using namespace std;

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/

struct L1_config{
   uint32_t L1_SET;
   uint32_t L1_INDEX; 
   uint32_t L1_BLOCKOFFSET;
   uint32_t L1_TAG;
   };
   
   
struct L2_config{
   uint32_t L2_SET;
   uint32_t L2_INDEX;
   uint32_t L2_BLOCKOFFSET;
   uint32_t L2_TAG;
   };

struct AddressComponents {
    uint32_t tag;
    uint32_t index;
    uint32_t blockoffset;
};
struct old_instr_components{
   uint32_t address;
   uint32_t indexbit;
   uint32_t tagbit;
};

class cache_content {
public:
    uint32_t tag;
    int lru;
    int dirty_bit;
    // Default constructor
    cache_content() : tag(0),dirty_bit(0){}
    };

class cache_statistics{
   public:
   int l1reads,l1read_miss,l1write,l1write_miss,l1write_backsl2,l2reads,l2read_miss,
   l2write,l2write_miss,l2write_backsMEM,MEMtraffic;
   // Default constructor
    cache_statistics() : l1reads(0), l1read_miss(0), l1write(0), l1write_miss(0),
                        l1write_backsl2(0), l2reads(0), l2read_miss(0),
                        l2write(0), l2write_miss(0), l2write_backsMEM(0), MEMtraffic(0) {}
};

cache_statistics stat;

void read_request(cache_content l2_cache[1000][200],cache_content level1cache[],uint32_t l1tagbits,
cache_content level2cache[],uint32_t l2tagbits, uint32_t l1_index_bits,uint32_t l1assoc,
uint32_t l2_index_bits,uint32_t l2assoc,uint32_t L1index,uint32_t L2index,int cachelvl);

void update_lru(cache_content lrucache[],uint32_t l1_index_bits,uint32_t assoc,int pos)
{int t=lrucache[pos].lru;

 for(uint32_t i=0;i<assoc;i++)
 { 
   if(lrucache[i].lru<t)
      lrucache[i].lru+=1;
 }
 lrucache[pos].lru=0;
}

int find_maxlru(cache_content lrucache[],uint32_t l1_index_bits,uint32_t assoc)
{
  int lruval=0,temp=-5;
  for(uint32_t i=0;i<assoc;i++){
    if(lrucache[i].lru>temp){
      lruval=i;
      temp=lrucache[i].lru;
    }
  }
  return lruval;
}

struct display
{
  int data;
  int dirty;
};

old_instr_components old_instr(uint32_t l1tagbits,uint32_t L1index,uint32_t L2index,
uint32_t l1_index_bits,uint32_t l2_index_bits)
{  old_instr_components old_inst;
   old_inst.address=(l1tagbits<<L1index)|l1_index_bits;
   old_inst.indexbit=((1<<L2index)-1)&old_inst.address;
   old_inst.tagbit=old_inst.address>>L2index;
   return old_inst;
}

void write_request(cache_content l2_cache[1000][200],cache_content level1cache[],uint32_t l1tagbits,
cache_content level2cache[],uint32_t l2tagbits, uint32_t l1_index_bits,uint32_t l1assoc,
uint32_t l2_index_bits,uint32_t l2assoc,uint32_t L1index,uint32_t L2index,int cachelvl) {
   
   int maxlru=0;
   old_instr_components old_instruction;
   //cout<<"cachelvl"<<cachelvl<<endl;
   if (cachelvl==1) {stat.l1write+=1;}

   else if(cachelvl==2) {stat.l2write+=1;}

   int flag=0;

   //check for write hit in L1
   if(cachelvl==1){
    for(uint32_t i=0;i<l1assoc;i++){
      if(level1cache[i].tag==l1tagbits)
         {level1cache[i].dirty_bit=1;
         flag=1;
         update_lru(level1cache,l1_index_bits,l1assoc,i);
         }
      }
   }
         
   //check for write hit in l2
   else if(cachelvl==2){
    for(uint32_t i=0;i<l2assoc;i++){
      if(level2cache[i].tag==l2tagbits){
         level2cache[i].dirty_bit=1;
         flag=1;
         update_lru(level2cache,l2_index_bits,l2assoc,i);
         }
      }
   } 
    
   //write miss, issuing read request to next level of memory ;
   if(flag==0){

      //write miss in cache L1
      if(cachelvl==1){
            stat.l1write_miss+=1;
            maxlru=find_maxlru(level1cache,l1_index_bits,l1assoc);

            if(level1cache[maxlru].dirty_bit==0){          
               level1cache[maxlru].tag=l1tagbits;
               update_lru(level1cache,l1_index_bits,l1assoc,maxlru);
               level1cache[maxlru].dirty_bit=1;  
               read_request(l2_cache,level1cache,l1tagbits,level2cache, l2tagbits, l1_index_bits, l1assoc,
               l2_index_bits,l2assoc,L1index,L2index,2);
            }

            else if(level1cache[maxlru].dirty_bit==1){
            //write code for eviction
            stat.l1write_backsl2+=1;
            old_instruction=old_instr(level1cache[maxlru].tag,L1index, L2index,l1_index_bits,l2_index_bits);
            write_request(l2_cache,level1cache,level1cache[maxlru].tag,l2_cache[old_instruction.indexbit], old_instruction.tagbit, 
            l1_index_bits, l1assoc, old_instruction.indexbit,l2assoc, L1index, L2index,2);
            level1cache[maxlru].tag=l1tagbits;
            update_lru(level1cache,l1_index_bits,l1assoc,maxlru);
            read_request(l2_cache,level1cache,l1tagbits,level2cache, l2tagbits, l1_index_bits, l1assoc,
            l2_index_bits,l2assoc,L1index,L2index,2);
            }
         }
      
   //write miss in L2         
   else{
         maxlru=find_maxlru(level2cache,l2_index_bits,l2assoc);
         stat.l2read_miss+=1;
         if(level2cache[maxlru].dirty_bit==0){
            level2cache[maxlru].dirty_bit=1;
            level2cache[maxlru].tag=l2tagbits;
            update_lru(level2cache,l2_index_bits,l2assoc,maxlru);
            }
         else if(level2cache[maxlru].dirty_bit==1){         
            stat.l2write_backsMEM+=1;
            stat.MEMtraffic+=1;    
            level2cache[maxlru].tag=l2tagbits;
            update_lru(level2cache,l2_index_bits,l2assoc,maxlru);       
         }
      }
    }
   
}

void read_request(cache_content l2_cache[1000][200], cache_content level1cache[],uint32_t l1tagbits,
cache_content level2cache[],uint32_t l2tagbits, uint32_t l1_index_bits,uint32_t l1assoc,
uint32_t l2_index_bits,uint32_t l2assoc,uint32_t L1index,uint32_t L2index,int cachelvl) {
   int maxlru=0, flag=0;
   old_instr_components old_instruction;
   
   if (cachelvl==1) {stat.l1reads+=1;}

   else if(cachelvl==2) {stat.l2reads+=1;}

   //check for read hit in L1
   if(cachelvl==1){
      for(uint32_t i=0;i<l1assoc;i++){
      if(level1cache[i].tag==l1tagbits){
         flag=1;\
         update_lru(level1cache,l1_index_bits,l1assoc,i);
         }
      }
   }

   //check for read hit in l2
   else if(cachelvl==2){
      for(uint32_t i=0;i<l2assoc;i++){
      if(level2cache[i].tag==l2tagbits){
         update_lru(level2cache,l2_index_bits,l2assoc,i);
         flag=1;
         }
      }
   }

   //read miss, issuing read in next level of memory
   if(flag==0)
   {
    //read miss in L1
    if(cachelvl==1){
      stat.l1read_miss+=1;     
         maxlru=find_maxlru(level1cache,l1_index_bits,l1assoc);
            update_lru(level1cache,l1_index_bits,l1assoc,maxlru); 
         if(level1cache[maxlru].dirty_bit==0){         
            read_request(l2_cache,level1cache,l1tagbits,level2cache, l2tagbits, l1_index_bits, l1assoc,
            l2_index_bits,l2assoc,L1index,L2index,2);
            }
         else if(level1cache[maxlru].dirty_bit==1){
            //write code for eviction
            stat.l1write_backsl2+=1;
            level1cache[maxlru].dirty_bit=0;
            old_instruction=old_instr(level1cache[maxlru].tag,L1index, L2index,l1_index_bits,l2_index_bits);
            //cout<<"old tag "<<old_instruction.tagbit<<"old index"<<old_instruction.indexbit;
            write_request(l2_cache,level1cache,level1cache[maxlru].tag,l2_cache[old_instruction.indexbit], old_instruction.tagbit, 
            l1_index_bits, l1assoc, old_instruction.indexbit,l2assoc, L1index, L2index,2);
            read_request(l2_cache,level1cache,l1tagbits,level2cache, l2tagbits, l1_index_bits, l1assoc,
            l2_index_bits,l2assoc,L1index,L2index,2);
      }
           
            level1cache[maxlru].tag=l1tagbits;       
      }

      //read miss in L2
      else{
         stat.l2read_miss+=1;
         stat.MEMtraffic+=1;
         maxlru=find_maxlru(level2cache,l2_index_bits,l2assoc);
         if(level2cache[maxlru].dirty_bit==1){
         level2cache[maxlru].dirty_bit=0;
         //issue write request to MEM
         stat.MEMtraffic+=1;
         stat.l2write_backsMEM+=1;
         }
         level2cache[maxlru].tag=l2tagbits;
         update_lru(level2cache,l2_index_bits,l2assoc,maxlru);
      }
   }
}


int main (int argc, char *argv[]) {
   std::string a;
   //AddressComponents L1_AddressComponents,L2_AddressComponents;
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];
   
   L1_config L1config;
   L2_config L2config;
   
   //Configuring L1 & L2 cache
   L1config.L1_SET = (params.L1_SIZE)/(params.L1_ASSOC*params.BLOCKSIZE);
   L1config.L1_INDEX = log2(L1config.L1_SET); 
   L1config.L1_BLOCKOFFSET = log2(params.BLOCKSIZE);
   L1config.L1_TAG = (32-L1config.L1_INDEX-L1config.L1_BLOCKOFFSET);
   L2config.L2_SET = 0;
   L2config.L2_INDEX = 0; 
   L2config.L2_BLOCKOFFSET = 0;
   L2config.L2_TAG = 0;
   if (params.L2_ASSOC!=0){
   L2config.L2_SET = (params.L2_SIZE)/(params.L2_ASSOC*params.BLOCKSIZE);
   L2config.L2_INDEX = log2(L2config.L2_SET); 
   L2config.L2_BLOCKOFFSET = log2(params.BLOCKSIZE);
   L2config.L2_TAG = (32-L2config.L2_INDEX-L2config.L2_BLOCKOFFSET);
   }
   
   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("PREF_N:     %u\n", params.PREF_N);
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);

   /*printf("===================================\n");
   printf("===== L1 Cache Configuration =====\n");
   printf("L1_SET:        %d\n",L1config.L1_SET);
   printf("L1_INDEX:      %d\n",L1config.L1_INDEX);
   printf("L1_BLOCKOFFSET %d\n",L1config.L1_BLOCKOFFSET);
   printf("L1_TAG:        %d\n",L1config.L1_TAG);
   printf("===================================\n");
   if (params.L2_ASSOC!=0){
   printf("===== L2 Cache Configuration =====\n");
   printf("L2_SET:        %d\n",L2config.L2_SET);
   printf("L2_INDEX:      %d\n",L2config.L2_INDEX);
   printf("L2_BLOCKOFFSET %d\n",L2config.L2_BLOCKOFFSET);
   printf("L2_TAG:        %d\n",L2config.L2_TAG);
   printf("===================================\n");
   }*/

   cache_content l1_cache[L1config.L1_SET][params.L1_ASSOC];
   cache_content l2_cache[1000][200];


   //initializing all the lru values of the caches
   for(uint32_t i=0;i<L1config.L1_SET;i++)
      {//int k=-1;
         for(uint32_t j=0;j<params.L1_ASSOC;j++)
         l1_cache[i][j].lru=j;
         //cout<<endl;
      }

   if(params.L2_ASSOC!=0)
   {
      for(uint32_t i=0;i<L2config.L2_SET;i++)
      {//int k=-1;
         for(uint32_t j=0;j<params.L2_ASSOC;j++)
            l2_cache[i][j].lru=j;
         //cout<<endl;
      }
   }
   
   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      /*if (rw == 'r')
      
         printf("r %x\n", addr);
      else if (rw == 'w')
         printf("w %x\n", addr);
      else {
         printf("Error: Unknown request type %c.\n", rw);
	 exit(EXIT_FAILURE);
      }*/
      
   AddressComponents L1_AddressComponents,L2_AddressComponents;
    // Calculate bitmask for each field
   uint32_t L1tagMask = (1u << L1config.L1_TAG) - 1;
   uint32_t L1indexMask = (1u << L1config.L1_INDEX) - 1;
   uint32_t L1blockoffsetMask = (1u << L1config.L1_BLOCKOFFSET) - 1;

    // Extract the fields
   L1_AddressComponents.tag = (addr >> (L1config.L1_INDEX + L1config.L1_BLOCKOFFSET)) & L1tagMask;
   L1_AddressComponents.index = (addr >> L1config.L1_BLOCKOFFSET) & L1indexMask;
   L1_AddressComponents.blockoffset = addr & L1blockoffsetMask;
   L2_AddressComponents.tag =0;
   L2_AddressComponents.index =0;
   L2_AddressComponents.blockoffset =0;

   if (params.L2_ASSOC!=0){
      uint32_t L2tagMask = (1u << L2config.L2_TAG) - 1;
      uint32_t L2indexMask = (1u << L2config.L2_INDEX) - 1;
      uint32_t L2blockoffsetMask = (1u << L2config.L2_BLOCKOFFSET) - 1;
      // Extract the fields
      L2_AddressComponents.tag = (addr >> (L2config.L2_INDEX + L2config.L2_BLOCKOFFSET)) & L2tagMask;
      L2_AddressComponents.index = (addr >> L2config.L2_BLOCKOFFSET) & L2indexMask;
      L2_AddressComponents.blockoffset = addr & L2blockoffsetMask;
      }
      
      /*cout<<L1_AddressComponents.tag<<endl;
      cout<<L1_AddressComponents.blockoffset<<endl;
      cout<<L1_AddressComponents.index<<endl;

      cout<<L2_AddressComponents.tag<<endl;
      cout<<L2_AddressComponents.blockoffset<<endl;
      cout<<L2_AddressComponents.index<<endl;*/

   if(rw=='w'){
      write_request(l2_cache,l1_cache[L1_AddressComponents.index],L1_AddressComponents.tag,
      l2_cache[L2_AddressComponents.index],L2_AddressComponents.tag,L1_AddressComponents.index,
      params.L1_ASSOC, L2_AddressComponents.index,params.L2_ASSOC,L1config.L1_INDEX,L2config.L2_INDEX,1);
      }
   else if(rw=='r'){
      read_request(l2_cache,l1_cache[L1_AddressComponents.index],L1_AddressComponents.tag,
      l2_cache[L2_AddressComponents.index],L2_AddressComponents.tag,L1_AddressComponents.index,
      params.L1_ASSOC, L2_AddressComponents.index,params.L2_ASSOC,L1config.L1_INDEX,L2config.L2_INDEX,1);
      }  
    }
cout<<endl;
cout << "===== L1 contents =====" << "\n";
for (uint32_t i = 0; i < L1config.L1_SET; i++){
   // printf("set\t%d:", i);
   cout<<"set";
   cout<<std::setw(7)<<i<<":";
   cout<<"\t";
   display temp[params.L1_ASSOC];
   for (uint32_t j = 0; j < params.L1_ASSOC; j++){
      temp[l1_cache[i][j].lru].data=l1_cache[i][j].tag;
      temp[l1_cache[i][j].lru].dirty=l1_cache[i][j].dirty_bit;
      }
   for(uint32_t j=0;j<params.L1_ASSOC;j++){
      if(temp[j].dirty==1){
         printf("%x D\t", temp[j].data);
         printf("\t");
      }     
      else{
         printf("%x\t", temp[j].data);
         printf("\t");
      }
   }
    cout << "\n";
  }
   if(params.L2_ASSOC!=0)
      printf("\n");
   if(params.L2_ASSOC!=0)
      cout << "===== L2 contents =====" << "\n";

  for (uint32_t i = 0; i < L2config.L2_SET; i++)
  {
    // printf("set\t%d:", i);
   cout<<"set";
   cout<<std::setw(7)<<i<<":";
   cout<<"\t";
   display temp[params.L2_ASSOC];
   for (uint32_t j = 0; j < params.L2_ASSOC; j++){
      temp[l2_cache[i][j].lru].data=l2_cache[i][j].tag;
      temp[l2_cache[i][j].lru].dirty=l2_cache[i][j].dirty_bit;     
      }
    for(uint32_t j=0;j<params.L2_ASSOC;j++){
      if(temp[j].dirty==1){
         printf("%x D\t", temp[j].data);
         printf("\t");
      }     
      else{
         printf("%x\t", temp[j].data);
         printf("\t");
      }
    }
    cout << "\n";
  }
  if(params.L2_ASSOC==0)
 {
  stat.l2read_miss=0;
  stat.l2reads=0;
  stat.l2write=0;
  stat.l2write_backsMEM=0;
  stat.l2write_miss=0;
 }
double l2_miss_rate=0.0000;
double l1_miss_rate=(double)(stat.l1write_miss +stat.l1read_miss)/(double)( stat.l1reads+stat.l1write);
if(params.L2_ASSOC)
   l2_miss_rate=(double)stat.l2read_miss /(double)stat.l2reads;
int x=0;
cout << dec << endl;
cout << "===== Measurements =====" << endl;
cout << "a. L1 reads:                   " << stat.l1reads << endl;
cout << "b. L1 read misses:             " << stat.l1read_miss << endl;
cout << "c. L1 writes:                  " << stat.l1write<< endl;
cout << "d. L1 write misses:            " << stat.l1write_miss << endl;
cout << "e. L1 miss rate:               " << setprecision(4) << fixed << l1_miss_rate << endl;
cout << "f. L1 writebacks:              " << stat.l1write_backsl2 << endl;
cout << "g. L1 prefetches:              " << x << endl;
cout << "h. L2 reads (demand):          " << stat.l2reads<< endl;
cout << "i. L2 read misses (demand):    " << stat.l2read_miss << endl;
cout << "j. L2 reads (prefetch):        " << x << endl;
cout << "k. L2 read misses (prefetch):  " << x << endl;
cout << "l. L2 writes:                  " << stat.l2write << endl;
cout << "m. L2 write misses:            " << stat.l2write_miss << endl;
cout << "n. L2 miss rate:               " << setprecision(4) << fixed << l2_miss_rate << endl;
cout << "o. L2 writebacks:              " << stat.l2write_backsMEM << endl;
cout << "p. L2 prefetches:              " << x << endl;
cout << "q. memory traffic:             " << stat.MEMtraffic << endl;
return(0);
}


