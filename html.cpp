//
// Created by Yegor Jbanov on 9/5/16.
//

#include "api.h"
#include "html.h"

namespace barista {

shared_ptr<RenderNode> Element::Instantiate(shared_ptr<Tree> tree) {
  return make_shared<RenderElement>(tree, shared_from_this());
}

void RenderElement::Update(shared_ptr<Element> newConfiguration) {
  RenderMultiChildParent::Update(newConfiguration);
}

} // namespace barista
