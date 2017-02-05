//
// Created by Yegor Jbanov on 9/4/16.
//

#include "api.h"
#include "sync.h"

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

RenderNode::RenderNode(shared_ptr<Tree> tree) : _tree(tree) { }

void RenderNode::Update(shared_ptr<Node> newConfiguration, ElementUpdate& update) {
  assert(newConfiguration != nullptr);
  _configuration = newConfiguration;
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

void RenderParent::Update(shared_ptr<Node> newConfiguration, ElementUpdate& update) {
  _hasDescendantsNeedingUpdate = false;
  RenderNode::Update(newConfiguration, update);
}

string Tree::RenderFrame(int indent) {
  auto treeUpdate = TreeUpdate();

  if (_topLevelNode == nullptr) {
    _topLevelNode = _topLevelWidget->Instantiate(shared_from_this());
    auto& rootInsertion = treeUpdate.CreateRootElement();
    _topLevelNode->Update(_topLevelWidget, rootInsertion);
  } else {
    auto& rootUpdate = treeUpdate.UpdateRootElement();
    _topLevelNode->Update(_topLevelWidget, rootUpdate);
  }

  return treeUpdate.Render(indent);
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

bool RenderStatelessWidget::CanUpdateUsing(shared_ptr<Node> newConfiguration) {
  assert(newConfiguration != nullptr);
  auto oldConfiguration = GetConfiguration();
  assert(oldConfiguration != nullptr);
  return typeid(oldConfiguration) == typeid(newConfiguration);
}

void RenderStatelessWidget::Update(shared_ptr<Node> configPtr, ElementUpdate& update) {
  assert(dynamic_cast<StatelessWidget*>(configPtr.get()));
  auto newConfiguration = static_pointer_cast<StatelessWidget>(configPtr);

  if (GetConfiguration() != newConfiguration) {
    // Build the new configuration and decide whether to reuse the child node
    // or replace with a new one.
    shared_ptr<Node> newChildConfiguration = newConfiguration->Build();
    if (_child != nullptr && _canUpdate(_child, newChildConfiguration)) {
      _child->Update(newChildConfiguration, update);
    } else {
      // Replace child
      if (_child != nullptr) {
        _child->Detach();
      }
      _child = newChildConfiguration->Instantiate(GetTree());
      _child->Update(newChildConfiguration, update);
      _child->Attach(shared_from_this());
    }
  } else if (GetHasDescendantsNeedingUpdate()) {
    assert(_child != nullptr);
    // Own configuration is the same, but some children are scheduled to be
    // updated.
    _child->Update(_child->GetConfiguration(), update);
  }

  RenderParent::Update(newConfiguration, update);
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

bool RenderStatefulWidget::CanUpdateUsing(shared_ptr<Node> newConfiguration) {
  assert(newConfiguration != nullptr);
  auto oldConfiguration = GetConfiguration();
  assert(oldConfiguration != nullptr);
  return typeid(oldConfiguration) == typeid(newConfiguration);
}

void RenderStatefulWidget::Update(shared_ptr<Node> configPtr, ElementUpdate& update) {
  assert(dynamic_cast<StatefulWidget*>(configPtr.get()));
  auto newConfiguration = static_pointer_cast<StatefulWidget>(configPtr);

  if (GetConfiguration() != newConfiguration) {
    // Build the new configuration and decide whether to reuse the child node
    // or replace with a new one.
    _state = newConfiguration->CreateState();
    _state->_config = newConfiguration;
    internalSetStateNode(_state, shared_from_this());
    shared_ptr<Node> newChildConfiguration = _state->Build();
    if (_child != nullptr && _sameType(newChildConfiguration.get(), _child->GetConfiguration().get())) {
      _child->Update(newChildConfiguration, update);
    } else {
      if (_child != nullptr) {
        _child->Detach();
      }
      _child = newChildConfiguration->Instantiate(GetTree());
      _child->Update(newChildConfiguration, update);
      _child->Attach(shared_from_this());
    }
  } else if (_isDirty) {
    _child->Update(_state->Build(), update);
  } else if (GetHasDescendantsNeedingUpdate()) {
    // Own configuration is the same, but some children are scheduled to be
    // updated.
    _child->Update(_child->GetConfiguration(), update);
  }

  _isDirty = false;
  RenderParent::Update(newConfiguration, update);
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

void RenderMultiChildParent::Update(shared_ptr<Node> configPtr, ElementUpdate& update) {
  assert(dynamic_cast<MultiChildNode*>(configPtr.get()));
  auto newConfiguration = static_pointer_cast<MultiChildNode>(configPtr);

  if (GetConfiguration() != nullptr) {
    assert(dynamic_cast<MultiChildNode*>(GetConfiguration().get()));
  }
  auto oldConfiguration = static_pointer_cast<MultiChildNode>(GetConfiguration());

  if (oldConfiguration == newConfiguration) {
    // No need to diff child lists.
    if (GetHasDescendantsNeedingUpdate()) {
      for (auto child = _currentChildren.begin(); child != _currentChildren.end(); child++) {
        auto& childUpdate = update.UpdateChildElement(child - _currentChildren.begin());
        (*child)->Update((*child)->GetConfiguration(), childUpdate);
      }
    }
    RenderParent::Update(configPtr, update);
    return;
  }

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
    if (key != "") {
      keyMap[key] = trackedChildIter;
    }
  }

  vector<int> sequence;
  vector<tuple<shared_ptr<Node>, int>> targetList;
  //           |                 |
  //           node              base index (or -1)

  TrackedChildren::iterator afterLastUsedUnkeyedChild = currentChildren.begin();
  for (auto iter = newChildren.begin(); iter != newChildren.end(); iter++) {
    shared_ptr<Node> node = *iter;
    auto key = node->GetKey();
    TrackedChildren::iterator baseChild = currentChildren.end();
    if (key != "") {
      auto baseEntry = keyMap.find(key);
      if (baseEntry != keyMap.end()) {
        baseChild = baseEntry->second;
      }
    } else {
      // Start with afterLastUsedUnkeyedChild and scan until the first child
      // we can update. Use it. This approach is naive. It does not support
      // swaps, for example. It does support removes though. For swaps, the
      // developer is expected to use keys anyway.
      TrackedChildren::iterator scanner = afterLastUsedUnkeyedChild;
      while(scanner != currentChildren.end()) {
        shared_ptr<RenderNode> currentChild = *(get<0>(*scanner));
        if (currentChild->CanUpdateUsing(node)) {
          auto& childUpdate = update.UpdateChildElement(get<1>(*scanner));
          currentChild->Update(node, childUpdate);
          baseChild = scanner;
          afterLastUsedUnkeyedChild = scanner + 1;
          break;
        }
        scanner++;
      }
    }

    int baseIndex = -1;

    if (baseChild != currentChildren.end()) {
      baseIndex = get<1>(*baseChild);
      get<2>(*baseChild) = true;
      sequence.push_back(baseIndex);
    }

    targetList.push_back({node, baseIndex});
  }

  // Compute removes
  for (auto i = currentChildren.begin(); i != currentChildren.end(); i++) {
    if (!get<2>(*i)) {
      update.RemoveChild((int) (i - currentChildren.begin()));
    }
  }

  // Compute inserts and updates
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
      auto& childInsertion = update.InsertChildElement(insertionIndex);
      auto childRenderNode = childNode->Instantiate(GetTree());
      newChildVector.push_back(childRenderNode);
      childRenderNode->Update(childNode, childInsertion);
      childRenderNode->Attach(shared_from_this());
    } else if (baseIndex != insertionIndex) {
      // Moved child
      update.MoveChild(insertionIndex, baseIndex);
      newChildVector.push_back(_currentChildren[baseIndex]);
    } else {
      newChildVector.push_back(_currentChildren[baseIndex]);
    }
  }
  _currentChildren = newChildVector;

  RenderParent::Update(configPtr, update);
}

}
