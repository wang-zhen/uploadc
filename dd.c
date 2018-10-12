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

#define BUFERSIZE 131072
pthread_mutex_t sp_mutex = PTHREAD_MUTEX_INITIALIZER;

char *
devname_to_ssdname(const char *devname)
{
        int i,len=0;
        char *ssd =  NULL;

        if(!devname)
                return ssd; 

        if (*devname != '/') 
            return ssd; 
        if ((devname[5] != 'f') || (devname[6] != 'i') || (devname[7] != 'o'))
            return ssd; 

        len = strlen(devname); 
		if(len < strlen("/dev/fioas0"))
			return ssd;

        ssd = malloc(len*sizeof(char));
        if(!ssd)
                return ssd; 

        memset(ssd, '\0', len);
        for(i=0; i < len; i++){
                if (*(devname+i) == 's' && *(devname+i+1) != 's'){
                        *(ssd + i) = '0'; 
                        *(ssd + i + 1) = '\0'; 
                        break;
                }    
                *(ssd+i) = *(devname+i);
        }    

        return ssd; 
}

int main(int argc, char **argv)
{
	char *s=NULL;
	char *p="/dev/fioa0";

	s = devname_to_ssdname(p);

	printf("p=%s, s=%s\n",p,s);

	return 0;
}
