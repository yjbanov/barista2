#include <iostream>

#include "api.h"

using namespace std;
using namespace barista;

template<typename T>
void Expect(T actual, T expected) {
  if (actual != expected) {
    cout << "Test failed:\n  Expected: " << expected << "\n  Was: " << actual << endl;
    exit(1);
  }
}

template void Expect<unsigned long>(unsigned long, unsigned long);
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

void ExpectHtml(shared_ptr<Tree> tree, string expectedHtml) {
  tree->RenderFrame();
  auto html = new string("");
  tree->PrintHtml(*html);
  Expect(*html, expectedHtml);
}

void ExpectDiff(shared_ptr<Tree> tree, HtmlDiff& expected) {
  auto diff = tree->RenderFrame();
  Expect(diff, expected.ComputeDiff());
}
