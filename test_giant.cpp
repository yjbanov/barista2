#include <iostream>
#include <sstream>
#include <vector>
#include <time.h>
#include <chrono>

#include "api.h"
#include "test.h"
#include "giant_widgets.h"

using namespace std;
using namespace std::chrono;
using namespace barista;

class WrapperState : public State {
 public:
  bool visible = true;

  WrapperState() : State() { };

  virtual shared_ptr<Node> Build() {
    auto container = El("div");
    if (visible) {
      container->AddChild(make_shared<SampleApp>());
    }
    return container;
  }
};

class Wrapper : public StatefulWidget {
 public:
  shared_ptr<WrapperState> state = nullptr;

  Wrapper() : StatefulWidget() {}

  virtual shared_ptr<State> CreateState() {
    return state = make_shared<WrapperState>();
  }
};

TEST(TestBootstrapGiantApp)
  auto before_boot = system_clock::now();
  cout << "In main " << duration_cast<milliseconds>(before_boot.time_since_epoch()).count() << endl;
  auto wrapper = make_shared<Wrapper>();
  auto tree = make_shared<Tree>(wrapper);
  auto html = tree->RenderFrame();
  auto after_boot = system_clock::now();
  duration<double> delta = after_boot - before_boot;
  cout << "Bootstrap time: " << delta.count() * 1000 << "ms; tree size: " << html.size() << " chars" << endl;

  for (int flip = 1; flip <= 10; flip++) {
    auto before_flip = system_clock::now();
    wrapper->state->visible = !wrapper->state->visible;
    wrapper->state->ScheduleUpdate();
    auto html = tree->RenderFrame();
    auto after_flip = system_clock::now();
    duration<double> delta = after_flip - before_flip;
    cout << "Flip #" << flip << " took: " << delta.count() * 1000 << "ms; tree size: " << html.size() << " chars" << endl;
  }
END_TEST

int main() {
  cout << "Start tests" << endl;
  TestBootstrapGiantApp();
  cout << "End tests" << endl;
  return 0;
}
