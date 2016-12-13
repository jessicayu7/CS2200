#include "reverselookup.h"
#include "pagetable.h"
#include "stats.h"
#include "tlb.h"

static uint64_t find_free_frame(stats_t *stats);

/**
 * This function is used to resolve a page fault. It finds a free frame, and returns
 * the PFN after doing some book keeping
 *
 * @param vpn The virtual page number that has to be looked up
 * @param rw Specifies if an access is a read or a write
 * @param stats The struct used to store stats
 *
 */
uint64_t page_fault_handler(uint64_t vpn, char rw, stats_t *stats)
{
	uint64_t old_pfn = find_free_frame(stats);
	task_struct *old_pcb;
	uint64_t old_vpn = rlt[old_pfn].vpn;

	old_pcb = rlt[old_pfn].task_struct;
	// return victim page frame number
	pte_t *old_pagetable;
	if(old_pcb) {
		old_pagetable = old_pcb ->pagetable;
		if (old_pagetable[old_vpn].valid) {
			if (old_pagetable[old_vpn].dirty) {
				stats -> writes_to_disk++;
			}
			old_pagetable[old_vpn].valid = 0;
			old_pagetable[old_vpn].frequency= 0;
			old_pagetable[old_vpn].dirty = 0;
		}

	}
	rlt[old_pfn].vpn = vpn;
	rlt[old_pfn].task_struct = current_process;
	rlt[old_pfn].valid = 1;

	current_pagetable[vpn].pfn=old_pfn;
	current_pagetable[vpn].valid = 1;
	return old_pfn;

}

/**
 * This functions finds a free frame by using the Least Frequently Used algorithm
 *
 * @param stats The struct used for keeping track of statistics
 *
 * @return The physical frame number that the page fault handler can use
 */
static uint64_t find_free_frame(stats_t *stats)
{
	// (1) Look for an invalid frame in the RLT - Return that frame
	// (2) Use Least frequently used to identify victim frame
	//		(a) Update the victim page table
	// 			(i) Use the RLT to find the page table of the victim process
	//			(ii) Mark it invalid
	//			(iii) Increment writebacks if the victim frame is dirty
	//		(b) Return the victim frame
	stats -> reads_from_disk++;
	uint64_t old_pfn = 0;


	uint64_t rltsize = 1 << rlt_size;	

	for (int i = 0; i <rltsize; i++) {
		if(!rlt[i].valid ){
			return i;

		}

	}
	//if there is already a invalid slot, we just use it without LFU rule
	
	uint64_t counter = rlt[0].task_struct -> pagetable[rlt[0].vpn].frequency;
		for (int j = 0; j < rltsize; j++) {
			//pte_t *foundpagetable = rlt[j].task_struct -> pagetable;
			uint64_t old_frequency = rlt[j].task_struct -> pagetable[rlt[j].vpn].frequency;
			if (old_frequency < counter) {
				old_pfn = j;
				counter = old_frequency;
			}
		}	
	


	// If the victim page table entry belongs to the current process, there is a chance
	// that it also resides in the TLB. So to avoid fake valid mappings from the TLB,
	// we clear that particular TLB entry.
	/******************* DO NOT MODIFY CODE BELOW THIS LINE **************/
	if (rlt[old_pfn].task_struct == current_process) {
		tlb_clearOne(rlt[old_pfn].vpn);
	}

	return old_pfn;
}