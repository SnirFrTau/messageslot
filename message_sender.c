#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h> 

#include "message_slot.h"

// -----------------------------------------------------------------------------

void check_feedback(int fb) {
    if (fb != 0) {
        fprintf(stderr, "message_sender: %s\n", strerr(errno));
        exit(1);
    }
}

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
    if (argc != 4) {
        errno = EINVAL;
        fprintf(stderr, "message_sender: %s\n", strerr(errno));
        exit(1);
    }

    // argv has all arguments
    char *fpath, *msg;
    unsigned int chid;
    int fdesc, feedback, msglen;

    *fpath = argv[1];
    chid = (unsigned int)strtoul(argv[2]); // Assuming a valid value
    *msg = argv[3];

    if ((fdesc = open(fp, O_RDWR)) < 0) {
        printerr(errno);
        exit(1);
    }
    
    feedback = ioctl(fdesc, chid, MSG_SLOT_COMMAND);
    check_feedback(feedback);
    feedback = write(fdesc, msg, strlen(msg));
    check_feedback(feedback);
    
    close(fdesc);
    return 0;
}