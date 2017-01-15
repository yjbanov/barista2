//
// Created by Yegor Jbanov on 9/5/16.
//

#ifndef BARISTA2_HTML_H
#define BARISTA2_HTML_H

#include <functional>
#include <map>
#include <memory>
#include <sstream>

#include "api.h"
#include "style.h"

namespace barista {

using namespace std;

// TODO: define an Event interface and pass it to EventListener.
typedef function<void()> EventListener;

class Element : public MultiChildNode, public enable_shared_from_this<Element> {
 public:
  Element(string tag) : _tag(tag), MultiChildNode() {}
  shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> tree);
  string GetTag() { return _tag; }
  map<string, string>::iterator GetAttributes() { return _attributes.begin(); };
  // TODO: rename to AddAttribute.
  void SetAttribute(string name, string value) { _attributes[name] = value; }
  void AddEventListener(string type, EventListener listener);
  void AddStyle(Style &style) { AddClassName(style.GetIdentifierClass()); }
  void AddClassName(string className) { _classNames.push_back(className); }

 private:
  // Monotonically increasing element ID counter.
  static int64_t _bidCounter;

  class EventListenerConfig {
   private:
    EventListenerConfig(string type, EventListener callback) :
      _type(type), _callback(callback) { }
    string _type;
    EventListener _callback;
    friend class Element;
    friend class RenderElement;
  };

  // HTML tag, e.g. "div", "button".
  string _tag;

  // HTML attributes, e.g. "id".
  map<string, string> _attributes;

  // User-defined CSS class names.
  vector<string> _classNames;

  // Barista ID.
  //
  // Used to uniquely identify this element when dispatching events.
  string _bid = "";

  // HTML event listeners, e.g. a click listener.
  vector<EventListenerConfig> _eventListeners;

  friend class RenderElement;
};

class RenderElement : public RenderMultiChildParent {
 public:
  RenderElement(shared_ptr<Tree> tree) : RenderMultiChildParent(tree) {}
  bool Update(shared_ptr<Node> newConfiguration);
  virtual void DispatchEvent(string type, string baristaId);
  virtual void PrintHtml(string &buf);
};

class Text : public Node {
 public:
  Text(string value) : _value(value), Node() {}
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> tree);
  string GetValue() { return _value; }

 private:
  string _value;
};

class RenderText : public RenderNode {
 public:
  RenderText(shared_ptr<Tree> tree) : RenderNode(tree) {}
  virtual void VisitChildren(RenderNodeVisitor visitor) {}
  virtual void DispatchEvent(string type, string baristaId) {}
  virtual void PrintHtml(string &buf) {
    auto conf = static_pointer_cast<Text>(GetConfiguration());
    buf += conf->GetValue();
  }
};

// A little boilerplate-reducing DSL
shared_ptr<Element> El(string tag);
shared_ptr<Text> Tx(string value);

}

#endif //BARISTA2_HTML_H
