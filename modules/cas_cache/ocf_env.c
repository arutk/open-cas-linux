/*
* Copyright(c) 2012-2019 Intel Corporation
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "cas_cache.h"
#include "utils/utils_rpool.h"

/* *** ALLOCATOR *** */


static int env_sort_is_aligned(const void *base, int align)
{
	return IS_ENABLED(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) ||
		((unsigned long)base & (align - 1)) == 0;
}

static void env_sort_u32_swap(void *a, void *b, int size)
{
	u32 t = *(u32 *)a;
	*(u32 *)a = *(u32 *)b;
	*(u32 *)b = t;
}

static void env_sort_u64_swap(void *a, void *b, int size)
{
	u64 t = *(u64 *)a;
	*(u64 *)a = *(u64 *)b;
	*(u64 *)b = t;
}

static void env_sort_generic_swap(void *a, void *b, int size)
{
	char t;

	do {
		t = *(char *)a;
		*(char *)a++ = *(char *)b;
		*(char *)b++ = t;
	} while (--size > 0);
}

void env_sort(void *base, size_t num, size_t size,
	int (*cmp_fn)(const void *, const void *),
	void (*swap_fn)(void *, void *, int size))
{
	/* pre-scale counters for performance */
	int64_t i = (num/2 - 1) * size, n = num * size, c, r;

	if (!swap_fn) {
		if (size == 4 && env_sort_is_aligned(base, 4))
			swap_fn = env_sort_u32_swap;
		else if (size == 8 && env_sort_is_aligned(base, 8))
			swap_fn = env_sort_u64_swap;
		else
			swap_fn = env_sort_generic_swap;
	}

	/* heapify */
	for ( ; i >= 0; i -= size) {
		for (r = i; r * 2 + size < n; r  = c) {
			c = r * 2 + size;
			if (c < n - size &&
				cmp_fn(base + c, base + c + size) < 0)
				c += size;
			if (cmp_fn(base + r, base + c) >= 0)
				break;
			swap_fn(base + r, base + c, size);
		}
	}

	/* sort */
	for (i = n - size; i > 0; i -= size) {
		swap_fn(base, base + i, size);
		for (r = 0; r * 2 + size < i; r = c) {
			c = r * 2 + size;
			if (c < i - size &&
				cmp_fn(base + c, base + c + size) < 0)
				c += size;
			if (cmp_fn(base + r, base + c) >= 0)
				break;
			swap_fn(base + r, base + c, size);
		}
	}
}

env_allocator *env_allocator_create(uint32_t size, const char *name)
{
       return kmem_cache_create(name, size, 64, 0, NULL);
}

void env_allocator_destroy(env_allocator *allocator)
{
       return kmem_cache_destroy(allocator);
}
