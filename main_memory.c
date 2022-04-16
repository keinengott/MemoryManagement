#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main_memory.h"
#include "helpers.h"

struct MainMemory get_main_memory() {
    struct MainMemory main_mem;
    int i;
    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        main_mem.memory[i] = 0;
        main_mem.second_chance[i] = 0;
        main_mem.dirty[i] = 0;
    }
    return main_mem;
}

void init_page_table(int* page_table, int max_running_procs) {
    int i, total_pages = get_total_pages(max_running_procs);
    for (i = 0; i < total_pages; i++) {
        page_table[i] = 0;
    }
}

struct MemoryStats get_memory_stats() {
    struct MemoryStats stats = {
        .num_memory_accesses = 0,
        .num_seconds = 0,
        .proc_cnt = 0,
        .num_page_faults = 0,
        .num_seg_faults = 0,
        .total_mem_access_time = 0
    };

    return stats;
}


void print_statistics(FILE* fp, struct MemoryStats stats) {
    char buffer[1000];

    // Calculate
    float mem_access_per_ms = stats.num_memory_accesses / (stats.num_seconds * 1000);
    float page_fault_per_access = stats.num_page_faults / (float) stats.num_memory_accesses;
    float avg_mem_access_speed = stats.total_mem_access_time / (float) stats.num_memory_accesses;
    avg_mem_access_speed /= ONE_MILLION; // Convert from nanoseconds to milliseconds
    float throughput = stats.proc_cnt / stats.num_seconds;

    // Print
    sprintf(buffer, "Statistics\n");
    sprintf(buffer + strlen(buffer), "  %-28s: %'d\n", "Memory Accesses", stats.num_memory_accesses);
    sprintf(buffer + strlen(buffer), "  %-28s: %.2f\n", "Memory Accesses/Millisecond", mem_access_per_ms);
    sprintf(buffer + strlen(buffer), "  %-28s: %.2f\n", "Page Faults/Memory Access", page_fault_per_access);
    sprintf(buffer + strlen(buffer), "  %-28s: %'.2f ms\n", "Avg Memory Access Speed", avg_mem_access_speed); 
    sprintf(buffer + strlen(buffer), "  %-28s: %'d\n", "Segmentation Faults", stats.num_seg_faults);
    sprintf(buffer + strlen(buffer), "  %-28s: %.2f processes/sec\n", "Throughput", throughput);
    sprintf(buffer + strlen(buffer), "\n");
    
    print_and_write(buffer, fp);
}

int get_total_pages(int max_running_procs) {
    return max_running_procs * PROCESS_PAGES;
}

int get_start_index(int pid) {
    return pid * PROCESS_PAGES;
}

int get_end_index(int start_index) {
    return start_index + PROCESS_PAGES;
}

bool page_number_is_valid(int pid, int page_number) {
    // Returns whether or not page_number is valid given pid
    int start_index = get_start_index(pid);
    int end_index = get_end_index(start_index);
    if (page_number < start_index || page_number > end_index) {
        return 0;
    }
    return 1;
}

int get_frame_from_main_memory(int* main_mem, int page_number) {
    // Returns frame number of page if it is in main memory
    int i;
    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        if (page_number != main_mem[i]) {
            continue;
        }
        return i;
    }
    return -1;
}

void free_frames(struct MainMemory* main_mem, int* page_table, int pid) {
    int start_index = get_start_index(pid);
    int end_index = get_end_index(start_index);
    int i, frame_number;
    for (i = start_index; i < end_index; i++) {
        if (page_table[i] == 0) {
            // No entry in page table
            continue;
        }
        frame_number = page_table[i];
        main_mem->memory[frame_number] = 0;
        main_mem->dirty[frame_number] = 0;
        main_mem->second_chance[frame_number] = 0;
        page_table[i] = 0;
    }
    return;
}

int get_free_frame_number(int* main_mem) {
    int i;
    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        if (main_mem[i] != 0) {
            continue;
        }
        return i;
    }
    return -1; // No free frame
}

bool main_memory_is_full(int free_frame_number) {
    if (free_frame_number < 0) {
        return 1;
    }
    return 0;
}

int second_chance_page_replacement(struct MainMemory* main_mem) {
    // This function is assumes main memory is full (i.e., there is a page stored in every frame)
    int i, frame_number = -1;
    
    i = main_mem->second_chance_ptr;
    
    while (frame_number < 0) {
        // Reset index so that if at end then we just restart at the beginning
        if (i == MAIN_MEMORY_SZE) {
            i = 0;
        }
        
        if (main_mem->second_chance[i]) {
            // This frames second chance bit is set, so give it a second chance
            main_mem->second_chance[i] = 0;
        }
        else {
            frame_number = i;
        }

        i++;
    }

    main_mem->second_chance_ptr = i;

    return frame_number; 
}

void add_frame_to_page_table(int frame_number, int* page_table, int pid) {
    int start_index = get_start_index(pid);
    int end_index = get_end_index(start_index);
    int i;
    for (i = start_index; i < end_index; i++) {
        if (page_table[i] != 0) {
            continue;
        }
        // No entry in page table
        page_table[i] = frame_number;
        
        break;
    }
}

void print_frames(int* page_table, int pid) {
    int start_index = get_start_index(pid);
    int end_index = get_end_index(start_index);
    int i;
    printf("pages = %d\n", end_index - start_index);
    for (i = start_index; i < end_index; i++) {
        if (page_table[i] != 0) {
            printf("%3d ", page_table[i]);
        }
        else {
            printf(". ");
        }
    }
    printf("\n");
}

void print_main_memory(FILE* fp, struct MainMemory main_mem) {
    char buffer[1000];
    int i;
    sprintf(buffer, "Main Memory\n  ");
    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        if (main_mem.memory[i] == 0) {
            sprintf(buffer + strlen(buffer), ".");
        }
        else if (main_mem.dirty[i]) {
            sprintf(buffer + strlen(buffer), "D");
        }
        else {
            // Occupied Frame
            sprintf(buffer + strlen(buffer), "U");
        }
    }

    sprintf(buffer + strlen(buffer), "\n  ");

    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        if (main_mem.memory[i] == 0) {
            sprintf(buffer + strlen(buffer), ".");
        }
        else if (main_mem.second_chance[i]) {
            sprintf(buffer + strlen(buffer), "1");
        }
        else {
            // second chance bit is not set
            sprintf(buffer + strlen(buffer), "0");
        }
    }

    sprintf(buffer + strlen(buffer), "\n");

    print_and_write(buffer, fp);
}
