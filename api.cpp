//
// Created by Yegor Jbanov on 9/4/16.
//

#include "api.h"
#include "diff.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <iostream>

using namespace std;

namespace barista {

bool _sameType(void *a, void *b) {
  return typeid(a) == typeid(b);
}

bool _canUpdate(shared_ptr<RenderNode> node, shared_ptr<Node> configuration) {
  if (!_sameType(node->GetConfiguration().get(), configuration.get())) {
    return false;
  }

  return node->GetConfiguration()->GetKey() == configuration->GetKey();
}

bool operator==(const std::shared_ptr<Key> a, const std::shared_ptr<Key> b) {
  return (a == nullptr && b == nullptr) ||
      (a != nullptr && b != nullptr && a->_value == b->_value);
}

RenderNode::RenderNode(shared_ptr<Tree> tree) : _tree(tree) { }

bool RenderNode::Update(shared_ptr<Node> newConfiguration) {
  assert(newConfiguration != nullptr);
  _configuration = newConfiguration;
  return true;
}


RenderParent::RenderParent(shared_ptr<Tree> tree) : RenderNode(tree) { }

void RenderParent::ScheduleUpdate() {
  _hasDescendantsNeedingUpdate = true;
  shared_ptr<RenderParent> parent = GetParent();
  while (parent != nullptr) {
    parent->_hasDescendantsNeedingUpdate = true;
    parent = parent->GetParent();
  }
}

bool RenderParent::Update(shared_ptr<Node> newConfiguration) {
  _hasDescendantsNeedingUpdate = false;
  RenderNode::Update(newConfiguration);
  return true;
}

string Tree::RenderFrame() {
  GetHtmlDiff()->AssertReady();

  ChildListDiff diff;
  if (_topLevelNode == nullptr) {
    _topLevelNode = _topLevelWidget->Instantiate(shared_from_this());
    GetHtmlDiff()->Lock();
    _topLevelNode->Update(_topLevelWidget);
    string html;
    _topLevelNode->PrintHtml(html);
    diff.NewChild(0, html);
    GetHtmlDiff()->Unlock();
    diff.Render();
  } else {
    diff.UpdateChild(0);
    diff.Render();
    _topLevelNode->Update(_topLevelWidget);
  }

  return GetHtmlDiff()->Finalize();
}

void Tree::VisitChildren(RenderNodeVisitor visitor) {
  visitor(_topLevelNode);
}

void Tree::DispatchEvent(string type, string baristaId) {
  _topLevelNode->DispatchEvent(type, baristaId);
}

void Tree::PrintHtml(string &buf) {
  if (_topLevelNode == nullptr) {
    buf += "null";
  } else {
    _topLevelNode->PrintHtml(buf);
  }
}


shared_ptr<RenderNode> StatelessWidget::Instantiate(shared_ptr<Tree> tree) {
  return make_shared<RenderStatelessWidget>(tree);
}

shared_ptr<RenderNode> StatefulWidget::Instantiate(shared_ptr<Tree> t) {
  return make_shared<RenderStatefulWidget>(t);
}

void State::ScheduleUpdate() { _node->ScheduleUpdate(); }

void internalSetStateNode(shared_ptr<State> state, shared_ptr<RenderStatefulWidget> node) {
  state->_node = node;
}

void RenderStatelessWidget::DispatchEvent(string type, string baristaId) {
  if (_child == nullptr) return;
  _child->DispatchEvent(type, baristaId);
}

bool RenderStatelessWidget::Update(shared_ptr<Node> configPtr) {
  assert(configPtr != nullptr);

  if (typeid(GetConfiguration()) != typeid(configPtr)) {
    return false;
  }

  auto newConfiguration = static_pointer_cast<StatelessWidget>(configPtr);
  if (GetConfiguration() != newConfiguration) {
    // Build the new configuration and decide whether to reuse the child node
    // or replace with a new one.
    shared_ptr<Node> newChildConfiguration = newConfiguration->Build();
    if (_child != nullptr && _canUpdate(_child, newChildConfiguration)) {
      _child->Update(newChildConfiguration);
    } else {
      // Replace child
      if (_child != nullptr) {
        _child->Detach();
      }
      _child = newChildConfiguration->Instantiate(GetTree());
      _child->Update(newChildConfiguration);
      _child->Attach(shared_from_this());
    }
  } else if (GetHasDescendantsNeedingUpdate()) {
    assert(_child != nullptr);
    // Own configuration is the same, but some children are scheduled to be
    // updated.
    _child->Update(_child->GetConfiguration());
  }
  return RenderParent::Update(newConfiguration);
}

void RenderStatefulWidget::VisitChildren(RenderNodeVisitor visitor) {
  visitor(_child);
}

void RenderStatefulWidget::ScheduleUpdate() {
  _isDirty = true;
  RenderParent::ScheduleUpdate();
}

void RenderStatefulWidget::DispatchEvent(string type, string baristaId) {
  if (_child == nullptr) return;
  _child->DispatchEvent(type, baristaId);
}

bool RenderStatefulWidget::Update(shared_ptr<Node> configPtr) {
  assert(configPtr != nullptr);

  if (typeid(GetConfiguration()) != typeid(configPtr)) {
    return false;
  }

  auto newConfiguration = static_pointer_cast<StatefulWidget>(configPtr);
  if (GetConfiguration() != newConfiguration) {
    // Build the new configuration and decide whether to reuse the child node
    // or replace with a new one.
    _state = newConfiguration->CreateState();
    _state->_config = newConfiguration;
    internalSetStateNode(_state, shared_from_this());
    shared_ptr<Node> newChildConfiguration = _state->Build();
    if (_child != nullptr && _sameType(newChildConfiguration.get(), _child->GetConfiguration().get())) {
      _child->Update(newChildConfiguration);
    } else {
      if (_child != nullptr) {
        _child->Detach();
      }
      _child = newChildConfiguration->Instantiate(GetTree());
      _child->Update(newChildConfiguration);
      _child->Attach(shared_from_this());
    }
  } else if (_isDirty) {
    _child->Update(_state->Build());
  } else if (GetHasDescendantsNeedingUpdate()) {
    // Own configuration is the same, but some children are scheduled to be
    // updated.
    _child->Update(_child->GetConfiguration());
  }

  _isDirty = false;
  return RenderParent::Update(newConfiguration);
}

void RenderMultiChildParent::VisitChildren(RenderNodeVisitor visitor) {
  for (auto child : _currentChildren) {
    visitor(child);
  }
}

vector<int> ComputeLongestIncreasingSubsequence(vector<int> & sequence) {
  auto len = sequence.size();
  vector<int> predecessors;
  vector<int> mins = {0};
  int longest = 0;
  for (int i = 0; i < len; i++) {
    // Binary search for the largest positive `j ≤ longest`
    // such that `list[mins[j]] < list[i]`
    int elem = sequence[i];
    int lo = 1;
    int hi = longest;
    while (lo <= hi) {
      int mid = (lo + hi) / 2;
      if (sequence[mins[mid]] < elem) {
        lo = mid + 1;
      } else {
        hi = mid - 1;
      }
    }

    // After searching, `lo` is 1 greater than the
    // length of the longest prefix of `list[i]`
    int expansionIndex = lo;

    // The predecessor of `list[i]` is the last index of
    // the subsequence of length `newLongest - 1`
    predecessors.push_back(mins[expansionIndex - 1]);
    if (expansionIndex >= mins.size()) {
      mins.push_back(i);
    } else {
      mins[expansionIndex] = i;
    }

    if (expansionIndex > longest) {
      // If we found a subsequence longer than any we've
      // found yet, update `longest`
      longest = expansionIndex;
    }
  }

  // Reconstruct the longest subsequence
  vector<int> lis((unsigned long) longest);
  int k = mins[longest];
  for (int i = longest - 1; i >= 0; i--) {
    lis[i] = sequence[k];
    k = predecessors[k];
  }
  return lis;
}

bool RenderMultiChildParent::Update(shared_ptr<Node> configPtr) {
  assert(configPtr != nullptr);

  if (!dynamic_pointer_cast<MultiChildNode>(configPtr)) {
    return false;
  }

  auto newConfiguration = static_pointer_cast<MultiChildNode>(configPtr);
  auto oldConfiguration = static_pointer_cast<MultiChildNode>(GetConfiguration());

  if (oldConfiguration == newConfiguration) {
    // No need to diff child lists.
    if (GetHasDescendantsNeedingUpdate()) {
      for (auto child : _currentChildren) {
        child->Update(child->GetConfiguration());
      }
    }
    return RenderParent::Update(configPtr);
  }

  ChildListDiff diff;
  vector<shared_ptr<Node>> newChildren = newConfiguration->GetChildren();

  // A tuple with tracking information about a child node
  using TrackedChild = tuple<
    vector<shared_ptr<RenderNode>>::iterator,  // points to _currentChildren
    int,  // index of the child in _currentChildren
    bool  // whether the child pointed to should be retained
  >;

  // A vector of tracked children.
  using TrackedChildren = vector<TrackedChild>;

  TrackedChildren currentChildren;
  map<string, TrackedChildren::iterator> keyMap;

  for (auto iter = _currentChildren.begin(); iter != _currentChildren.end(); iter++) {
    auto node = *iter;
    currentChildren.push_back({
        iter,
        (int) (iter - _currentChildren.begin()),
        false,
    });
    auto trackedChildIter = currentChildren.end() - 1;
    shared_ptr<Node> config = node->GetConfiguration();
    auto key = config->GetKey();
    if (key != nullptr) {
      keyMap[key->GetValue()] = trackedChildIter;
    }
  }

  vector<int> sequence;
  vector<tuple<shared_ptr<Node>, int>> targetList;
  //           |                 |
  //           node              base index (or -1)

  for (auto iter = newChildren.begin(); iter != newChildren.end(); iter++) {
    shared_ptr<Node> node = *iter;
    auto key = node->GetKey();
    int baseIndex = -1;
    if (key != nullptr) {
      auto baseEntry = keyMap.find(key->GetValue());
      if (baseEntry != keyMap.end()) {
        TrackedChild baseChild = *(baseEntry->second);
        baseIndex = get<1>(baseChild);
        get<2>(baseChild) = true;
        sequence.push_back(baseIndex);
      }
    }
    targetList.push_back({node, baseIndex});
  }

  // Compute removes
  for (auto i = currentChildren.begin(); i != currentChildren.end(); i++) {
    if (!get<2>(*i)) {
      diff.Remove((int) (i - currentChildren.begin()));
    }
  }

  // Compute inserts
  vector<int> lis = ComputeLongestIncreasingSubsequence(sequence);
  auto insertionPoint = lis.begin();
  vector<shared_ptr<RenderNode>> newChildVector;
  int baseCount = (int) _currentChildren.size();
  for (auto targetEntry = targetList.begin(); targetEntry != targetList.end(); targetEntry++) {
    // Three possibilities:
    //   - it's a new child => its base index == -1
    //   - it's a moved child => its base index != -1 && base index != insertion index
    //   - it's a noop child => its base index != -1 && base index == insertion index

    // Index in the base list of the moved child, or -1
    int baseIndex = get<1>(*targetEntry);
    // Index in the base list before which target child must be inserted.
    int insertionIndex = baseCount;
    if (insertionPoint != lis.end()) {
      insertionIndex = *insertionPoint;
      insertionPoint++;
    }

    if (baseIndex == -1) {
      // New child
      shared_ptr<Node> childNode = get<0>(*targetEntry);

      // Lock the diff object so child nodes do not push diffs.
      bool wasLocked = GetHtmlDiff()->IsLocked();
      GetHtmlDiff()->Lock();
      auto childRenderNode = childNode->Instantiate(GetTree());
      newChildVector.push_back(childRenderNode);
      childRenderNode->Update(childNode);
      childRenderNode->Attach(shared_from_this());
      if (!wasLocked) {
        string html;
        childRenderNode->PrintHtml(html);
        diff.NewChild(insertionIndex, html);
        GetHtmlDiff()->Unlock();
      }
    } else if (baseIndex != insertionIndex) {
      // Moved child
      diff.Move(insertionIndex, baseIndex);
      newChildVector.push_back(_currentChildren[baseIndex]);
    } else {
      newChildVector.push_back(_currentChildren[baseIndex]);
    }
  }
  _currentChildren = newChildVector;

  if (GetHtmlDiff()->IsNotLocked()) {
    diff.Render();
  }

  return RenderParent::Update(configPtr);
}

}
