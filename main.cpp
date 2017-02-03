#include <iostream>
#include <emscripten/emscripten.h>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;

class SampleAppState : public State {
 public:
  bool greet = true;
  int rows = 5;

  virtual shared_ptr<Node> Build() {
    auto container = El("div");

    auto message = El("div");
    if (greet) {
      message->AddChild(Tx("Hello"));
    } else {
      message->AddChild(Tx("Ciao!!!"));
    }

    for (int i = 0; i < rows; i++) {
      cout << "adding row " << i << endl;
      auto row = El("div");
      row->AddChild(Tx("row #" + to_string(i)));
      message->AddChild(row);
    }

    auto button = El("button");
    button->AddEventListener("click", [&]() {
      cout << "Clicked! " << greet << endl;
      greet = !greet;
      rows++;
      ScheduleUpdate();
    });
    button->AddChild(Tx("Greeting"));

    container->AddChild(message);
    container->AddChild(button);
    return container;
  }
};

class SampleApp : public StatefulWidget {
 public:
  SampleApp() : StatefulWidget() {}

  virtual shared_ptr<State> CreateState() {
    return make_shared<SampleAppState>();
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
