
#ifndef EYAX_ELF_H
#define EYAX_ELF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ELF initialization */
bool elf_init(void);
bool elf_is_initialized(void);

/* ELF validation */
bool elf_check_file(const void *image, size_t image_size);
bool elf_check_supported(const void *image, size_t image_size);
bool elf_validate_image(const void *image, size_t image_size);

#endif
