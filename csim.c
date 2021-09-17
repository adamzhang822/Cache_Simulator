#include "cachelab.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>


typedef struct trace{
    long int address;
    int tag;
    int set;
    char type;
}trace_t;

int create_mask(int bits){
    int mask = 0;
    for(int i=bits-1;i>=0;i--){
        mask += 1<<i;
    }
    return mask;
}

trace_t *create_trace(char op_type, long int addr, int s, int b){
    trace_t *new_trace = (trace_t *)malloc(sizeof(trace_t));
    if(new_trace == NULL){
        printf("Malloc failed for create_trace!!!");
        exit(0);
    }
    new_trace->address = addr;
    int mask_set = create_mask(s);
    new_trace->set = (addr>>b) & mask_set;
    new_trace->tag = addr>>(b+s);
    new_trace->type = op_type;
    return new_trace;
}

void print_trace(trace_t* trace){
    printf("The address of this trace is: %ld \n", trace->address);
    printf("The tag of this trace is: %d \n", trace->tag);
    printf("The set of this trace is: %d \n", trace->set);
    printf("The type of this trace is: %c \n", trace->type);
    return;
}

typedef struct cache_line{
    int valid_bit;
    int block_size;
    int tag;
    long int min_addr;
    long int max_addr;
    int last_used;
}cache_line_t;

cache_line_t *create_lines(int b){
    cache_line_t* lines = (cache_line_t*)malloc(8*sizeof(cache_line_t));
    if(lines == NULL){
        printf("Malloc failed for create_lines!!");
        exit(0);
    }
    if(lines == NULL){
        printf("Malloc failed!");
        exit(0);
    }
    for(int i=0;i<=7;i++){
        lines[i].valid_bit = 0;
        lines[i].block_size = 1<<b;
    }
    return lines;
}

typedef struct cache_set{
    int num_lines;
    cache_line_t *lines;
}cache_set_t;

cache_set_t *create_cache(int s, int E, int b){
    int num_sets = 1<<s;
    cache_set_t *cache = (cache_set_t*)malloc(num_sets*sizeof(cache_set_t));
    if(cache==NULL){
        printf("Malloc failed for create_cache!!");
        exit(0);
    }
    for(int i=0;i<=num_sets-1;i++){
        cache[i].lines = create_lines(b);
        cache[i].num_lines = E;
    }
    return cache;
}

void free_cache(cache_set_t* cache, int num_sets){
    for(int i=0;i<=num_sets-1;i++){
        free(cache[i].lines);
    }
    free(cache);
}

void write_to_line(cache_line_t *line, trace_t *trace){
    line->valid_bit = 1;
    line->tag = trace->tag;
    int block_size = line->block_size;
    long int address = trace->address;
    line->min_addr = (address/block_size) * block_size;
    //printf("writing to line now, the min_addr is: %ld \n",line->min_addr);
    line->max_addr = line->min_addr + (block_size - 1);
    //printf("writing to line now, the max_addr is: %ld \n", line->max_addr);
    line->last_used = 0;
    return;
}

int match_to_line(cache_line_t *line, trace_t *trace){
    if(line->valid_bit==0){
        return 0;
    }
    if(line->tag != trace->tag){
        return 0;
    }
    long int addr = trace->address;
    if((line->max_addr)>=addr && (line->min_addr)<=addr){
        line->valid_bit = 1;
        line->last_used = 0;
        return 1;
    }
    return 0;
}

void update_recency(cache_set_t *cache, int num_sets, int num_lines){
    for(int i=0;i<=num_sets-1;i++){
        for(int j=0;j<=num_lines-1;j++){
            cache[i].lines[j].last_used++;
        }
    }
    return;
}

int find_oldest_or_invalid(cache_set_t *set, int num_lines){
    int max_age = 0;
    int oldest;
    for(int i=0;i<=num_lines-1;i++){
        if(set->lines[i].valid_bit==0){
            return i;
        }
        if(set->lines[i].last_used > max_age){
            max_age = set->lines[i].last_used;
            oldest = i;
        }
    }
    return oldest;
}

void process_trace(cache_set_t *cache, trace_t *trace, int s, int* stats, int verbose){
    int set_number = trace->set;
    int num_lines = cache->num_lines;
    int num_sets = 1<<s;
    int found = 0;
    int to_write;
    cache_line_t *to_write_line;
    cache_set_t* set = cache+set_number;

    for(int i=0;i<=num_lines-1;i++){
        found = match_to_line(set->lines+i,trace);
        if(found){
            if(verbose){
                printf("hit \n");
            }
            stats[0]++;
            ((set->lines)+i)->last_used = 0;
            break;
        }
    }

    if(!found){
        to_write = find_oldest_or_invalid(set,num_lines);
        //printf("the set to be replaced is set: %d \n", to_write);
        to_write_line = set->lines+to_write;
        if(to_write_line->valid_bit){
            stats[1]++;
            stats[2]++;
            if(verbose){
                printf("miss eviction \n");
            }
            write_to_line(to_write_line,trace);
        }else{
            stats[1]++;
            if(verbose){
                printf("miss \n");
            }
            write_to_line(to_write_line,trace);
        }
    }
    update_recency(cache,num_sets,num_lines);
    return;
}



int main(int argc, char *argv[])
{
    int s = 4, E = 1, b = 4;
    int verbose = 0;
    int *stats = (int*)malloc(3*sizeof(int));
    char * file_path;
    FILE *fp;

    // Parsing arguements

    if(argc > 1){
        for (int count = 1; count < argc; count++){
            char * opt = argv[count];
            if(opt[0]=='-'){
                switch(opt[1]){
                    case 'v':
                        verbose = 1;
                        break;
                    case 's':
                        s = atoi(argv[count+1]);
                        count++;
                        break;
                    case 'E':
                        E = atoi(argv[count+1]);
                        count++;
                        break;
                    case 'b':
                        b = atoi(argv[count+1]);
                        count++;
                        break;
                    case 't':
                        file_path = argv[count+1];
                        count++;
                        break;
                    default:
                        printf("wrong arguement! exit the function");
                        exit(0);
                }
            }
        }
    }
    printf("s is %d, E is %d, b is %d, verbose is %d \n", s,E,b,verbose);
    int num_sets = 1<<s;

    cache_set_t *cache_sim = create_cache(s, E, b);

    fp = fopen(file_path,"r");
    char text[100];
    while(fgets(text,100,fp)!=NULL){
        char* temp = text;
        if(text[0]!='I'){
            if(verbose){
                printf("%s",text);
            }
            temp++;
            char type = temp[0];
            temp += 2;
            char address[16];
            sscanf(temp, "%[^,]",address);
            long int address_num = strtol(address,NULL,16);
            trace_t * current_trace = create_trace(type,address_num,s,b);
            process_trace(cache_sim, current_trace, s, stats, verbose);
            if(current_trace->type == 'M'){
                process_trace(cache_sim,current_trace,s,stats,verbose);
            }
            // print_trace(current_trace);
            free(current_trace);
        }
    }
    
    free_cache(cache_sim,num_sets);
    printSummary(stats[0], stats[1], stats[2]);
    return 0;
}

/* For debugging:
This is the final version! 
*/

       
