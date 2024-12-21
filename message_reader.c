#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h> 
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "message_slot.h"

// -----------------------------------------------------------------------------

void check_feedback(int fb, int wanted) {
    if (fb != wanted) {
        printf("%d\n", errno);
        fprintf(stderr, "message_reader: %s\n", strerror(errno));
        exit(1);
    }
}

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
    if (argc != 3) {
        errno = EINVAL;
	printf("%d", errno);
        fprintf(stderr, "message_reader: %s\n", strerror(errno));
        exit(1);
    }

    // argv has all arguments
    char *fpath, *end;
    char msg[BUF_LEN];
    unsigned int chid;
    int fdesc, feedback;

    fpath = argv[1];
    chid = (unsigned int)strtoul(argv[2], &end, 10); // Assuming a valid value

    if ((fdesc = open(fpath, O_RDWR)) < 0) {
        check_feedback(1, 0);
    }
    
    feedback = ioctl(fdesc, chid, MSG_SLOT_COMMAND);
    check_feedback(feedback, SUCCESS);
    feedback = read(fdesc, msg, BUF_LEN);
    check_feedback(feedback, BUF_LEN);
    
    close(fdesc);
    write(STDOUT_FILENO, msg, BUF_LEN);
    printf("\n");
    
    return 0;
}
