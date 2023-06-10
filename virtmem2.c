/**
 * virtmem.c 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define TLB_SIZE    16

#define PAGE_BITS   10
#define PAGES       (1 << PAGE_BITS)
#define PAGE_MASK   (PAGES - 1) << PAGE_BITS
#define PAGE_SIZE   PAGES

#define OFFSET_BITS 10
#define OFFSET_MASK ((1 << OFFSET_BITS) - 1)

#define FRAMES 256

#define MEMORY_SIZE PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
    int reference_bit;
    unsigned int logical;
    unsigned int physical;
};

// TLB is kept track of as a circular array,
// with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];

// number of inserts into TLB that have been completed.
// Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page.
// Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int max(int a, int b) {
    if (a > b)
        return a;
    return b;
}

// Returns the physical address from TLB or -1 if not present.
int search_tlb(unsigned int logical_page) {
    /* TODO */
    for(int i = 0; i < TLB_SIZE; i++) {
        if(tlb[i].logical == logical_page) {
            return tlb[i].physical;
        }
    }

    return -1;
}

// Adds the specified mapping to the TLB,
// replacing the oldest mapping (FIFO replacement).
void add_to_tlb(unsigned int logical, unsigned int physical) {
    /* TODO */

    while(1){

        if ( tlb[tlbindex].reference_bit != 1 ) {

            tlb[tlbindex].reference_bit = 1;
            tlb[tlbindex].logical = logical;
            tlb[tlbindex].physical = physical;
            tlbindex = ((tlbindex + 1) % TLB_SIZE);
            return;
        }

        tlb[tlbindex].reference_bit = 0;

        tlbindex = ((tlbindex + 1) % TLB_SIZE);

    }

}

int leastRecentlyUsed(int* references, int page_faults){

	if ( page_faults <= FRAMES ) { return page_faults - 1; } 		

  	int least_used = INT_MAX;
  	int value = 0;

  	for ( int i = 0; i < PAGES; i++ ) {
        if ( pagetable[i] != -1 ) {
            if ( value == 0 || value > references[i] ) {
                least_used = i;
                value = references[i];
            }
		}
  	}
	return pagetable[least_used]; 

}

int main(int argc, const char *argv[]) {
        
    if (argc != 5) {
        fprintf(stderr, "Usage ./virtmem backingstore input -p (0 or 1) \n");
        exit(1);
    }

    const char *backing_filename = argv[1];
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    int replacement_choice = atoi(argv[4]);

    // Fill page table entries with -1 for initially empty table.
    int i;
    for (i = 0; i < PAGES; i++) {
        pagetable[i] = -1;
    }

    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];

    // Data we need to keep track of to compute stats at end.
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;

    // Number of the next unallocated physical page in main memory
    unsigned int free_page = 0;

    int references[PAGES];
    for (int i = 0; i < PAGES; i++) {
        references[i] = 0;
    }

    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
        total_addresses++;
        int logical_address = atoi(buffer);

        // TODO
        // Calculate the page offset and logical page number from logical_address
        int offset = logical_address & OFFSET_MASK;
        int logical_page = (logical_address & PAGE_MASK) >> OFFSET_BITS;

        references[logical_page] = total_addresses;

        int physical_page = search_tlb(logical_page);

        // TLB hit
        if (physical_page != -1) {
            tlb_hits++;
            // TLB miss
        } else {
            physical_page = pagetable[logical_page];

            // Page fault
            if (physical_page == -1) {
                /* TODO */
                page_faults++;

                if ( replacement_choice == 0 ) {

                    physical_page = free_page;
                    free_page++;

                } else if ( replacement_choice == 1 ) {

                    physical_page = leastRecentlyUsed(references, page_faults);

                }

                memcpy(physical_page * PAGE_SIZE + main_memory, logical_page * PAGE_SIZE + backing, PAGE_SIZE);

                for(int i = 0; i < PAGES; i++){
                    if(pagetable[i] == physical_page){
                        pagetable[i] = -1;
                    }
                }

                pagetable[logical_page] = physical_page;
            }

            add_to_tlb(logical_page, physical_page);

        }

        int physical_address = (physical_page << OFFSET_BITS) | offset;
        signed char value = main_memory[physical_page * PAGE_SIZE + offset];

        // TODO: revert to normal when finished
        printf("Accessing logical:  %d\n", logical_page);
        printf("Virtual address: %d, Physical address: %d Value: %d\n",
               logical_address, physical_address, value);
    }

    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}
