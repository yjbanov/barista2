//
// Created by Yegor Jbanov on 9/5/16.
//

#include <string>
#include <iostream>

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

bool RenderElement::CanUpdateUsing(shared_ptr<Node> newConfiguration) {
  assert(newConfiguration != nullptr);
  assert(GetConfiguration() != nullptr);
  // TODO: figure out why dynamic_pointer_cast doesn't work.
  if (!dynamic_cast<Element*>(newConfiguration.get())) {
    return false;
  }

  shared_ptr<Element> element = static_pointer_cast<Element>(newConfiguration);
  if (GetConfiguration() != nullptr) {
    shared_ptr<Element> config = static_pointer_cast<Element>(GetConfiguration());
    if (config->GetTag() != element->GetTag()) {
      return false;
    }
  }
  return true;
}

void RenderElement::Update(shared_ptr<Node> configPtr, ElementUpdate& update) {
  // TODO: figure out why dynamic_pointer_cast doesn't work
  assert(dynamic_cast<Element*>(configPtr.get()) != nullptr);

  shared_ptr<Element> newConfiguration = static_pointer_cast<Element>(configPtr);
  shared_ptr<Element> oldConfiguration = static_pointer_cast<Element>(GetConfiguration());
  if (GetConfiguration() != nullptr) {
    shared_ptr<Element> config = static_pointer_cast<Element>(GetConfiguration());
    if (oldConfiguration->_text != newConfiguration->_text) {
      update.SetText(newConfiguration->_text);
    }
  } else {
    update.SetTag(newConfiguration->GetTag());
    auto key = newConfiguration->GetKey();
    if (key != nullptr) {
      update.SetKey(key->GetValue());
    }
    update.SetText(newConfiguration->_text);
  }

  RenderMultiChildParent::Update(configPtr, update);
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
  if (conf->_text != "") {
    buf += conf->_text;
  }
  RenderMultiChildParent::PrintHtml(buf);
  buf += "</" + conf->_tag + ">";
}

shared_ptr<Element> El(string tag) {
  return make_shared<Element>(tag);
}

shared_ptr<Element> Tx(string value) {
  auto span = make_shared<Element>("span");
  span->SetText(value);
  return span;
}

} // namespace barista
