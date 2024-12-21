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
    chid = (unsigned int)strtoul(argv[2], &end, 10); // Assuming a valid value
    msg = argv[3];

    if ((fdesc = open(fpath, O_RDWR)) < 0) {
        check_feedback(1, 0);
    }

    printf("About to connect to ioctl\n");
    feedback = ioctl(fdesc, chid, MSG_SLOT_COMMAND);
    check_feedback(feedback, SUCCESS);
    printf("Sening message\n");
    feedback = write(fdesc, msg, strlen(msg));
    check_feedback(feedback, strlen(msg));
    printf("Sent message\n");
    
    close(fdesc);
    return 0;
}
