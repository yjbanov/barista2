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

void Expect(string, string);
void ExpectHtml(shared_ptr<Tree>, string);

void TestPrintTag() {
  auto tree = make_shared<Tree>(make_shared<TestWidget>());
  ExpectHtml(tree, "<div></div>");
}

void TestPrintText() {
  auto tree = make_shared<Tree>(make_shared<Text>("hello"));
  ExpectHtml(tree, "hello");
}

int main() {
  TestPrintTag();
  TestPrintText();
  return 0;
}

void Expect(string actual, string expected) {
  if (actual != expected) {
    cout << "Test failed:\n  Expected: " << expected << "\n  Was: " << actual << endl;
    exit(1);
  }
}

void ExpectHtml(shared_ptr<Tree> tree, string expectedHtml) {
  tree->RenderFrame();
  auto html = new string("");
  tree->PrintHtml(*html);
  Expect(*html, expectedHtml);
}
