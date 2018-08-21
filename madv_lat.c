#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

unsigned long long rdtsc(void)
{
	unsigned lo, hi;

	__asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));

	return (unsigned long long)hi << 32 | lo;
}

int main(void)
{
	char **regions;
	size_t sz_regions[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192,
		16384, 32768};
	unsigned nr_regions;
	unsigned nr_iters;
	int i, j;
	unsigned long long tsc, averaged_tsc;

	nr_iters = 100;
	nr_regions = sizeof(sz_regions) / sizeof(sz_regions[0]);

	regions = (char **)malloc(sizeof(char *) * nr_regions);
	for (i = 0; i < nr_regions; i++)
		regions[i] = (char *)malloc(sz_regions[i]);

	for (i = 0; i < nr_regions; i++) {
		averaged_tsc = 0;
		for (j = 0; j < nr_iters; j++) {
			tsc = rdtsc();
			madvise(regions[i], sz_regions[i], MADV_WILLNEED);
			averaged_tsc += rdtsc() - tsc;
		}
		printf("%zu	%llu\n", sz_regions[i],
				averaged_tsc / nr_iters);
	}

	for (i = 0; i < nr_regions; i++)
		free(regions[i]);
	free(regions);


	printf("hello\n");
}
