#include "pagetable.h"
#include <assert.h>
/**
 * This function checks the page table of the current process to find
 * the VPN to PFN mapping.
 * 
 * @param vpn The virtual page number that has to be translated
 * @param offset The page offset of the address that is being translated
 * @param rw Specifies if access is read or a write
 * @param stats The struct used for storing statistics
 *
 * @return The page frame number (aka physical frame number) PFN linked to the VPN
 */
uint64_t page_lookup(uint64_t vpn, uint64_t offset, char rw, stats_t *stats)
{	

	stats -> translation_faults++;
	uint64_t pfn;
	if (current_pagetable[vpn].valid) {
		if(rw =='w') {
			current_pagetable[vpn].dirty = 1;
		}
		pfn = current_pagetable[vpn].pfn;
		current_pagetable[vpn].frequency++;
	}
	else {
		pfn = page_fault_handler(vpn, rw, stats);
		stats -> page_faults++;
		if(rw =='w') {
			current_pagetable[vpn].dirty = 1;
		}
		current_pagetable[vpn].frequency++;
		current_pagetable[vpn].valid =1;

	}

	// (1) Check the "current_pagetable" at the param VPN to find the mapping
	// (2) If the mapping does not exist, call the "page_fault_handler" function
	// (3) Make sure to increment stats
	// (4) Make sure to mark the entry dirty if access is a write
	// (5) Make sure to increment the frequency count of the VPN that has been accessed

	/********* TODO ************/

	return pfn;
}