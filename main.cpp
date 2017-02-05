#include <iostream>
#include <emscripten/emscripten.h>
#include <list>
#include <locale>
#include <codecvt>
#include <string>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;

class SampleAppState : public State {
 public:
  bool greet = true;
  int rowCounter = 1;
  list<int> rows;

  SampleAppState() {
    for (int i = 0; i < 4; i++) {
      AddRow();
    }
  };

  void AddRow() {
    rows.push_back(rowCounter++);
  }

  virtual shared_ptr<Node> Build() {
    auto container = El("div");

    auto message = El("div");
    auto text = greet ? Tx("Hello") : Tx("Ciao!!!");

    for (auto r = rows.begin(); r != rows.end(); r++) {
      auto row = El("div");
      row->SetKey(make_shared<Key>(to_string(*r)));
      row->SetText("row #" + to_string(*r));
      auto removeButton = El("button");
      removeButton->SetText("Remove");
      removeButton->AddEventListener("click", [&]() {
        rows.erase(r);
        ScheduleUpdate();
      });
      row->AddChild(removeButton);
      message->AddChild(row);
    }

    auto button = El("button");
    button->SetText("Add Row");
    button->AddEventListener("click", [&]() {
      cout << "Clicked! " << greet << endl;
      greet = !greet;
      AddRow();
      ScheduleUpdate();
    });

    container->AddChild(button);
    container->AddChild(text);
    container->AddChild(message);
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
  tree = make_shared<Tree>(make_shared<SampleApp>());
  EM_ASM(
    allReady();
  );
}

}
