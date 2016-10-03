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

void ExpectHtml(shared_ptr<Tree> tree, string expectedHtml) {
  tree->RenderFrame();
  auto html = new string("");
  tree->PrintHtml(*html);
  Expect(*html, expectedHtml);
}
