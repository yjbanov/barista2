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

class CreateFirstChildDiffState : public State {
 public:
  bool withChild = false;

  virtual shared_ptr<Node> Build() {
    auto parent = make_shared<Element>("div");
    parent->SetKey("parent");
    if (withChild) {
      auto child = make_shared<Element>("span");
      child->SetKey("child");
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



TEST(TestPrintTag)
  auto update = TreeUpdate();
  auto& rootUpdate = update.CreateRootElement();
  rootUpdate.SetTag("div");
  auto tree = make_shared<Tree>(make_shared<ElementTagTest>());
  ExpectTreeUpdate(tree, update);
END_TEST

TEST(TestPrintText)
  auto tree = make_shared<Tree>(Tx("hello"));
  ExpectHtml(tree, "<span>hello</span>");
END_TEST

TEST(TestPrintElementWithChildren)
  auto tree = make_shared<Tree>(make_shared<ElementWithChildrenTest>());
  ExpectHtml(tree, "<div><span>hello</span><span></span></div>");
END_TEST

TEST(TestPrintElementWithAttrs)
  auto tree = make_shared<Tree>(make_shared<ElementWithAttrsTest>());
  ExpectHtml(tree, "<div id=\"foo\"></div>");
END_TEST

TEST(TestPrintEventListener)
  auto widget = make_shared<EventListenerTest>();
  auto tree = make_shared<Tree>(widget);
  ExpectHtml(tree, "<div><button _bid=\"1\"></button></div>");
  Expect((int) widget->eventLog.size(), 0);
  tree->DispatchEvent("click", "1");
  Expect((int) widget->eventLog.size(), 1);
  tree->DispatchEvent("scroll", "1");
  Expect((int) widget->eventLog.size(), 1);
END_TEST

TEST(TestPrintClasses)
  auto elem = make_shared<Element>("div");
  elem->AddClassName("foo");
  elem->AddClassName("bar");
  auto tree = make_shared<Tree>(elem);
  ExpectHtml(tree, "<div class=\"foo bar\"></div>");
END_TEST

TEST(TestStyleBasics)
  Style::DangerouslyResetIdCounterForTesting();
  vector<StyleAttribute> attrs = {
      {"padding", "5px"},
      {"margin", "8px"},
  };
  Style s = style(attrs);
  Expect(s.GetCss(), string("padding: 5px;\nmargin: 8px;\n"));
END_TEST

TEST(TestStyleApplication)
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
END_TEST

TEST(TestBasicStatefulWidget)
  auto widget = make_shared<BasicStatefulWidgetTest>();
  auto tree = make_shared<Tree>(widget);
  ExpectHtml(tree, "<span>Hello</span>");
  widget->state->label = "World";
  ExpectHtml(tree, "<span>Hello</span>");
  widget->state->ScheduleUpdate();
  ExpectHtml(tree, "<span>World</span>");
  cout << "Success" << endl;
END_TEST

TEST(TestKeyedCreateRootDiff)
  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.CreateRootElement();
  rootUpdate.SetTag("div");
  rootUpdate.SetKey("a");

  auto div = make_shared<Element>("div");
  div->SetKey("a");
  auto tree = make_shared<Tree>(div);
  ExpectTreeUpdate(tree, treeUpdate);
END_TEST

TEST(TestKeyedCreateFirstChildDiff)
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
END_TEST

TEST(TestKeyedRemoveOnlyChildDiff)
  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.UpdateRootElement();
  rootUpdate.RemoveChild(0);

  auto before = make_shared<Element>("div");
  before->SetKey("parent");
  auto beforeChild = make_shared<Element>("span");
  beforeChild->SetKey("child");
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  after->SetKey("parent");
  test->ExpectStateDiff(after, treeUpdate);
END_TEST

TEST(TestIdenticalRebuild)
  auto before = make_shared<Element>("div");
  auto beforeChild = make_shared<Element>("span");
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  auto afterChild = make_shared<Element>("span");
  after->AddChild(afterChild);

  auto update = TreeUpdate();
  test->ExpectStateDiff(after, update);
END_TEST

void ExpectLis(vector<int> input, vector<int> output) {
  ExpectVector(ComputeLongestIncreasingSubsequence(input), output);
}

TEST(TestComputeLongestIncreasingSubsequence)
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
END_TEST

TEST(TestUnkeyedCreateRootDiff)
  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.CreateRootElement();
  rootUpdate.SetTag("div");

  auto div = make_shared<Element>("div");
  auto tree = make_shared<Tree>(div);
  ExpectTreeUpdate(tree, treeUpdate);
END_TEST

TEST(TestUnkeyedCreateFirstChildDiff)
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
END_TEST

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

TEST(TestSyncerCreate)
  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.CreateRootElement();
  rootUpdate.SetTag("div");
  rootUpdate.SetKey("a");
  rootUpdate.SetAttribute("a", "b");
  rootUpdate.SetAttribute("c", "d");
  auto& childUpdate = rootUpdate.InsertChildElement(0);
  childUpdate.SetTag("button");
  childUpdate.SetText("hello");

  nlohmann::json j;
  j["create"] = "<div _bkey=\"a\" a=\"b\" c=\"d\"><button>hello</button></div>";

  Expect(
    treeUpdate.Render(2),
    j.dump(2)
  );
END_TEST

TEST(TestSyncerUpdate)
  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.UpdateRootElement();
  rootUpdate.SetTag("div");
  rootUpdate.SetKey("a");
  rootUpdate.SetText("hello");
  rootUpdate.SetAttribute("a", "b");

  nlohmann::json j;
  j["update"] = {
      {"tag", "div"},
      {"text", "hello"},
      {"index", 0},
      {"attrs", {
          {"a", "b"}
      }}
  };

  Expect(
      treeUpdate.Render(2),
      j.dump(2)
  );
END_TEST

TEST(TestAttrsCreate)
  auto treeUpdate = TreeUpdate();
  auto &rootUpdate = treeUpdate.CreateRootElement();
  rootUpdate.SetTag("div");
  rootUpdate.SetAttribute("id", "1");
  rootUpdate.SetAttribute("style", "hidden: true;");

  auto div = make_shared<Element>("div");
  div->SetAttribute("id", "1");
  div->SetAttribute("style", "hidden: true;");
  auto tree = make_shared<Tree>(div);
  ExpectTreeUpdate(tree, treeUpdate);
END_TEST

TEST(TestAttrsUpdate)
  auto treeUpdate = TreeUpdate();
  auto& rootUpdate = treeUpdate.UpdateRootElement();
  auto& childUpdate = rootUpdate.UpdateChildElement(0);
  childUpdate.SetAttribute("update", "a");
  childUpdate.SetAttribute("create", "b");
  childUpdate.SetAttribute("remove", "");

  auto before = make_shared<Element>("div");
  auto beforeChild = make_shared<Element>("span");
  beforeChild->SetAttribute("unchanged", "u");
  beforeChild->SetAttribute("update", "z");
  beforeChild->SetAttribute("remove", "y");
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  auto afterChild = make_shared<Element>("span");
  afterChild->SetAttribute("unchanged", "u");
  afterChild->SetAttribute("update", "a");
  afterChild->SetAttribute("create", "b");
  after->AddChild(afterChild);
  test->ExpectStateDiff(after, treeUpdate);
END_TEST

TEST(TestAddEventListeners)
  RenderElement::DangerouslyResetBaristaIdCounterForTesting();
  auto treeUpdate = TreeUpdate();
  auto &rootUpdate = treeUpdate.CreateRootElement();
  rootUpdate.SetTag("div");
  rootUpdate.SetBaristaId("1");

  auto div = make_shared<Element>("div");
  div->AddEventListener("click", []() {});
  auto tree = make_shared<Tree>(div);
  ExpectTreeUpdate(tree, treeUpdate);
END_TEST

TEST(TestPreserveEventListeners)
  RenderElement::DangerouslyResetBaristaIdCounterForTesting();
  auto treeUpdate = TreeUpdate();

  auto before = make_shared<Element>("div");
  auto beforeChild = make_shared<Element>("span");
  beforeChild->AddEventListener("click", [](){});
  before->AddChild(beforeChild);
  auto test = make_shared<BeforeAfterTest>(before);

  auto after = make_shared<Element>("div");
  auto afterChild = make_shared<Element>("span");
  afterChild->AddEventListener("click", [](){});
  after->AddChild(afterChild);
  test->ExpectStateDiff(after, treeUpdate);
END_TEST

int main() {
  cout << "Start tests" << endl;
  TestSyncerCreate();
  TestSyncerUpdate();
  TestPrintTag();
  TestPrintText();
  TestPrintElementWithChildren();
  TestPrintElementWithAttrs();
  TestPrintEventListener();
  TestPrintClasses();
  TestStyleBasics();
  TestStyleApplication();
  TestBasicStatefulWidget();
  TestComputeLongestIncreasingSubsequence();
  TestHtmlDiffing();
  TestAttrsCreate();
  TestAttrsUpdate();
  TestAddEventListeners();
  TestPreserveEventListeners();
  cout << "End tests" << endl;
  return 0;
}
