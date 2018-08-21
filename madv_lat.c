#include <argp.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

unsigned long long rdtsc(void)
{
	unsigned lo, hi;

	__asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));

	return (unsigned long long)hi << 32 | lo;
}

#define PAGE_ALIGN(p) ((void *)(((unsigned long long)p >> 12) << 12))

static struct argp_option options[] = {
	{
		.name = "nr_iters",
		.key = 'i',
		.arg = "<nr_iters>",
		.flags = 0,
		.doc = "number of iterations for measurement",
		.group = 0,
	},
	{
		.name = "sz_min_region",
		.key = 'm',
		.arg = "<sz_min_region>",
		.flags = 0,
		.doc = "size of minimal region to measure",
		.group = 0,
	},
	{
		.name = "sz_max_region",
		.key = 'M',
		.arg = "<sz_max_region>",
		.flags = 0,
		.doc = "size of maximum region to measure",
		.group = 0,
	},

	{}
};

unsigned nr_iters = 1000;
size_t sz_min_region = 1024, sz_max_region = 4096;

error_t parse_options(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case 'i':
		nr_iters = atol(arg);
		break;
	case 'm':
		sz_min_region = atol(arg);
		break;
	case 'M':
		sz_max_region = atol(arg);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

struct region {
	char *addr;
	char *addr_aligned;
	size_t sz;
};

unsigned mkregions(size_t sz_min, size_t sz_max, struct region **ret_ptr)
{
	struct region *regions = NULL;
	struct region *r;
	size_t sz_region;
	unsigned nr_regions;

	for (nr_regions = 1, sz_region = sz_min; sz_region <= sz_max;
			nr_regions += 1, sz_region *= 2) {
		regions = realloc(regions, sizeof(struct region) * nr_regions);
		if (regions == NULL)
			err(1, "Failed realloc()\n");
		r = &regions[nr_regions - 1];
		r->sz = sz_region;
		r->addr = (char *)malloc(sz_region);
		r->addr_aligned = PAGE_ALIGN(r->addr);
	}

	*ret_ptr = regions;
	return nr_regions - 1;
}

#ifdef DBG

void pr_regions(struct region *regions, unsigned nr_regions)
{
	struct region *r;
	unsigned i;

	printf("addr	aligned	size\n");
	for (i = 0; i < nr_regions; i++) {
		r = &regions[i];
		printf("%p\t%p\t%zu\n", r->addr, r->addr_aligned, r->sz);
	}
}

#endif

int main(int argc, char *argv[])
{
	struct region *regions;
	unsigned nr_regions;
	struct argp argp = {
		.options = options,
		.parser = parse_options,
		.args_doc = "",
		.doc = "Measure madvise() latency for "
			"various size memory regions",
	};
	int i, j;
	unsigned long long tsc, averaged_tsc;

	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, NULL);

	nr_regions = mkregions(sz_min_region, sz_max_region, &regions);

#ifdef DBG
	pr_regions(regions, nr_regions);
#endif

	printf("%15s	%15s", "size", "latency (cycles)\n");
	for (i = 0; i < nr_regions; i++) {
		averaged_tsc = 0;
		for (j = 0; j < nr_iters; j++) {
			tsc = rdtsc();
			madvise(regions[i].addr_aligned, regions[i].sz,
					MADV_WILLNEED);
			averaged_tsc += rdtsc() - tsc;
		}
		printf("%15zu	%15llu\n", regions[i].sz,
				averaged_tsc / nr_iters);
	}

	for (i = 0; i < nr_regions; i++)
		free(regions[i].addr);
	free(regions);
}
