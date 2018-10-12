#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fs.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define BUFERSIZE 4096

ssize_t ssdpwrite(int fd, int sfd, const void *buf, size_t count, off_t offset)
{
    int blocksize, index, writesize;
    ssize_t n, block;
    off_t flsize,boffset=0;
    size_t ret = 0;
    struct stat st; 

    if (ioctl(fd, FIGETBSZ, &blocksize)) {
        perror("FIBMAP ioctl failed");
        goto error;
    }   

    if (fstat(fd, &st)) {
        perror("fstat error");
        goto error;
    }   

    flsize = st.st_size;

    index = offset/blocksize;
	boffset = offset % blocksize;
    writesize = blocksize; 

    if((offset + count) > flsize)
        count = flsize - offset;

    while(count > 0){
        block = index;
        if (ioctl(fd, FIBMAP, &block)) {
            perror("FIBMAP ioctl failed");
            goto error;
        }   

        if(count < writesize)
            writesize = count;

		printf("writesize:%d block:%ld\n", writesize, block);
        n = pwrite(sfd,buf,writesize,(off_t)(block*blocksize + offset));
        if(n < 0){
            perror("pread err");
            goto error;
        }
		if(boffset){
			boffset = 0;
			writesize = blocksize;
		}
        count -= n;
        ret += n;
        index++;
        buf += n;
    }

    return ret; 
error:
    return -1; 
}


int main(int argc, char **argv)
{
    int sfd, dfd, bfd, n, offset = 0;
    char buf[BUFERSIZE] = {0,};

    assert(argv[1] != NULL);
    assert(argv[2] != NULL);
    assert(argv[3] != NULL);

    sfd = open(argv[1], O_RDONLY);
    if (sfd <= 0) {
        perror("error opening file");
        goto end;
    }

    dfd = open(argv[2], O_RDWR);
    if (dfd <= 0) {
        perror("error opening file");
        goto end;
    }
    bfd = open(argv[3], O_RDWR);
    if (bfd <= 0) {
        perror("error opening file");
        goto end;
    }

	while(1){
    	n=pread(sfd, buf, BUFERSIZE, offset);
		if(n <= 0){
			printf("n=%d\n",n);
        	perror("END");
			break;
		}

    	n=ssdpwrite(dfd, bfd, buf, n, offset);

		offset += n;
		memset(buf, '\0', BUFERSIZE);
	}

end:
	if(sfd)
    	close(sfd);
	if(dfd)
    	close(dfd);
	if(bfd)
    	close(bfd);
    return 0;
}
