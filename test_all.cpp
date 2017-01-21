#include <iostream>
#include <vector>

#include "api.h"
#include "html.h"
#include "sync.h"
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

class BasicStatefulWidgetTestState : public State {
 public:
  virtual shared_ptr<Node> Build() {
    return make_shared<Text>(label);
  }

  string label = "Hello";
};

class BasicStatefulWidgetTest : public StatefulWidget {
 public:
  shared_ptr<BasicStatefulWidgetTestState> state = nullptr;

  virtual shared_ptr<State> CreateState() {
    return state = make_shared<BasicStatefulWidgetTestState>();
  }
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

void TestBasicStatefulWidget() {
  auto widget = make_shared<BasicStatefulWidgetTest>();
  auto tree = make_shared<Tree>(widget);
  ExpectHtml(tree, "Hello");
  widget->state->label = "World";
  ExpectHtml(tree, "Hello");
  widget->state->ScheduleUpdate();
  ExpectHtml(tree, "World");
}

void TestKeyedCreateRootDiff() {
  ChildListDiff childDiff;
  childDiff.NewChild(0, "<div _bkey=\"a\"></div>");
  HtmlDiff diff;
  diff.UpdateChildren(&childDiff);

  auto div = make_shared<Element>("div");
  div->SetKey(make_shared<Key>("a"));
  auto tree = make_shared<Tree>(div);
  ExpectDiff(tree, diff);
}

class CreateFirstChildDiffState : public State {
 public:
  bool withChild = false;

  virtual shared_ptr<Node> Build() {
    auto parent = make_shared<Element>("div");
    parent->SetKey(make_shared<Key>("parent"));
    if (withChild) {
      auto child = make_shared<Element>("span");
      child->SetKey(make_shared<Key>("child"));
      parent->AddChild(child);
    }
    return parent;
  }
};

class CreateFirstChildDiffTest : public StatefulWidget {
 public:
  shared_ptr<CreateFirstChildDiffState> state = nullptr;

  virtual shared_ptr<State> CreateState() {
    return state = make_shared<CreateFirstChildDiffState>();
  }
};

void TestKeyedCreateFirstChildDiff() {
  HtmlDiff diff;

  ChildListDiff parentDiff;
  parentDiff.UpdateChild(0);
  diff.UpdateChildren(&parentDiff);

  ChildListDiff childDiff;
  childDiff.NewChild(0, "<span _bkey=\"child\"></span>");
  diff.UpdateChildren(&childDiff);

  auto widget = make_shared<CreateFirstChildDiffTest>();
  auto tree = make_shared<Tree>(widget);
  tree->RenderFrame();
  widget->state->withChild = true;
  widget->state->ScheduleUpdate();
  ExpectDiff(tree, diff);
}

class BeforeAfterTestState : public State {
 public:
  BeforeAfterTestState(shared_ptr<Node> beforeState) : _state(beforeState) { };

  void NextState(shared_ptr<Node> nextState) { _state = nextState; }

  virtual shared_ptr<Node> Build() { return _state; }

 private:
  shared_ptr<Node> _state;
};

class BeforeAfterTest : public StatefulWidget {
 public:
  BeforeAfterTest(shared_ptr<Node> beforeState) {
    _state = make_shared<BeforeAfterTestState>(beforeState);
  };

  void ExpectStateDiff(shared_ptr<Node> afterState, HtmlDiff & expectedDiff) {
    auto tree = make_shared<Tree>(shared_from_this());
    tree->RenderFrame();
    _state->NextState(afterState);
    _state->ScheduleUpdate();
    ExpectDiff(tree, expectedDiff);
  }

  virtual shared_ptr<State> CreateState() { return _state; }

 private:
  shared_ptr<BeforeAfterTestState> _state;
};

void TestKeyedRemoveOnlyChildDiff() {
  HtmlDiff diff;

  ChildListDiff parentDiff;
  parentDiff.UpdateChild(0);
  diff.UpdateChildren(&parentDiff);

  ChildListDiff childDiff;
  childDiff.Remove(0);
  diff.UpdateChildren(&childDiff);

  auto before = make_shared<Element>("div");
  before->SetKey(make_shared<Key>("parent"));
  auto beforeChild = make_shared<Element>("span");
  beforeChild->SetKey(make_shared<Key>("child"));
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  after->SetKey(make_shared<Key>("parent"));
  test->ExpectStateDiff(after, diff);
}

void TestIdenticalRebuild() {
  HtmlDiff diff;

  auto before = make_shared<Element>("div");
  auto beforeChild = make_shared<Element>("span");
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  auto afterChild = make_shared<Element>("span");
  after->AddChild(afterChild);
  test->ExpectStateDiff(after, diff);
}

void TestMixedHtmlDiffing() {
  HtmlDiff diff;

  auto b = make_shared<Element>("div");
  auto bc0 = make_shared<Element>("span");
  bc0->SetKey(make_shared<Key>("1"));
  b->AddChild(bc0);
  auto bc1 = make_shared<Element>("span");
  b->AddChild(bc1);
  auto bc2 = make_shared<Element>("span");
  bc2->SetKey(make_shared<Key>("2"));
  b->AddChild(bc2);
  auto bc3 = make_shared<Element>("span");
  b->AddChild(bc3);
  auto bc4 = make_shared<Element>("span");
  b->AddChild(bc4);
  auto bc5 = make_shared<Element>("span");
  bc5->SetKey(make_shared<Key>("3"));
  b->AddChild(bc5);
  auto bc6 = make_shared<Element>("span");
  bc6->SetKey(make_shared<Key>("4"));
  b->AddChild(bc6);
  auto bc7 = make_shared<Element>("span");
  b->AddChild(bc7);
  auto test = make_shared<BeforeAfterTest>(b);

  auto after = make_shared<Element>("div");
  auto afterChild = make_shared<Element>("span");
  after->AddChild(afterChild);
  test->ExpectStateDiff(after, diff);
}

void ExpectLis(vector<int> input, vector<int> output) {
  ExpectVector(ComputeLongestIncreasingSubsequence(input), output);
}

void TestComputeLongestIncreasingSubsequence() {
  // trivial
  ExpectLis({}, {});
  ExpectLis({1}, {1});
  ExpectLis({1, 4, 6, 7, 8}, {1, 4, 6, 7, 8});

  // swaps
  ExpectLis({1, 3, 2}, {1, 2});
  ExpectLis({0, 3, 2, 4}, {0, 2, 4});
  ExpectLis({3, 2, 1}, {1});

  // shifts
  ExpectLis({1, 2, 3, 4, 5, 0}, {1, 2, 3, 4, 5});
  ExpectLis({5, 0, 1, 2, 3, 4}, {0, 1, 2, 3, 4});
}

void TestUnkeyedCreateRootDiff() {
  ChildListDiff childDiff;
  childDiff.NewChild(0, "<div></div>");
  HtmlDiff diff;
  diff.UpdateChildren(&childDiff);

  auto div = make_shared<Element>("div");
  auto tree = make_shared<Tree>(div);
  ExpectDiff(tree, diff);
}

void TestUnkeyedCreateFirstChildDiff() {
  HtmlDiff diff;

  auto before = make_shared<Element>("div");
  auto beforeChild = make_shared<Element>("span");
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  auto afterChild = make_shared<Element>("span");
  after->AddChild(afterChild);
  test->ExpectStateDiff(after, diff);
}

void TestKeyedHtmlDiffing() {
  TestKeyedCreateRootDiff();
  TestKeyedCreateFirstChildDiff();
  TestKeyedRemoveOnlyChildDiff();
}

void TestUnkeyedHtmlDiffing() {
  TestUnkeyedCreateRootDiff();
  // TestUnkeyedCreateFirstChildDiff();
  //TestIdenticalRebuild();
}

void TestHtmlDiffing() {
  TestKeyedHtmlDiffing();
  TestUnkeyedHtmlDiffing();
  //TestMixedHtmlDiffing();
}

void TestSyncer() {
  auto host = sync::ElementUpdate(-1);
  auto root = host.InsertChildElement(0);
  root.SetTag("span");
  root.InsertChildText(0, "Hello, World!");
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
  TestBasicStatefulWidget();
  TestComputeLongestIncreasingSubsequence();
  TestHtmlDiffing();
  TestSyncer();
  cout << "End tests" << endl;
  return 0;
}
