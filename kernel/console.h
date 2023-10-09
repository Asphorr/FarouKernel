#include <iostream>
using namespace std;

int main() {
    cout << "Hello, world!" << endl;

    char ch;
    cin >> ch;

    string str;
    getline(cin, str);

    cout << "Read character: " << ch << endl;
    cout << "Read string: " << str << endl;

    return 0;
}
