//
// Created by Yegor Jbanov on 9/4/16.
//

#include "api.h"

#include <memory>
#include <cassert>
#include <vector>

using namespace std;

namespace barista {

void RenderNode::DispatchEvent(ref<Event> event) {
  if (_parent != NULL) {
    _parent->DispatchEvent(event);
  }
}

RenderParent::RenderParent(ref<Tree> tree, ref<Node> configuration)
  : RenderNode(tree, configuration) {}

void RenderParent::ScheduleUpdate() {
  _hasDescendantsNeedingUpdate = true;
  ref<RenderParent> parent = GetParent();
  while (parent != NULL) {
    parent->_hasDescendantsNeedingUpdate = true;
    parent = parent->GetParent();
  }
}

void RenderParent::Update(ref<Node> newConfiguration) {
  _hasDescendantsNeedingUpdate = false;
  RenderNode::Update(newConfiguration);
}

}