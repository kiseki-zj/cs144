#include <iostream>
#include <string>

using namespace std;

class A {
    private:
        string str;
    public:
        A() : str("fuck A") {cout << "fuck A" << endl;}
};

class B {
    private:
        A a;
    public:
        B() { cout << "fuck B" << endl;
        a = A();}

};

int main() {
    B b;
}