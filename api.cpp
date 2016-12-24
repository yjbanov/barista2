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

void RenderNode::Update(shared_ptr<Node> newConfiguration) {
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

void RenderParent::Update(shared_ptr<Node> newConfiguration) {
  _hasDescendantsNeedingUpdate = false;
  RenderNode::Update(newConfiguration);
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

void RenderStatelessWidget::Update(shared_ptr<Node> configPtr) {
  assert(configPtr != nullptr);
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
  RenderParent::Update(newConfiguration);
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

void RenderStatefulWidget::Update(shared_ptr<Node> configPtr) {
  assert(configPtr != nullptr);
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
  RenderParent::Update(newConfiguration);
}

void RenderMultiChildParent::VisitChildren(RenderNodeVisitor visitor) {
  for (auto child : _currentChildren) {
    visitor(child);
  }
}

void RenderMultiChildParent::_appendChildren(
    vector<shared_ptr<Node>>::iterator from,
    vector<shared_ptr<Node>>::iterator to) {
  assert((to - from) > 0);
  for (auto nodeiter = from; nodeiter != to; nodeiter++) {
    shared_ptr<Node> node = *nodeiter;
    shared_ptr<RenderNode> renderNode = node->Instantiate(GetTree());
    GetHtmlDiff()->Move(_currentChildren.size());
    renderNode->Update(node);
    renderNode->Attach(shared_from_this());
    _currentChildren.push_back(renderNode);
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

void RenderMultiChildParent::Update(shared_ptr<Node> configPtr) {
  assert(configPtr != nullptr);
  auto newConfiguration = static_pointer_cast<MultiChildNode>(configPtr);
  auto oldConfiguration = static_pointer_cast<MultiChildNode>(GetConfiguration());

  if (oldConfiguration == newConfiguration) {
    // No need to diff child lists.
    if (GetHasDescendantsNeedingUpdate()) {
      for (auto child : _currentChildren) {
        child->Update(child->GetConfiguration());
      }
    }
    RenderParent::Update(configPtr);
    return;
  }

  ChildListDiff diff;
  vector<shared_ptr<Node>> newChildren = newConfiguration->GetChildren();
  bool performPairWiseDiffing = false;
  int baseCount = (int) _currentChildren.size();
  map<string, tuple<shared_ptr<RenderNode>, int, bool>> baseMap;
  //  |             |                       |    |
  //  key           render node             |    keep?
  //                                        base index

  for (auto iter = _currentChildren.begin(); iter != _currentChildren.end(); iter++) {
    auto node = *iter;
    auto key = node->GetConfiguration()->GetKey();
    if (key == nullptr) {
      // Not all children are keyed; default to pair-wise diffing
      performPairWiseDiffing = true;
      break;
    }
    baseMap[key->GetValue()] = {
        node,
        (int) (iter - _currentChildren.begin()),
        false, // assume it's removed, unless we find it later in the target list
    };
  }

  vector<int> sequence;
  if (!performPairWiseDiffing) {
    vector<tuple<shared_ptr<Node>, int>> targetList;
    //           |                 |
    //           node              base index (or -1)

    for (auto iter = newChildren.begin(); iter != newChildren.end(); iter++) {
      shared_ptr<Node> node = *iter;
      auto key = node->GetKey();
      if (key == nullptr) {
        // Not all children are keyed; default to pair-wise diffing
        performPairWiseDiffing = true;
        break;
      }
      auto baseEntry = baseMap.find(key->GetValue());
      int baseIndex = -1;
      if (baseEntry != baseMap.end()) {
        baseIndex = get<1>(baseEntry->second);
        get<2>(baseEntry->second) = true;
        sequence.push_back(baseIndex);
      }
      targetList.push_back({node, baseIndex});
    }

    if (!performPairWiseDiffing) {
      // Compute removes
      for (auto i = _currentChildren.begin(); i != _currentChildren.end(); i++) {
        auto baseChild = *i;
        auto tuple = baseMap[baseChild->GetConfiguration()->GetKey()->GetValue()];
        if (!get<2>(tuple)) {
          diff.Remove(get<1>(tuple));
        }
      }

      // Compute inserts
      vector<int> lis = ComputeLongestIncreasingSubsequence(sequence);
      auto insertionPoint = lis.begin();
      vector<shared_ptr<RenderNode>> newChildVector;
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
    }
  }

  if (performPairWiseDiffing) {
    _pairWiseDiff(oldConfiguration, newConfiguration);
  }

  if (GetHtmlDiff()->IsNotLocked()) {
    diff.Render();
  }

  RenderParent::Update(configPtr);
}

/// When child elements are not keyed, do a naive pair-wise scan and try matching by type.
void RenderMultiChildParent::_pairWiseDiff(
    shared_ptr<MultiChildNode> oldConfiguration,
    shared_ptr<MultiChildNode> newConfiguration) {
  vector<shared_ptr<Node>> newChildList = newConfiguration->GetChildren();

  if (_currentChildren.empty()) {
    if (!newChildList.empty()) {
      _appendChildren(newChildList.begin(), newChildList.end());
    }
  } else if (newChildList.empty()) {
    if (!_currentChildren.empty()) {
      _removeAllCurrentChildren();
    }
  } else {
    // TODO: switch from indexes to iterators.
    long from = 0;
    while(from < _currentChildren.size() &&
        from < newChildList.size() &&
        _canUpdate(_currentChildren[from], newChildList[from])) {
      _currentChildren[from]->Update(newChildList[from]);
      from++;
    }

    if (from == _currentChildren.size()) {
      if (from < newChildList.size()) {
        // More children were added at the end, append them
        _appendChildren(newChildList.begin() + from, newChildList.end());
      }
    } else if (from == newChildList.size()) {
      // Some children at the end were removed, remove them
      for (auto i = _currentChildren.size() - 1; i >= from; i--) {
        _currentChildren[i]->Detach();
      }
      _currentChildren.erase(_currentChildren.begin() + from, _currentChildren.end());
    } else {
      // Walk lists from the end and try to update as much as possible
      auto currTo = _currentChildren.size();
      auto newTo = newChildList.size();
      while(currTo > from &&
          newTo > from &&
          _canUpdate(_currentChildren[currTo - 1], newChildList[newTo - 1])) {
        _currentChildren[currTo - 1]->Update(newChildList[newTo - 1]);
        currTo--;
        newTo--;
      }

      if (newTo == from && currTo > from) {
        // Some children in the middle were removed, remove them
        for (auto i = currTo - 1; i >= from; i--) {
          _currentChildren[i]->Detach();
        }
        _currentChildren.erase(_currentChildren.begin() + from, _currentChildren.begin() + currTo);
      } else if (currTo == from && newTo > from) {
        // New children were inserted in the middle, insert them
        vector<shared_ptr<Node>> newChildren(newChildList.begin() + from, newChildList.begin() + newTo);

        vector<shared_ptr<RenderNode>> insertedChildren;
        for (shared_ptr<Node> vn : newChildren) {
          auto child = vn->Instantiate(GetTree());
          child->Update(vn);
          child->Attach(shared_from_this());
          insertedChildren.push_back(child);
        }

        _currentChildren.insert(_currentChildren.begin() + from, insertedChildren.begin(), insertedChildren.end());
      } else {
        // We're strictly in the middle of both lists, at which point nodes
        // moved around, were added, or removed. If the nodes are keyed, map
        // them by key and figure out all the moves.

        // TODO: this implementation is very naive; it plucks _all_ children
        // and rearranges them according to new configuration. A smarter
        // implementation would compute the minimum sufficient number of
        // moves to transform the tree into the desired configuration.

        vector<shared_ptr<RenderNode>> disputedRange;
        for (long i = from; i < currTo; i++) {
          auto child = _currentChildren[i];
          child->Detach();
          disputedRange.push_back(child);
        }

        vector<shared_ptr<RenderNode>> newRange;
        for (long i = from; i < newTo; i++) {
          auto newChild = newChildList[i];
          // First try to fing an existing node that could be updated
          bool updated = false;
          for (auto child : disputedRange) {
            if (_canUpdate(child, newChild)) {
              child->Update(newChild);
              child->Attach(shared_from_this());
              updated = true;
              newRange.push_back(child);
              break;
            }
          }

          if (!updated) {
            auto child = newChild->Instantiate(GetTree());
            child->Update(newChild);
            child->Attach(shared_from_this());
            newRange.push_back(child);
          }
        }

        auto oldChildren = _currentChildren;
        _currentChildren = vector<shared_ptr<RenderNode>>();
        _currentChildren.insert(_currentChildren.end(), oldChildren.begin(), oldChildren.begin() + from);
        _currentChildren.insert(_currentChildren.end(), newRange.begin(), newRange.end());
        _currentChildren.insert(_currentChildren.end(), oldChildren.begin() + currTo, oldChildren.end());
        assert(_currentChildren.size() == newChildList.size());
      }
    }
  }
}

}
