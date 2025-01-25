#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <wait.h>

#include "message_slot.h"


void check_feedback(int fb, int wanted) {
    if (fb != wanted) {
        fprintf(stderr, "message_sender: %s\n", strerror(errno));
        exit(1);
    }
}


int main (int argc, char **argv) {
    int pid, status;
    char *fpath, *msg, *end;
    unsigned int chid;
    int fdesc, feedback;
    char msgbuf[BUF_LEN];

    fpath = "/dev/slot1";
    
    pid = fork();
    if (!pid) { // Child
	if ((fdesc = open(fpath, O_RDWR)) < 0) {
	    check_feedback(1, 0);
	}
	
	chid = 3;
	msg = "Heyo";
    
	feedback = ioctl(fdesc, MSG_SLOT_CHANNEL, chid);
	check_feedback(feedback, SUCCESS);
	feedback = write(fdesc, msg, strlen(msg));
	check_feedback(feedback, strlen(msg));

	printf("Cool!\n");

	close(fdesc);

	return 0;
    }
    else { // Parent
	
	waitpid(pid, &status, 0);
	if (WIFEXITED(status) && WEXITSTATUS(status)) {
	    printf("Error\n");
	}
	else {
	    printf("Exited Normally\n");
	}
	printf("Ok here we go\n");

	if ((fdesc = open(fpath, O_RDWR)) < 0) {
	    check_feedback(1, 0);
	}
	
	chid = 4;
	msg = "Wazzap";
    
	feedback = ioctl(fdesc, MSG_SLOT_CHANNEL, chid);
	check_feedback(feedback, SUCCESS);
	feedback = write(fdesc, msg, strlen(msg));
	check_feedback(feedback, strlen(msg));

	printf("Written to %d\n", chid);
	feedback = read(fdesc, msgbuf, BUF_LEN);
	if (feedback <= 0) {
	    errno = EINVAL;
	    fprintf(stderr, "message_reader: %s\n", strerror(errno));
	    exit(1);
	}
    
	write(STDOUT_FILENO, msg, feedback);

	chid = 5;
	printf("\nBack to %d\n", chid);
	
	memset(msgbuf, 0, BUF_LEN);
	printf("Preparing to ioctl %d!\n", chid);
	feedback = ioctl(fdesc, MSG_SLOT_CHANNEL, chid);

	
	feedback = read(fdesc, msgbuf, BUF_LEN);
	close(fdesc);
	if (feedback <= 0) {
	    errno = EINVAL;
	    fprintf(stderr, "message_reader: %s\n", strerror(errno));
	    exit(1);
	}
    
	write(STDOUT_FILENO, msgbuf, feedback);

	return 0;
    }
}
