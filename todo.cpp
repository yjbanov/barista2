#include <iostream>
#include <emscripten/emscripten.h>
#include <list>
#include <locale>
#include <codecvt>
#include <string>

#include "api.h"
#include "html.h"
#include "todo_widgets.h"

using namespace std;
using namespace barista;

shared_ptr<Tree> tree;

// This is intentionally static so that the string is not
// destroyed after it is returned to the JS side.
string lastDiff;

extern "C" {

const char* RenderFrame() {
  lastDiff = tree->RenderFrame(2);
  return lastDiff.c_str();
}

void DispatchEvent(char* type, char* baristaId) {
  tree->DispatchEvent(type, baristaId);
}

int main() {
  EM_ASM(
      enteredMain();
  );
  tree = make_shared<Tree>(make_shared<SampleApp>());
  EM_ASM(
    allReady();
  );
}

}
