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

void Element::AddEventListener(string type, EventListener listener) {
  _eventListeners.push_back(EventListenerConfig(type, listener));
}

int64_t RenderElement::_bidCounter = 1;

bool RenderElement::CanUpdateUsing(shared_ptr<Node> newConfiguration) {
  assert(newConfiguration != nullptr);
  assert(GetConfiguration() != nullptr);
  // TODO: figure out why dynamic_pointer_cast doesn't work.
  if (!dynamic_cast<Element*>(newConfiguration.get())) {
    return false;
  }

  shared_ptr<Element> element = static_pointer_cast<Element>(newConfiguration);
  if (GetConfiguration() != nullptr) {
    assert(dynamic_cast<Element*>(GetConfiguration().get()));
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

  if (GetConfiguration() != nullptr) {
    assert(dynamic_cast<Element*>(GetConfiguration().get()));
  }
  shared_ptr<Element> oldConfiguration = static_pointer_cast<Element>(GetConfiguration());

  if (oldConfiguration != nullptr) {
    if (oldConfiguration->_text != newConfiguration->_text) {
      update.SetText(newConfiguration->_text);
    }
    if (newConfiguration->_eventListeners.size() > 0) {
      if (oldConfiguration->_bid != "") {
        newConfiguration->_bid = oldConfiguration->_bid;
      } else {
        newConfiguration->_bid = to_string(_bidCounter++);
        update.SetBaristaId(newConfiguration->_bid);
      }
    }
    auto& newAttrs = newConfiguration->_attributes;
    auto& oldAttrs = oldConfiguration->_attributes;
    if (newAttrs != oldAttrs) {
      // Find updates
      for (auto attr = newAttrs.begin(); attr != newAttrs.end(); attr++) {
        auto name = attr->first;
        auto newValue = attr->second;
        auto oldValueIter = oldAttrs.find(name);
        if (oldValueIter == oldAttrs.end() || newValue != oldValueIter->second) {
          update.SetAttribute(attr->first, attr->second);
        }
      }

      // Find removes
      for (auto attr = oldAttrs.begin(); attr != oldAttrs.end(); attr++) {
        auto name = attr->first;
        if (newAttrs.find(name) == newAttrs.end()) {
          update.SetAttribute(attr->first, "");
        }
      }
    }
  } else {
    update.SetTag(newConfiguration->GetTag());
    auto key = newConfiguration->GetKey();
    if (key != nullptr) {
      update.SetKey(key->GetValue());
    }
    if (newConfiguration->_bid != "") {
      update.SetBaristaId(newConfiguration->_bid);
    }
    if (newConfiguration->_eventListeners.size() > 0) {
      newConfiguration->_bid = to_string(_bidCounter++);
      update.SetBaristaId(newConfiguration->_bid);
    }
    update.SetText(newConfiguration->_text);
    if (newConfiguration->_attributes.size() > 0) {
      auto first = newConfiguration->_attributes.begin();
      auto last = newConfiguration->_attributes.end();
      for (auto attr = first; attr != last; attr++) {
        update.SetAttribute(attr->first, attr->second);
      }
    }
  }

  RenderMultiChildParent::Update(configPtr, update);
}

void RenderElement::DispatchEvent(string type, string baristaId) {
  assert(dynamic_cast<Element*>(GetConfiguration().get()));
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
  assert(dynamic_cast<Element*>(GetConfiguration().get()));
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
