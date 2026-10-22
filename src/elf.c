
/*
 * eyaX Kernel - ELF64 Loader Foundation
 *
 * Architecture: x86-64
 * Supported format: ELF64, little-endian, ET_EXEC
 *
 * This file validates ELF images and inspects PT_LOAD segments.
 * Actual memory allocation, page mapping, segment copying, and
 * user-mode execution must be implemented using eyaX's own
 * memory manager and paging interfaces.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================
 * ELF64 ABI types
 * ============================================================ */

typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;
typedef uint64_t Elf64_Off;
typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Section;
typedef uint16_t Elf64_Versym;

/* ============================================================
 * ELF identification constants
 * ============================================================ */

#define ELF_NIDENT 16

#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6

#define ELFMAG0     0x7F
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

#define ELFCLASS64  2
#define ELFDATA2LSB 1

#define ET_NONE     0
#define ET_REL      1
#define ET_EXEC     2

#define EM_X86_64   62
#define EV_CURRENT  1

/* ============================================================
 * ELF64 file header
 * ============================================================ */

typedef struct __attribute__((packed)) {
    uint8_t     e_ident[ELF_NIDENT];
    Elf64_Half  e_type;
    Elf64_Half  e_machine;
    Elf64_Word  e_version;
    Elf64_Addr  e_entry;
    Elf64_Off   e_phoff;
    Elf64_Off   e_shoff;
    Elf64_Word  e_flags;
    Elf64_Half  e_ehsize;
    Elf64_Half  e_phentsize;
    Elf64_Half  e_phnum;
    Elf64_Half  e_shentsize;
    Elf64_Half  e_shnum;
    Elf64_Half  e_shstrndx;
} Elf64_Ehdr;

_Static_assert(sizeof(Elf64_Ehdr) == 64,
               "Invalid ELF64 header size");

/* ============================================================
 * ELF64 program header
 * ============================================================ */

typedef struct __attribute__((packed)) {
    Elf64_Word  p_type;
    Elf64_Word  p_flags;
    Elf64_Off   p_offset;
    Elf64_Addr  p_vaddr;
    Elf64_Addr  p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_Phdr;

_Static_assert(sizeof(Elf64_Phdr) == 56,
               "Invalid ELF64 program header size");

/* ============================================================
 * Program header constants
 * ============================================================ */

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_PHDR    6
#define PT_TLS     7

#define PF_X       1
#define PF_W       2
#define PF_R       4

/* ============================================================
 * Internal range-validation helpers
 * ============================================================ */

/*
 * Checks whether [offset, offset + length) lies inside a buffer.
 *
 * Subtraction is used to avoid overflow in offset + length.
 */
static bool elf_range_valid(
    uint64_t offset,
    uint64_t length,
    size_t buffer_size)
{
    if (offset > (uint64_t)buffer_size)
        return false;

    if (length > (uint64_t)buffer_size - offset)
        return false;

    return true;
}

/*
 * Checks whether adding two unsigned 64-bit values would overflow.
 */
static bool elf_add_overflow(
    uint64_t a,
    uint64_t b)
{
    return b > UINT64_MAX - a;
}

/*
 * A nonzero ELF alignment must be a power of two.
 */
static bool elf_valid_alignment(uint64_t alignment)
{
    if (alignment <= 1)
        return true;

    return (alignment & (alignment - 1)) == 0;
}

/* ============================================================
 * Basic ELF identification
 * ============================================================ */

/*
 * Checks the ELF magic bytes.
 *
 * The buffer must contain at least four bytes.
 */
bool elf_check_file(
    const void *image,
    size_t image_size)
{
    if (image == NULL || image_size < 4)
        return false;

    const uint8_t *bytes = (const uint8_t *)image;

    return bytes[EI_MAG0] == ELFMAG0 &&
           bytes[EI_MAG1] == ELFMAG1 &&
           bytes[EI_MAG2] == ELFMAG2 &&
           bytes[EI_MAG3] == ELFMAG3;
}

/* ============================================================
 * ELF header validation
 * ============================================================ */

/*
 * Validates the ELF header and program-header table boundaries.
 *
 * This does not yet validate every loadable segment.
 */
bool elf_check_supported(
    const void *image,
    size_t image_size)
{
    if (!elf_check_file(image, image_size))
        return false;

    if (image_size < sizeof(Elf64_Ehdr))
        return false;

    const Elf64_Ehdr *hdr =
        (const Elf64_Ehdr *)image;

    /* Require 64-bit, little-endian ELF. */
    if (hdr->e_ident[EI_CLASS] != ELFCLASS64)
        return false;

    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB)
        return false;

    if (hdr->e_ident[EI_VERSION] != EV_CURRENT)
        return false;

    /* Require current ELF version and x86-64. */
    if (hdr->e_version != EV_CURRENT)
        return false;

    if (hdr->e_machine != EM_X86_64)
        return false;

    /* Initial loader supports fixed-address executables only. */
    if (hdr->e_type != ET_EXEC)
        return false;

    if (hdr->e_ehsize != sizeof(Elf64_Ehdr))
        return false;

    /* Require a usable program-header table. */
    if (hdr->e_phnum == 0)
        return false;

    if (hdr->e_phentsize != sizeof(Elf64_Phdr))
        return false;

    /*
     * Validate the complete table before accessing any entry.
     */
    uint64_t table_size =
        (uint64_t)hdr->e_phnum * sizeof(Elf64_Phdr);

    if (!elf_range_valid(
            hdr->e_phoff,
            table_size,
            image_size))
        return false;

    return true;
}

/* ============================================================
 * Program-header access
 * ============================================================ */

/*
 * Returns a program header by index.
 *
 * Returns NULL if the image, table, or index is invalid.
 * The returned pointer refers to data inside the original image.
 */
const Elf64_Phdr *elf_program_header(
    const void *image,
    size_t image_size,
    uint16_t index)
{
    if (!elf_check_supported(image, image_size))
        return NULL;

    const Elf64_Ehdr *hdr =
        (const Elf64_Ehdr *)image;

    if (index >= hdr->e_phnum)
        return NULL;

    const uint8_t *bytes =
        (const uint8_t *)image;

    uint64_t offset =
        hdr->e_phoff +
        (uint64_t)index * sizeof(Elf64_Phdr);

    if (!elf_range_valid(
            offset,
            sizeof(Elf64_Phdr),
            image_size))
        return NULL;

    return (const Elf64_Phdr *)(bytes + offset);
}

/* ============================================================
 * Loadable-segment validation
 * ============================================================ */

/*
 * Validates a single PT_LOAD segment.
 *
 * Checks:
 *   - File-backed data stays inside the ELF image.
 *   - File size does not exceed memory size.
 *   - Virtual-address range does not overflow.
 *   - Alignment is valid.
 *   - File and virtual offsets obey ELF alignment rules.
 */
bool elf_check_load_segment(
    const void *image,
    size_t image_size,
    const Elf64_Phdr *phdr)
{
    if (image == NULL || phdr == NULL)
        return false;

    if (phdr->p_type != PT_LOAD)
        return false;

    if (phdr->p_filesz > phdr->p_memsz)
        return false;

    if (!elf_range_valid(
            phdr->p_offset,
            phdr->p_filesz,
            image_size))
        return false;

    if (elf_add_overflow(phdr->p_vaddr, phdr->p_memsz))
        return false;

    if (!elf_valid_alignment(phdr->p_align))
        return false;

    /*
     * For alignment greater than one, the file offset and
     * virtual address must be congruent modulo p_align.
     */
    if (phdr->p_align > 1) {
        if ((phdr->p_vaddr % phdr->p_align) !=
            (phdr->p_offset % phdr->p_align))
            return false;
    }

    return true;
}

/* ============================================================
 * Complete executable-image validation
 * ============================================================ */

/*
 * Validates the header, every PT_LOAD segment, and the entry point.
 *
 * Returns true only if:
 *   - The image is a supported ELF64 executable.
 *   - Every PT_LOAD segment passes validation.
 *   - At least one PT_LOAD segment exists.
 *   - The entry point lies inside an executable PT_LOAD segment.
 *
 * This function does not map or execute the image.
 */
bool elf_validate_image(
    const void *image,
    size_t image_size)
{
    if (!elf_check_supported(image, image_size))
        return false;

    const Elf64_Ehdr *hdr =
        (const Elf64_Ehdr *)image;

    bool found_load_segment = false;
    bool entry_is_executable = false;

    for (uint16_t i = 0; i < hdr->e_phnum; ++i) {
        const Elf64_Phdr *phdr =
            elf_program_header(image, image_size, i);

        if (phdr == NULL)
            return false;

        if (phdr->p_type != PT_LOAD)
            continue;

        if (!elf_check_load_segment(
                image,
                image_size,
                phdr))
            return false;

        found_load_segment = true;

        /*
         * The entry point must be inside the actual memory
         * extent of an executable segment, not merely its
         * file-backed portion.
         */
        if ((phdr->p_flags & PF_X) != 0 &&
            phdr->p_memsz != 0 &&
            hdr->e_entry >= phdr->p_vaddr &&
            hdr->e_entry - phdr->p_vaddr < phdr->p_memsz) {
            entry_is_executable = true;
        }
    }

    if (!found_load_segment)
        return false;

    if (!entry_is_executable)
        return false;

    return true;
}

static bool elf_initialized = false;

bool elf_init(void) {
    elf_initialized = true;
    return true;
}

bool elf_is_initialized(void) {
    return elf_initialized;
}

/* ============================================================
 * End of ELF loader foundation
 * ============================================================ */
