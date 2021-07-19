#include <iostream>
#include "byte_stream.hh"
#include <string>
using namespace std;

int main() {
    ByteStream bs(15);
    bs.write("cat");
    cout << bs.buffer.size() << endl;
    string res = bs.peek_output(3);
    cout << res << endl;
}