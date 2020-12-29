rm -rf client server
gcc sv.c -lpthread -o server
gcc cl.c -o client
./server
