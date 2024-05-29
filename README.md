# client-server
* ### Commands to setup server/client on VM:
```bash 
sudo yum install git && git clone https://github.com/YrWaifu/client-server.git && sudo yum install gcc && sudo yum install openssl-devel && sudo yum install openssl-devel
```

* ### Guide to build

From client-server/build directory

```bash
make # - to build client and server
make rebuild # - to clear build directory and remake client and server
make test_client_send # - to build send'er
```

* ### Guide to use

From client-server/build directory

```bash
./server -p <PORT>
./client -i <IP_ADDRESS> -p <PORT

# For both next tests you need to rewrite first line in messages.txt (if you use template)
# First line in messages.txt in format <IP>:<PORT> <>NICKNAME> <CHANNEL>
# Second line will be TEST STARTED
# Next lines with messages to send fow test
./test_client_send> -f <MESSAGES.TXT> -t <SECONDS>
python3 test_client_read
```