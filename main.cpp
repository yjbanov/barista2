#include <iostream>
#include <memory>

#include "api.h"

using namespace std;
using namespace barista;

class TestWidget : public StatelessWidget {
 public:
  TestWidget() : StatelessWidget(nullptr) {}
  shared_ptr<Node> Build() {
    return nullptr;
  }
};

int main() {
  auto tree = make_shared<Tree>(make_shared<TestWidget>());
  tree->RenderFrame();
  cout << "Hello, World!" << endl;
  return 0;
}
