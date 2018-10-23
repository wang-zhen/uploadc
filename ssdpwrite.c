#define _GNU_SOURCE

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
#define MEMALIGN 4096

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
    writesize = blocksize - boffset; 

    if((offset + count) > flsize)
        count = flsize - offset;

    printf("blocksize:%d flsize: %ld count%ld,offset:%ld\n",blocksize, flsize, count, offset);
    while(count > 0){
        block = index;
        if (ioctl(fd, FIBMAP, &block)) {
            perror("FIBMAP ioctl failed");
            goto error;
        }   

        if(count < writesize)
            writesize = count;

        printf("wangzhe index:%d  block:%ld\n",index,block);
        n = pwrite(sfd,buf,writesize,(off_t)(block*blocksize + boffset));
        if(n < 0){
            perror("pwrite err");
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
    int sfd, dfd, bfd, ret, rn, wn, offset = 0;
    unsigned char *buf = NULL;

    assert(argv[1] != NULL);
    assert(argv[2] != NULL);
    assert(argv[3] != NULL);

    ret = posix_memalign((void **)&buf, MEMALIGN, BUFERSIZE);
    if (ret) {
        perror("posix_memalign failed");
        exit(1);
    }

    sfd = open(argv[1], O_RDWR);
    if (sfd <= 0) {
        perror("error opening file");
        goto end;
    }

    dfd = open(argv[2], O_RDWR | O_DIRECT | O_SYNC);
    if (dfd <= 0) {
        perror("error opening file");
        goto end;
    }
    syncfs(dfd);

    bfd = open(argv[3], O_RDWR | O_DIRECT | O_SYNC);
    if (bfd <= 0) {
        perror("error opening file");
        goto end;
    }
    syncfs(bfd);

    while(1){
        rn=pread(sfd, buf, BUFERSIZE, offset);
        if(rn < 0){
            perror("read error");
            break;
        }
        if(rn == 0){
            perror("read end");
            break;
        }

        wn=ssdpwrite(dfd, bfd, buf, rn, offset);
        if(wn != rn){
             perror("END");
             break;
        }

        offset += rn;
        memset(buf, '\0', BUFERSIZE);
    }

end:
	if(buf)
    	free(buf);
	if(sfd)
    	close(sfd);
	if(dfd)
    	close(dfd);
	if(bfd)
    	close(bfd);
    return 0;
}
