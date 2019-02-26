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


unsigned int pagesize;
unsigned int page_count = 32;

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

char page_tmp[PAGE_4K];
int use_exchange_page2;

extern void copy_page(char *vto, char *vfrom);

void exchange_page(char *from, char *to)
{
	uint64_t tmp;
	int i;
	for (i = 0; i < pagesize; i += sizeof(tmp)) {
		tmp = *((uint64_t *)(from + i));
		*((uint64_t *)(from + i)) = *((uint64_t *)(to + i));
		*((uint64_t *)(to + i)) = tmp;
	}
}

void exchange_page2(char *from, char *to)
{
	int i;
	for (i = 0; i < pagesize/PAGE_4K; i++) {
		copy_page(page_tmp, from + i * PAGE_4K);
		copy_page(from + i * PAGE_4K, to + i * PAGE_4K);
		copy_page(to + i * PAGE_4K, page_tmp);
	}
}

double get_us()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec  + (double) tp.tv_usec *1.e-6);
}

char *get_pages_at(unsigned long nodemask, int filled_val)
{
	char *pages;
	int i;

	pages = aligned_alloc(PAGE_2M, pagesize * page_count);
	if (!pages) {
		printf("Unable to allocate memory\n");
		exit(1);
	}

	/* Use THPs to reduce TLB overhead */
	madvise(pages, pagesize * page_count, MADV_HUGEPAGE);
	mbind(pages, pagesize * page_count, MPOL_BIND, &nodemask, sizeof(nodemask)*8, 0);

	for (i = 0; i < page_count; i++)
		pages[i * pagesize] = (char)(i + filled_val);

	return pages;
}

int main(int argc, char **argv)
{
	int i, rc;
	double begin = 0, end = 0;
	unsigned long nodemask_src = 1<<SRC_NODE;
	unsigned long nodemask_dst = 1<<DST_NODE;
	char *pages_src;
	char *pages_dst;
	int errors;
	int nr_nodes;

	pagesize = THP_PAGE_SIZE;

	nr_nodes = numa_max_node()+1;

	if (nr_nodes < 2) {
		printf("A minimum of 2 nodes is required for this test.\n");
		exit(1);
	}

	setbuf(stdout, NULL);
	if (argc > 1)
		sscanf(argv[1], "%d", &page_count);
	if (argc > 2)
		use_exchange_page2 = 1;

	pages_src = get_pages_at(nodemask_src, 0);
	pages_dst = get_pages_at(nodemask_dst, 1);

	begin = get_us();

	/* Exchange pages */
	for (i = 0; i < page_count; i++)
		if (!use_exchange_page2)
			exchange_page(pages_src + i * pagesize, pages_dst + i * pagesize);
		else
			exchange_page2(pages_src + i * pagesize, pages_dst + i * pagesize);

	end = get_us();

	printf("Total time: %f us\n", (end-begin)*1000000);

	for (i = 0; i < page_count; i++) {
		if (pages_src[ i* pagesize ] != (char)(i + 1) &&
			pages_dst[ i* pagesize ] != (char)i) {
			fprintf(stderr, "*** Page %d contents corrupted.\n", i);
			errors++;
		}
	}

	if (!errors)
		printf("Test successful.\n");
	else
		fprintf(stderr, "%d errors.\n", errors);

	return errors > 0 ? 1 : 0;
}
