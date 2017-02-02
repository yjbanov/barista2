#include <iostream>
#include <sstream>
#include <vector>

#include "api.h"
#include "html.h"
#include "sync.h"
#include "style.h"
#include "test.h"
#include "lib/json/src/json.hpp"

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
    auto child = Tx("hello");
    elem->AddChild(child);
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
    return Tx(label);
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
  cout << "TestPrintTag" << endl;
  auto tree = make_shared<Tree>(make_shared<ElementTagTest>());
  ExpectHtml(tree, "<div></div>");
  cout << "Success" << endl;
}

void TestPrintText() {
  cout << "TestPrintText" << endl;
  auto tree = make_shared<Tree>(Tx("hello"));
  ExpectHtml(tree, "<span>hello</span>");
  cout << "Success" << endl;
}

void TestPrintElementWithChildren() {
  cout << "TestPrintElementWithChildren" << endl;
  auto tree = make_shared<Tree>(make_shared<ElementWithChildrenTest>());
  ExpectHtml(tree, "<div><span>hello</span><span></span></div>");
  cout << "Success" << endl;
}

void TestPrintElementWithAttrs() {
  cout << "TestPrintElementWithAttrs" << endl;
  auto tree = make_shared<Tree>(make_shared<ElementWithAttrsTest>());
  ExpectHtml(tree, "<div id=\"foo\"></div>");
  cout << "Success" << endl;
}

void TestEventListener() {
  cout << "TestEventListener" << endl;
  auto widget = make_shared<EventListenerTest>();
  auto tree = make_shared<Tree>(widget);
  ExpectHtml(tree, "<div><button _bid=\"1\"></button></div>");
  Expect((int) widget->eventLog.size(), 0);
  tree->DispatchEvent("click", "1");
  Expect((int) widget->eventLog.size(), 1);
  tree->DispatchEvent("scroll", "1");
  Expect((int) widget->eventLog.size(), 1);
  cout << "Success" << endl;
}

void TestClasses() {
  cout << "TestClasses" << endl;
  auto elem = make_shared<Element>("div");
  elem->AddClassName("foo");
  elem->AddClassName("bar");
  auto tree = make_shared<Tree>(elem);
  ExpectHtml(tree, "<div class=\"foo bar\"></div>");
  cout << "Success" << endl;
}

void TestStyleBasics() {
  cout << "TestStyleBasics" << endl;
  Style::DangerouslyResetIdCounterForTesting();
  vector<StyleAttribute> attrs = {
      {"padding", "5px"},
      {"margin", "8px"},
  };
  Style s = style(attrs);
  Expect(s.GetCss(), string("padding: 5px;\nmargin: 8px;\n"));
  cout << "Success" << endl;
}

void TestStyleApplication() {
  cout << "TestStyleApplication" << endl;
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
  cout << "Success" << endl;
}

void TestBasicStatefulWidget() {
  cout << "TestBasicStatefulWidget" << endl;
  auto widget = make_shared<BasicStatefulWidgetTest>();
  auto tree = make_shared<Tree>(widget);
  ExpectHtml(tree, "<span>Hello</span>");
  widget->state->label = "World";
  ExpectHtml(tree, "<span>Hello</span>");
  widget->state->ScheduleUpdate();
  ExpectHtml(tree, "<span>World</span>");
  cout << "Success" << endl;
}

void TestKeyedCreateRootDiff() {
  cout << "TestKeyedCreateRootDiff" << endl;

  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.CreateRootElement();
  rootUpdate.SetTag("div");
  rootUpdate.SetKey("a");

  auto div = make_shared<Element>("div");
  div->SetKey(make_shared<Key>("a"));
  auto tree = make_shared<Tree>(div);
  ExpectTreeUpdate(tree, treeUpdate);

  cout << "Success" << endl;
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
  cout << "TestKeyedCreateFirstChildDiff" << endl;
  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.UpdateRootElement();
  auto& childUpdate = rootUpdate.InsertChildElement(0);
  childUpdate.SetTag("span");
  childUpdate.SetKey("child");

  auto widget = make_shared<CreateFirstChildDiffTest>();
  auto tree = make_shared<Tree>(widget);
  tree->RenderFrame();
  widget->state->withChild = true;
  widget->state->ScheduleUpdate();
  ExpectTreeUpdate(tree, treeUpdate);
  cout << "Success" << endl;
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

  void ExpectStateDiff(shared_ptr<Node> afterState, TreeUpdate& expectedUpdate) {
    auto tree = make_shared<Tree>(shared_from_this());
    tree->RenderFrame();
    _state->NextState(afterState);
    _state->ScheduleUpdate();
    ExpectTreeUpdate(tree, expectedUpdate);
  }

  virtual shared_ptr<State> CreateState() { return _state; }

 private:
  shared_ptr<BeforeAfterTestState> _state;
};

void TestKeyedRemoveOnlyChildDiff() {
  cout << "TestKeyedRemoveOnlyChildDiff" << endl;

  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.UpdateRootElement();
  rootUpdate.RemoveChild(0);

  auto before = make_shared<Element>("div");
  before->SetKey(make_shared<Key>("parent"));
  auto beforeChild = make_shared<Element>("span");
  beforeChild->SetKey(make_shared<Key>("child"));
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  after->SetKey(make_shared<Key>("parent"));
  test->ExpectStateDiff(after, treeUpdate);

  cout << "Success" << endl;
}

void TestIdenticalRebuild() {
  cout << "TestIdenticalRebuild" << endl;

  auto before = make_shared<Element>("div");
  auto beforeChild = make_shared<Element>("span");
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  auto afterChild = make_shared<Element>("span");
  after->AddChild(afterChild);

  auto update = TreeUpdate();
  test->ExpectStateDiff(after, update);

  cout << "Success" << endl;
}

void ExpectLis(vector<int> input, vector<int> output) {
  ExpectVector(ComputeLongestIncreasingSubsequence(input), output);
}

void TestComputeLongestIncreasingSubsequence() {
  cout << "TestComputeLongestIncreasingSubsequence" << endl;

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

  cout << "Success" << endl;
}

void TestUnkeyedCreateRootDiff() {
  cout << "TestUnkeyedCreateRootDiff" << endl;

  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.CreateRootElement();
  rootUpdate.SetTag("div");

  auto div = make_shared<Element>("div");
  auto tree = make_shared<Tree>(div);
  ExpectTreeUpdate(tree, treeUpdate);

  cout << "Success" << endl;
}

void TestUnkeyedCreateFirstChildDiff() {
  cout << "TestUnkeyedCreateFirstChildDiff" << endl;

  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.UpdateRootElement();
  auto& childUpdate = rootUpdate.InsertChildElement(0);
  childUpdate.SetTag("span");

  auto before = make_shared<Element>("div");
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  auto afterChild = make_shared<Element>("span");
  after->AddChild(afterChild);
  test->ExpectStateDiff(after, treeUpdate);

  cout << "Success" << endl;
}

void TestKeyedHtmlDiffing() {
  TestKeyedCreateRootDiff();
  TestKeyedCreateFirstChildDiff();
  TestKeyedRemoveOnlyChildDiff();
}

void TestUnkeyedHtmlDiffing() {
  TestUnkeyedCreateRootDiff();
  TestUnkeyedCreateFirstChildDiff();
  TestIdenticalRebuild();
}

void TestHtmlDiffing() {
  TestKeyedHtmlDiffing();
  TestUnkeyedHtmlDiffing();
}

void TestSyncerCreate() {
  cout << "TestSyncerCreate" << endl;

  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.CreateRootElement();
  rootUpdate.SetTag("div");
  rootUpdate.SetKey("a");
  rootUpdate.SetText("hello");

  nlohmann::json j;
  j["create"] = "<div _bkey=\"a\">hello</div>";

  Expect(
    treeUpdate.Render(2),
    j.dump(2)
  );

  cout << "Success" << endl;
}

void TestSyncerUpdate() {
  cout << "TestSyncerUpdate" << endl;

  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.UpdateRootElement();
  rootUpdate.SetTag("div");
  rootUpdate.SetKey("a");
  rootUpdate.SetText("hello");

  nlohmann::json j;
  j["update"] = {
      {"tag", "div"},
      {"text", "hello"},
      {"index", 0},
  };

  Expect(
      treeUpdate.Render(2),
      j.dump(2)
  );

  cout << "Success" << endl;
}

int main() {
  cout << "Start tests" << endl;
  TestSyncerCreate();
  TestSyncerUpdate();
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
  cout << "End tests" << endl;
  return 0;
}
