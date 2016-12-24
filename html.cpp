//
// Created by Yegor Jbanov on 9/5/16.
//

#include <string>

#include "api.h"
#include "html.h"

namespace barista {

shared_ptr<RenderNode> Element::Instantiate(shared_ptr<Tree> tree) {
  return make_shared<RenderElement>(tree);
}

int64_t Element::_bidCounter = 1;

void Element::AddEventListener(string type, EventListener listener) {
  _bid = to_string(_bidCounter++);
  _eventListeners.push_back(EventListenerConfig(type, listener));
}

void RenderElement::Update(shared_ptr<Node> configPtr) {
  RenderMultiChildParent::Update(configPtr);
}

void RenderElement::DispatchEvent(string type, string baristaId) {
  shared_ptr<Element> config = static_pointer_cast<Element>(GetConfiguration());
  if (config->_bid == baristaId) {
    for (auto listener : config->_eventListeners) {
      if (listener._type == type) {
        listener._callback();
      }
    }
  } else {
    VisitChildren([type, baristaId](shared_ptr<RenderNode> child) {
      child->DispatchEvent(type, baristaId);
    });
  }
}

void RenderElement::PrintHtml(string &buf) {
  auto conf = static_pointer_cast<Element>(GetConfiguration());
  buf += "<" + conf->_tag;
  if (conf->_attributes.size() > 0) {
    auto first = conf->_attributes.begin();
    auto last = conf->_attributes.end();
    for (auto attr = first; attr != last; attr++) {
      buf += " " + attr->first + "=\"" + attr->second + "\"";
    }
  }
  if (conf->_bid != "") {
    buf += " _bid=\"" + conf->_bid + "\"";
  }
  if (conf->GetKey() != nullptr) {
    buf += " _bkey=\"" + conf->GetKey()->GetValue() + "\"";
  }
  if (conf->_classNames.size() > 0) {
    buf += " class=\"";
    auto classNames = conf->_classNames;
    buf += classNames[0];
    for (auto i = classNames.begin() + 1; i != classNames.end(); i++) {
      buf += " " + *i;
    }
    buf += "\"";
  }
  buf += ">";
  RenderMultiChildParent::PrintHtml(buf);
  buf += "</" + conf->_tag + ">";
}

shared_ptr<RenderNode> Text::Instantiate(shared_ptr<Tree> tree) {
  return make_shared<RenderText>(tree);
}

shared_ptr<Element> El(string tag) {
  return make_shared<Element>(tag);
}

shared_ptr<Text> Tx(string value) {
  return make_shared<Text>(value);
}

} // namespace barista
