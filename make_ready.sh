make all
sudo insmod message_slot.ko
make clean
sudo mknod /dev/slot1 c 235 2
sudo chmod 666 /dev/slot1
gcc -O3 -Wall -std=c11 message_sender.c -o sender
gcc -O3 -Wall -std=c11 message_reader.c -o reader
