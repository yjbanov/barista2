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

class ElementWithChildrenTest : public StatelessWidget {
 public:
  ElementWithChildrenTest() : StatelessWidget() {}
  shared_ptr<Node> Build() {
    auto elem = make_shared<Element>("div");
    auto childDiv = make_shared<Element>("div");
    childDiv->AddChild(make_shared<Text>("hello"));
    elem->AddChild(childDiv);
    return elem;
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

void TestPrintElementWithChildren() {
  auto tree = make_shared<Tree>(make_shared<ElementWithChildrenTest>());
  ExpectHtml(tree, "<div><div>hello</div></div>");
}

int main() {
  TestPrintTag();
  TestPrintText();
  TestPrintElementWithChildren();
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
