Zeynep Sıla Kaya 69101


We implemented the both parts.

In part 1, we only implemented the todo parts.Since virtual address space and physical address space were equal in part 1 there were no need for any page replacement algorithm.
In case of a page fault, basically we found the location with the logical address from the backing file and copied it to the main memory and updated the page table and TLB.

In part 2, we changed the unsigned chars to int.In this part there were a need for page replacement algorithm to find a victim page.In fifo, we used a logic similar to adding to TLB. In LRU, we used an array to see all page references in the LRU and in case of a page fault we find the least recently used one from that array.

The program can compiled with the makefile.

To run the first part of the program:
./virtmem BACKING_STORE.bin addresses.txt

To run the second part of the program with the FIFO
./virtmem_part2 BACKING_STORE.bin addresses.txt -p 0
To run the second part of the program with the LRU
./virtmem_part2 BACKING_STORE.bin addresses.txt -p 1
