//
// Created by Yegor Jbanov on 9/5/16.
//

#include "api.h"
#include "html.h"

namespace barista {

shared_ptr<RenderNode> Element::Instantiate(shared_ptr<Tree> tree) {
  return make_shared<RenderElement>(tree);
}

void RenderElement::Update(shared_ptr<Node> configPtr) {
  shared_ptr<Element> newConfiguration = static_pointer_cast<Element>(configPtr);
  RenderMultiChildParent::Update(configPtr);
}

shared_ptr<RenderNode> Text::Instantiate(shared_ptr<Tree> tree) {
  return make_shared<RenderText>(tree);
}

} // namespace barista
