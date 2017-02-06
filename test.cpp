#include <iostream>

#include "api.h"
#include "sync.h"

using namespace std;
using namespace barista;

template<typename T>
void Expect(T actual, T expected) {
  if (actual != expected) {
    cout << "Test failed:\n  Expected: " << expected << "\n  Was: " << actual << endl;
    exit(1);
  } else {
    cout << "PASSED: " << expected << endl;
  }
}

template void Expect<unsigned long>(unsigned long, unsigned long);
template void Expect<unsigned int>(unsigned int, unsigned int);
template void Expect<int>(int, int);
template void Expect<string>(string, string);

template<typename E>
void _vectorsDiffer(string message, vector<E> actual, vector<E> expected) {
  cout << message << endl;
  cout << "Expected: [";
  for (auto elem = expected.begin(); elem != expected.end(); elem++) {
    cout << *elem;
    if (expected.end() - elem > 1) { cout << ", "; }
  }
  cout << "]" << endl;

  cout << "Actual: [";
  for (auto elem = actual.begin(); elem != actual.end(); elem++) {
    cout << *elem;
    if (actual.end() - elem > 1) { cout << ", "; }
  }
  cout << "]" << endl;

  exit(1);
}

template<typename E>
void ExpectVector(vector<E> actual, vector<E> expected) {
  if (actual.size() != expected.size()) {
    _vectorsDiffer<E>("Vectors have different size", actual, expected);
  }

  auto aIter = actual.begin();
  auto eIter = expected.begin();
  while(aIter != actual.end()) {
    if (*aIter != *eIter) {
      _vectorsDiffer<E>(
          "Vectors have different elements at position " + to_string(aIter - actual.begin()),
          actual, expected
      );
    }
    aIter++;
    eIter++;
  }
}

template void ExpectVector<int>(vector<int>, vector<int>);

void ExpectTreeUpdate(shared_ptr<Tree> tree, TreeUpdate& expected) {
  auto diff = tree->RenderFrame(2);
  Expect(diff, expected.Render(2));
}
