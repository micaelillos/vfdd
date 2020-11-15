/*
 * mutil.c -  memory map functions
 * 
 * include LICENSE
 */ 
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <msglog.h>
#include <mutil.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 * open and memory map a file for read or write
 */
char *mu_mmap_open(const char *fname, int mode, size_t * psize, off_t offset )
{
   char *addr = MAP_FAILED;        /* mmap address      */
   int fd = -1;
   struct stat stbuf;
   int ret;
   int openFlags = mode;
   int mmapFlags;

   mode &= 3;
   ret = stat(fname, &stbuf);
   /* if file exist, file size is set to  *psize, if not 0 */
   if ( ret == 0 ){
      if ( psize && *psize == 0 ){
         *psize = stbuf.st_size;
      }
   } else {
       stbuf.st_size = 0;
   }
   if ( mode == O_RDONLY ){
      mmapFlags = PROT_READ;
   } else if ( mode == O_WRONLY ){
      openFlags |= O_CREAT | O_TRUNC;
      mmapFlags = PROT_READ | PROT_WRITE;
   } else { /* O_RDWR */
      openFlags |=  O_CREAT;
      mmapFlags = PROT_READ | PROT_WRITE;
   }

   if ( (fd = open(fname, openFlags, 0644  )) < 0) {
      msg_errorl(3, "Unable to open output file '%s' - %s", fname,
               strerror(errno) );
      return MAP_FAILED;
   }
   if ( mode != O_RDONLY ){
      if ( ! psize || *psize == 0){
         msg_errorl(2, "File '%s' size not specified", fname );
         goto finish;
      }
      if ( openFlags & O_TRUNC ){ /* this is to force lseek+write */
          stbuf.st_size = 0;
      }
      /* Something needs to be written at the end of the file */
      /* only if requested size greater than current size     */
      if ( *psize  > (size_t) stbuf.st_size ){
         lseek(fd, *psize - 1, SEEK_SET);
         write(fd, "", 1);
      }
   } else if ( *psize == 0 ) {  /* rad_only, size = 0 */ 
      msg_errorl(2, "Mmap will fail for read_only 0 sized file '%s'", fname );
      goto finish;
   }
      
   addr = mmap( NULL, *psize, mmapFlags, MAP_SHARED, fd, offset);
   if ( addr == MAP_FAILED) {
      msg_errorl(2, "Can't map file '%s': %s", fname, strerror (errno));
   }
finish:
   close(fd);
   return addr;
}

void mu_mmap_close(void *addr, size_t size)
{
   munmap( addr, size);
}
