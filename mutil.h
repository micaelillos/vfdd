#ifndef MUTIL_H
#define MUTIL_H

/*
 * mutil.h -  memory map functions
 *
 * include LICENSE
 */
#include <fcntl.h>     /* O_RDWR O_RDONLY */
#include <sys/mman.h>

/*
 * prototypes
 */
char *mu_mmap_open(const char *fname, int mode, size_t * psize, off_t offset );
void mu_mmap_close(void *addr, size_t size);

#endif /* MUTIL_H */
