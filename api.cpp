//
// Created by Yegor Jbanov on 9/4/16.
//

#include "api.h"

#include <cassert>
#include <memory>

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

RenderNode::RenderNode(shared_ptr<Tree> tree, shared_ptr<Node> configuration) : _tree(tree), _configuration(configuration) {
  Update(configuration);
}

void RenderNode::Update(shared_ptr<Node> newConfiguration) {
  assert(newConfiguration != NULL);
  _configuration = newConfiguration;
}


RenderParent::RenderParent(shared_ptr<Tree> tree, shared_ptr<Node> configuration)
  : RenderNode(tree, configuration) {}

void RenderParent::ScheduleUpdate() {
  _hasDescendantsNeedingUpdate = true;
  shared_ptr<RenderParent> parent = GetParent();
  while (parent != NULL) {
    parent->_hasDescendantsNeedingUpdate = true;
    parent = parent->GetParent();
  }
}

void RenderParent::Update(shared_ptr<Node> newConfiguration) {
  _hasDescendantsNeedingUpdate = false;
  RenderNode::Update(newConfiguration);
}

void Tree::RenderFrame() {
  if (_topLevelNode == NULL) {
    _topLevelNode = _topLevelWidget->Instantiate(shared_from_this());
  } else {
    _topLevelNode->Update(_topLevelNode->GetConfiguration());
  }
}

void Tree::VisitChildren(RenderNodeVisitor visitor) {
  visitor(_topLevelNode);
}

shared_ptr<RenderNode> StatelessWidget::Instantiate(shared_ptr<Tree> tree) {
  return make_shared<RenderStatelessWidget>(tree, shared_from_this());
}

shared_ptr<RenderNode> StatefulWidget::Instantiate(shared_ptr<Tree> t) {
  return make_shared<RenderStatefulWidget>(t, shared_from_this());
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

void RenderStatelessWidget::Update(shared_ptr<StatelessWidget> newConfiguration) {
  assert(newConfiguration != NULL);
  if (GetConfiguration() != newConfiguration) {
    // Build the new configuration and decide whether to reuse the child node
    // or replace with a new one.
    shared_ptr<Node> newChildConfiguration = newConfiguration->Build();
    if (_child != NULL && _canUpdate(_child, newChildConfiguration)) {
      _child->Update(newChildConfiguration);
    } else {
      // Replace child
      if (_child != NULL) {
        _child->Detach();
      }
      _child = newChildConfiguration->Instantiate(GetTree());
      _child->Attach(shared_from_this());
    }
  } else if (GetHasDescendantsNeedingUpdate()) {
    // Own configuration is the same, but some children are scheduled to be
    // updated.
    _child->Update(_child->GetConfiguration());
  }
  Update(newConfiguration);
}

void RenderStatefulWidget::Update(shared_ptr<StatefulWidget> newConfiguration) {
  assert(newConfiguration != nullptr);
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
  Update(newConfiguration);
}

}
