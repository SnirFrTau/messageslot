#ifndef MSGSLOT_H
#define MSGSLOT_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define MAJOR_NUM 235
#define MINOR_COUNT 256
#define SUCCESS 0
#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_slot"

// Generate command number to be unsigned long, since this is what ioctl receives
#define MSG_SLOT_COMMAND _IOWR(MAJOR_NUM, 0, unsigned long)

#endif
