#include <iostream>
#include <emscripten/emscripten.h>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;

class SampleApp : public StatelessWidget {
 public:
  SampleApp() : StatelessWidget() {}
  shared_ptr<Node> Build() {
    auto container = El("div");

    auto message = El("div");
    message->AddChild(Tx("Hello"));

    auto button = El("button");
    button->AddEventListener("click", []() {
      cout << "Clicked!" << endl;
    });
    button->AddChild(Tx("Greeting"));

    container->AddChild(message);
    container->AddChild(button);
    return container;
  }
};

shared_ptr<Tree> tree;

extern "C" {

const char *RenderFrame() {
  return tree->RenderFrame().c_str();
}

void DispatchEvent(char* type, char* baristaId) {
  tree->DispatchEvent(type, baristaId);
}

int main() {
  tree = make_shared<Tree>(make_shared<SampleApp>());
  EM_ASM(
    console.log('Entered main()');
    allReady();
  );
}

}
