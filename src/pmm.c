
#include "pmm.h"
#include "limine.h"

#include <stddef.h>
#include <stdint.h>

#define PMM_MAX_PHYS_ADDR (64ULL * 1024 * 1024 * 1024)
#define PMM_MAX_PAGES     (PMM_MAX_PHYS_ADDR / PMM_PAGE_SIZE)
#define PMM_BITMAP_SIZE   ((PMM_MAX_PAGES + 7) / 8)

extern volatile struct limine_memmap_request eX_MemoryMapRequest;

/*
 * Supports physical addresses below 64 GiB.
 * The bitmaps live in the kernel's BSS.
 */
static uint8_t pmm_bitmap[PMM_BITMAP_SIZE];
static uint8_t pmm_usable_bitmap[PMM_BITMAP_SIZE];

static uint64_t pmm_available_pages;
static bool pmm_initialized;


static bool bitmap_get(const uint8_t *bitmap, uint64_t index)
{
    return (bitmap[index / 8] & (uint8_t)(1u << (index % 8))) != 0;
}

static void bitmap_set(uint8_t *bitmap, uint64_t index)
{
    bitmap[index / 8] |= (uint8_t)(1u << (index % 8));
}

static void bitmap_clear(uint8_t *bitmap, uint64_t index)
{
    bitmap[index / 8] &= (uint8_t)~(1u << (index % 8));
}

bool pmm_init(void)
{
    volatile struct limine_memmap_response *response =
    eX_MemoryMapRequest.response;

    if (response == NULL || response->entries == NULL)
        return false;

    /* Treat every frame as reserved until proven usable. */
    for (uint64_t i = 0; i < PMM_BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFF;
        pmm_usable_bitmap[i] = 0;
    }

    pmm_available_pages = 0;
    pmm_initialized = false;

    for (uint64_t i = 0; i < response->entry_count; i++) {
        struct limine_memmap_entry *entry = response->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        /* Prevent base + length from overflowing. */
        if (entry->base >= PMM_MAX_PHYS_ADDR)
            continue;

        uint64_t end;

        if (entry->length > PMM_MAX_PHYS_ADDR - entry->base)
            end = PMM_MAX_PHYS_ADDR;
        else
            end = entry->base + entry->length;

        /* Only expose complete, aligned 4 KiB frames. */
        uint64_t first_page =
            (entry->base + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
        uint64_t last_page = end / PMM_PAGE_SIZE;

        if (first_page >= PMM_MAX_PAGES)
            continue;

        if (last_page > PMM_MAX_PAGES)
            last_page = PMM_MAX_PAGES;

        for (uint64_t page = first_page; page < last_page; page++) {
            /*
             * Do not make page zero allocatable.
             * The usable bitmap lets free_page distinguish usable
             * frames from reserved memory.
             */
            bitmap_set(pmm_usable_bitmap, page);

            if (page == 0)
                continue;

            if (!bitmap_get(pmm_bitmap, i)) {
                bitmap_clear(pmm_bitmap, page);
                pmm_available_pages++;
            }
        }
    }

    pmm_initialized = true;
    return true;
}

uint64_t pmm_alloc_page(void)
{
    if (!pmm_initialized)
        return 0;

    for (uint64_t i = 1; i < PMM_MAX_PAGES; i++) {
        if (!bitmap_get(pmm_usable_bitmap, i))
            continue;

        if (bitmap_get(pmm_bitmap, i)) {
            bitmap_set(pmm_bitmap, i);
            pmm_available_pages--;

            return i * PMM_PAGE_SIZE;
        }
    }

    /* Zero means allocation failure. */
    return 0;
}

bool pmm_free_page(uint64_t address)
{
    if (!pmm_initialized || address == 0)
        return false;

    if ((address % PMM_PAGE_SIZE) != 0)
        return false;

    uint64_t page = address / PMM_PAGE_SIZE;

    if (page >= PMM_MAX_PAGES)
        return false;

    /* Never free memory that wasn't marked as usable. */
    if (!bitmap_get(pmm_usable_bitmap, page))
        return false;

    /* Reject double frees. */
    if (!bitmap_get(pmm_bitmap, page))
        return false;

    bitmap_clear(pmm_bitmap, page);
    pmm_available_pages++;

    return true;
}

uint64_t pmm_free_pages(void)
{
    if (!pmm_initialized)
        return 0;

    return pmm_available_pages;
}
