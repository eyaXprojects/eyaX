#ifndef EYAX_PMM_H
#define EYAX_PMM_H

#include <stdbool.h>
#include <stdint.h>

#define PMM_PAGE_SIZE 4096

bool pmm_init(void);
uint64_t pmm_alloc_page(void);
bool pmm_free_page(uint64_t address);
uint64_t pmm_free_pages(void);

#endif
