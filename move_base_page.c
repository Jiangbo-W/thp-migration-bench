/*
 * Test program to test the moving of a processes pages.
 *
 * From:
 * http://numactl.sourcearchive.com/documentation/2.0.2/migrate__pages_8c-source.html
 *
 * (C) 2006 Silicon Graphics, Inc.
 *          Christoph Lameter <clameter@sgi.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include "numa.h"
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <numaif.h>
#include <sys/time.h>


unsigned long pagesize;
unsigned long page_count = 32;

char *page_base;
char *pages;

void **addr;
int *status;
int *nodes;
unsigned long errors;
int nr_nodes;

#define PAGE_4K (4UL*1024)
#define PAGE_2M (PAGE_4K*512)

#define PAGE_64K (64UL*1024)
#define PAGE_16M (PAGE_64K*256)

#ifdef ARCH_PPC64
#define BASE_PAGE_SIZE PAGE_64K
#define THP_PAGE_SIZE  PAGE_16M
#else
#define BASE_PAGE_SIZE PAGE_4K
#define THP_PAGE_SIZE  PAGE_2M
#endif

#define PRESENT_MASK (1UL<<63)
#define SWAPPED_MASK (1UL<<62)
#define PAGE_TYPE_MASK (1UL<<61)
#define PFN_MASK     ((1UL<<55)-1)

#define KPF_THP      (1UL<<22)

#define SRC_NODE 0
#define DST_NODE 1

double get_us()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec  + (double) tp.tv_usec *1.e-6);
}

void print_paddr_and_flags(char *bigmem, int pagemap_file, int kpageflags_file)
{
	uint64_t paddr;
	uint64_t page_flags;

	if (pagemap_file) {
		pread(pagemap_file, &paddr, sizeof(paddr), ((long)bigmem>>12)*sizeof(paddr));


		if (kpageflags_file) {
			pread(kpageflags_file, &page_flags, sizeof(page_flags), 
				(paddr & PFN_MASK)*sizeof(page_flags));

			fprintf(stderr, "vpn: 0x%lx, pfn: 0x%lx is %s %s, %s, %s\n",
				((long)bigmem)>>12,
				(paddr & PFN_MASK),
				paddr & PAGE_TYPE_MASK ? "file-page" : "anon",
				paddr & PRESENT_MASK ? "there": "not there",
				paddr & SWAPPED_MASK ? "swapped": "not swapped",
				page_flags & KPF_THP ? "thp" : "not thp"
				/*page_flags*/
				);

		}
	}



}


int main(int argc, char **argv)
{
	unsigned long i, rc;
	double begin = 0, end = 0;
	const char *pagemap_template = "/proc/%d/pagemap";
	const char *kpageflags_proc = "/proc/kpageflags";
	char move_pages_stats_proc[255];
	char pagemap_proc[255];
	char stats_buffer[1024] = {0};
	int pagemap_fd;
	int kpageflags_fd;
	//unsigned long nodemask = 1<<SRC_NODE;
	unsigned long src_nodemask;
	unsigned long src_node, dst_node;

	pagesize = BASE_PAGE_SIZE;

	nr_nodes = numa_max_node()+1;
	printf("nr_nodes: %d\n", nr_nodes);

	if (nr_nodes < 2) {
		printf("A minimum of 2 nodes is required for this test.\n");
		exit(1);
	}

	setbuf(stdout, NULL);
	if (argc != 4) {
		printf("%s [src_node] [dst_node] [count]\n", argv[0]);
		exit(1);
	}
	sscanf(argv[1], "%lu", &src_node);
	if (src_nodemask < 0 || src_node > nr_nodes) {
		printf("Invalid src node: %d\n", src_node);
		exit(1);
	}
	sscanf(argv[2], "%lu", &dst_node);
	if (dst_node < 0 || dst_node >= nr_nodes) {
		printf("Invalid dst node: %d\n", dst_node);
		exit(1);
	}
	sscanf(argv[3], "%d", &page_count);
	if (page_count <= 0) {
		printf("Invalid count: %d\n", page_count);
		exit(1);
	}
	src_nodemask = 1 << src_node;
	printf("%lu %lu page count: %lu\n", src_node, dst_node, page_count);

	printf("migrate_pages() test ......\n");

	page_base = aligned_alloc(pagesize, pagesize*page_count);
	addr = malloc(sizeof(char *) * page_count);
	status = malloc(sizeof(int *) * page_count);
	nodes = malloc(sizeof(int *) * page_count);
	if (!page_base || !addr || !status || !nodes) {
		printf("Unable to allocate memory\n");
		exit(1);
	}

	madvise(page_base, pagesize*page_count, MADV_NOHUGEPAGE);
	mbind(page_base, pagesize*page_count, MPOL_BIND, &src_nodemask,
					sizeof(src_nodemask)*8, 0);

	pages = page_base;

	for (i = 0; i < page_count; i++) {
		pages[ i * pagesize] = (char) i;
		addr[i] = pages + i * pagesize;
		nodes[i] = dst_node;
		status[i] = -123;
	}

	sprintf(pagemap_proc, pagemap_template, getpid());
	pagemap_fd = open(pagemap_proc, O_RDONLY);

	if (pagemap_fd == -1)
	{
		perror("read pagemap:");
		exit(-1);
	}

	kpageflags_fd = open(kpageflags_proc, O_RDONLY);

	if (kpageflags_fd == -1)
	{
		perror("read kpageflags:");
		exit(-1);
	}

	/*
	for (i = 0; i < page_count; ++i) {
		print_paddr_and_flags(pages+pagesize*i, pagemap_fd, kpageflags_fd);
	}
	*/

	begin = get_us();

	/* Move to starting node */
	rc = numa_move_pages(0, page_count, addr, nodes, status, 0);

	if (rc < 0 && errno != ENOENT) {
		perror("move_pages");
		exit(1);
	}

	end = get_us();

	printf("+++++After moved to node %d+++++\n", dst_node);
	/*
	for (i = 0; i < page_count; ++i) {
		print_paddr_and_flags(pages+pagesize*i, pagemap_fd, kpageflags_fd);
	}
	*/

	printf("Total time: %f us\n", (end-begin)*1000000);

	/* Get page state after migration */
	numa_move_pages(0, page_count, addr, NULL, status, 0);
	for (i = 0; i < page_count; i++) {
		if (pages[ i* pagesize ] != (char) i) {
			fprintf(stderr, "*** Page contents corrupted.\n");
			errors++;
		} else if (status[i] != dst_node) {
			fprintf(stderr, "*** Page on the wrong node\n");
			errors++;
		}
	}

	if (!errors)
		printf("Test successful.\n");
	else
		fprintf(stderr, "%d errors.\n", errors);

	close(pagemap_fd);
	close(kpageflags_fd);

	return errors > 0 ? 1 : 0;
}
