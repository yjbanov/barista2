#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

int main() {
  auto before_boot = system_clock::now();
  cout << duration_cast<milliseconds>(before_boot.time_since_epoch()).count() << endl;
}
