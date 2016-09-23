#include <iostream>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;

class TestWidget : public StatelessWidget {
 public:
  TestWidget() : StatelessWidget() {}
  shared_ptr<Node> Build() {
    return make_shared<Element>("div");
  }
};

int main() {
  auto tree = make_shared<Tree>(make_shared<TestWidget>());
  tree->RenderFrame();
  auto html = new string("HTML: ");
  tree->PrintHtml(*html);
  cout << *html << endl;
  return 0;
}
