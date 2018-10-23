#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <assert.h>
#include <pthread.h>
#include <sys/ioctl.h>

#define BUFERSIZE 4096
#define MEMALIGN 4096

ssize_t ssdpread(int fd, int bfd, void *buf, size_t count, off_t offset)
{
    int blocksize, index, readsize;
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
    readsize = blocksize - boffset;

    if((offset + count) > flsize)
        count = flsize - offset;

    printf("blocksize:%d flsize: %ld count%ld,offset:%ld\n",blocksize, flsize, count, offset);
    while(count > 0){
        block = index;
        if (ioctl(fd, FIBMAP, &block)) {
            perror("FIBMAP ioctl failed");
            goto error;
        }   

        if(count < readsize)
            readsize = count;

        printf("wangzhe index:%d  block:%ld\n",index,block);
        n = pread(bfd,buf,readsize,(off_t)(block*blocksize + boffset));
        if(n < 0){
            goto error;
        }
        if(boffset){
            boffset = 0;
            readsize = blocksize;
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
    int sfd, dfd, bfd, rn, ret,seek = 0;
    off_t flsize,truesize;
    struct stat st; 
    unsigned char *buf = NULL;


    assert(argv[1] != NULL);
    assert(argv[2] != NULL);
    assert(argv[3] != NULL);

    ret = posix_memalign((void **)&buf, MEMALIGN, BUFERSIZE);
    if (ret) {
        perror("posix_memalign failed");
        exit(1);
    }   

    sfd = open(argv[1], O_RDWR | O_DIRECT | O_SYNC);
    if (sfd <= 0) {
        perror("error opening file");
        goto end;
    }
    syncfs(sfd);

    dfd = open(argv[2], O_RDWR | O_CREAT, 0644);
    if (dfd <= 0) {
        perror("error opening file");
        goto end;
    }

    bfd = open(argv[3], O_RDWR | O_DIRECT | O_SYNC);
    if (bfd <= 0) {
        perror("error opening file");
        goto end;
    }
    syncfs(bfd);

    if (fstat(sfd, &st)) {
        perror("fstat error");
        goto end;
    }   

    truesize = st.st_size;
    flsize = truesize;
    if(truesize % MEMALIGN)
        flsize = (truesize & ~(MEMALIGN-1)) + MEMALIGN;

    printf("file:%s  truesize:%ld,flsize:%ld\n",argv[1],truesize,flsize);
    if (flsize != truesize){
        if(ftruncate(sfd,flsize) < 0){
            perror("ftruncate error");
            exit(-1);
        }
    }

    while(1){
        rn=ssdpread(sfd, bfd, buf, BUFERSIZE, seek);
        //rn=pread(sfd, buf, BUFERSIZE, seek);
        printf("main seek:%d,readrn:%d\n",seek,rn);
        if(rn < 0){
            perror("read error");
            break;
        }
        if(rn == 0){
            perror("read end");
            break;
        }
	if(pwrite(dfd, buf, rn, seek) != rn ){
	    perror("pwrite error");
            break;
         }

	 seek += rn;
	 memset(buf, 0, BUFERSIZE);
    }

    if (flsize != truesize){
        if(ftruncate(sfd,truesize) < 0){
            perror("ftruncate error");
            exit(-1);
        }
    }

end:
    if(sfd)
        close(sfd);
    if(dfd)
        close(dfd);
    if(bfd)
        close(bfd);
    if(buf)
        free(buf);
    return 0;
}
