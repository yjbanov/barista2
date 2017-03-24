#include <iostream>

#include "api.h"
#include "html.h"
#include "sync.h"

using namespace std;
using namespace barista;

template<typename T>
void Expect(T actual, T expected) {
  if (actual != expected) {
    cout << "Test failed:\n  Expected: " << expected << "\n  Was: " << actual << endl;
    exit(1);
  } else {
    cout << "PASSED: " << expected << endl;
  }
}

template void Expect<unsigned long>(unsigned long, unsigned long);
template void Expect<unsigned int>(unsigned int, unsigned int);
template void Expect<int>(int, int);
template void Expect<bool>(bool, bool);
template void Expect<string>(string, string);

template<typename E>
void _vectorsDiffer(string message, vector<E> actual, vector<E> expected) {
  cout << message << endl;
  cout << "Expected: [";
  for (auto elem = expected.begin(); elem != expected.end(); elem++) {
    cout << *elem;
    if (expected.end() - elem > 1) { cout << ", "; }
  }
  cout << "]" << endl;

  cout << "Actual: [";
  for (auto elem = actual.begin(); elem != actual.end(); elem++) {
    cout << *elem;
    if (actual.end() - elem > 1) { cout << ", "; }
  }
  cout << "]" << endl;

  exit(1);
}

template<typename E>
void ExpectVector(vector<E> actual, vector<E> expected) {
  if (actual.size() != expected.size()) {
    _vectorsDiffer<E>("Vectors have different size", actual, expected);
  }

  auto aIter = actual.begin();
  auto eIter = expected.begin();
  while(aIter != actual.end()) {
    if (*aIter != *eIter) {
      _vectorsDiffer<E>(
          "Vectors have different elements at position " + to_string(aIter - actual.begin()),
          actual, expected
      );
    }
    aIter++;
    eIter++;
  }
}

template void ExpectVector<int>(vector<int>, vector<int>);
template void ExpectVector<string>(vector<string>, vector<string>);

void ExpectTreeUpdate(shared_ptr<Tree> tree, TreeUpdate& expected) {
  auto diff = tree->RenderFrame(2);
  Expect(diff, expected.Render(2));
}

class ChildDiffState : public State {
 public:
  virtual shared_ptr<Node> Build() {
    auto parent = El("parent");
    for (auto& childSpec : spec) {
      auto child = parent->El(get<0>(childSpec));
      auto key = get<1>(childSpec);
      if (key != "") {
        child->SetKey(key);
      }
    }
    return parent;
  }

  vector<tuple<string, string>> spec;
};

class ChildDiff : public StatefulWidget {
 public:
  ChildDiff()
    : state(make_shared<ChildDiffState>()) { };

  virtual shared_ptr<State> CreateState() {
    return state;
  }

  shared_ptr<ChildDiffState> state;
};

void ExpectChildDiff(
    vector<tuple<string, string>> before,
    vector<tuple<string, string>> after,
    TreeUpdate & expectedUpdate
) {
  auto widget = make_shared<ChildDiff>();

  // Render before; drop the diff
  widget->state->spec = before;
  auto tree = make_shared<Tree>(widget);
  tree->RenderFrame();

  // Render after; compare the diff
  widget->state->spec = after;
  widget->state->ScheduleUpdate();

  ExpectTreeUpdate(tree, expectedUpdate);
}
