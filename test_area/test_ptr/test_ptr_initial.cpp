#include <iostream>
using namespace std;

int main() {
    unsigned char* data;
    data = (unsigned char *)malloc(16);
    for (int i = 0; i < 16; i++) {
        data[i] = i;
        cout << static_cast<int>(data[i]) << " ";
    }
    cout << endl;
    free(data);
}