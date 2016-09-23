//
// Created by Yegor Jbanov on 9/4/16.
//

#include "api.h"

#include <cassert>
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

void Tree::RenderFrame() {
  if (_topLevelNode == nullptr) {
    _topLevelNode = _topLevelWidget->Instantiate(shared_from_this());
  }

  _topLevelNode->Update(_topLevelWidget);
}

void Tree::VisitChildren(RenderNodeVisitor visitor) {
  visitor(_topLevelNode);
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

void RenderStatefulWidget::VisitChildren(RenderNodeVisitor visitor) {
  visitor(_child);
}

void RenderStatefulWidget::ScheduleUpdate() {
  _isDirty = true;
  RenderParent::ScheduleUpdate();
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
    renderNode->Update(node);
    renderNode->Attach(shared_from_this());
    _currentChildren.push_back(renderNode);
  }
}

void RenderMultiChildParent::Update(shared_ptr<Node> configPtr) {
  assert(configPtr != nullptr);
  auto newConfiguration = static_pointer_cast<MultiChildNode>(configPtr);
  if (GetConfiguration() != newConfiguration) {
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
      // Both are not empty
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
  } else if (GetHasDescendantsNeedingUpdate()) {
    for (auto child : _currentChildren) {
      child->Update(child->GetConfiguration());
    }
  }
  RenderParent::Update(configPtr);
}

}
