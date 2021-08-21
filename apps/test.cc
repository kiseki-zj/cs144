#include "socket.hh"
#include "util.hh"
#include <cstdlib>
#include <iostream>
#include "tcp_sponge_socket.hh"
int main() {
    CS144TCPSocket fd;
    Address addr{"www.baidu.com"};
    fd.connect(addr);
    fd.write("123");

}