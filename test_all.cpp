#include <iostream>
#include <vector>

#include "api.h"
#include "html.h"
#include "style.h"
#include "test.h"

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
  Expect(widget->eventLog.size(), 0UL);
  tree->DispatchEvent("click", "1");
  Expect(widget->eventLog.size(), 1UL);
  tree->DispatchEvent("scroll", "1");
  Expect(widget->eventLog.size(), 1UL);
}

void TestClasses() {
  auto elem = make_shared<Element>("div");
  elem->AddClassName("foo");
  elem->AddClassName("bar");
  auto tree = make_shared<Tree>(elem);
  ExpectHtml(tree, "<div class=\"foo bar\"></div>");
}

void TestStyleBasics() {
  Style::DangerouslyResetIdCounterForTesting();
  vector<StyleAttribute> attrs = {
      {"padding", "5px"},
      {"margin", "8px"},
  };
  Style s = style(attrs);
  Expect(s.GetCss(), string("padding: 5px;\nmargin: 8px;\n"));
}

void TestStyleApplication() {
  Style::DangerouslyResetIdCounterForTesting();
  vector<StyleAttribute> attrs = { };
  Style s1 = style(attrs);
  Style s2 = style(attrs);

  auto elem = make_shared<Element>("div");
  elem->AddStyle(s1);
  elem->AddStyle(s2);
  elem->AddClassName("foo");
  auto tree = make_shared<Tree>(elem);
  ExpectHtml(tree, "<div class=\"_s1 _s2 foo\"></div>");
}

int main() {
  cout << "Start tests" << endl;
  TestPrintTag();
  TestPrintText();
  TestPrintElementWithChildren();
  TestPrintElementWithAttrs();
  TestEventListener();
  TestClasses();
  TestStyleBasics();
  TestStyleApplication();
  cout << "End tests" << endl;
  return 0;
}
