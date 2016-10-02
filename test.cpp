#include <iostream>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;

class ElementTagTest : public StatelessWidget {
 public:
  ElementTagTest() : StatelessWidget() {}
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
    elem->AddChild(make_shared<Element>("span"));
    return elem;
  }
};

class ElementWithAttrsTest : public StatelessWidget {
 public:
  ElementWithAttrsTest() : StatelessWidget() {}
  shared_ptr<Node> Build() {
    auto elem = make_shared<Element>("div");
    elem->SetAttribute("id", "foo");
    return elem;
  }
};

class EventListenerTest : public StatelessWidget {
 public:
  EventListenerTest() : StatelessWidget() {}
  shared_ptr<Node> Build() {
      auto div = make_shared<Element>("div");
      auto btn = make_shared<Element>("button");
      btn->AddEventListener("click", [this]() {
        eventLog.push_back("click");
      });
      div->AddChild(btn);
      return div;
  }

  vector<string> eventLog;
};

void Expect(unsigned long, unsigned long);
void Expect(string, string);
void ExpectHtml(shared_ptr<Tree>, string);

void TestPrintTag() {
  auto tree = make_shared<Tree>(make_shared<ElementTagTest>());
  ExpectHtml(tree, "<div></div>");
}

void TestPrintText() {
  auto tree = make_shared<Tree>(make_shared<Text>("hello"));
  ExpectHtml(tree, "hello");
}

void TestPrintElementWithChildren() {
  auto tree = make_shared<Tree>(make_shared<ElementWithChildrenTest>());
  ExpectHtml(tree, "<div><div>hello</div><span></span></div>");
}

void TestPrintElementWithAttrs() {
  auto tree = make_shared<Tree>(make_shared<ElementWithAttrsTest>());
  ExpectHtml(tree, "<div id=\"foo\"></div>");
}

void TestEventListener() {
  auto widget = make_shared<EventListenerTest>();
  auto tree = make_shared<Tree>(widget);
  ExpectHtml(tree, "<div><button _bid=\"1\"></button></div>");
  Expect(widget->eventLog.size(), 0);
  tree->DispatchEvent("click", "1");
  Expect(widget->eventLog.size(), 1);
  tree->DispatchEvent("scroll", "1");
  Expect(widget->eventLog.size(), 1);
}

int main() {
  cout << "Start tests" << endl;
  TestPrintTag();
  TestPrintText();
  TestPrintElementWithChildren();
  TestPrintElementWithAttrs();
  TestEventListener();
  cout << "End tests" << endl;
  return 0;
}

template<typename T>
void Expect(T actual, T expected) {
  if (actual != expected) {
    cout << "Test failed:\n  Expected: " << expected << "\n  Was: " << actual << endl;
    exit(1);
  }
}

void Expect(string actual, string expected) {
  Expect<string>(actual, expected);
}

void Expect(unsigned long actual, unsigned long expected) {
  Expect<unsigned long>(actual, expected);
}

void ExpectHtml(shared_ptr<Tree> tree, string expectedHtml) {
  tree->RenderFrame();
  auto html = new string("");
  tree->PrintHtml(*html);
  Expect(*html, expectedHtml);
}
