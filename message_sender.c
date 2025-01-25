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
        fprintf(stderr, "message_sender: %s\n", strerror(errno));
        exit(1);
    }
}

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
    if (argc != 4) {
        errno = EINVAL;
        fprintf(stderr, "message_sender: %s\n", strerror(errno));
        exit(1);
    }

    // argv has all arguments
    char *fpath, *msg, *end;
    unsigned int chid;
    int fdesc, feedback;

    fpath = argv[1];
    // Assuming a valid value
    // Note however that due to a hardcoded conflict in chid=2 I offset by 3
    chid = (unsigned int)strtoul(argv[2], &end, 10);
    msg = argv[3];

    if ((fdesc = open(fpath, O_RDWR)) < 0) {
        check_feedback(1, 0);
    }
    
    feedback = ioctl(fdesc, MSG_SLOT_CHANNEL, chid);
    check_feedback(feedback, SUCCESS);
    feedback = write(fdesc, msg, strlen(msg));
    check_feedback(feedback, strlen(msg));
    
    close(fdesc);
    return 0;
}
