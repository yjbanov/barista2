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
  Element(string tag) : MultiChildNode(), _tag(tag) {}
  shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> tree);
  string GetTag() { return _tag; }
  map<string, string>::iterator GetAttributes() { return _attributes.begin(); };
  // TODO: rename to AddAttribute.
  void SetAttribute(string name, string value) { _attributes[name] = value; }
  void AddEventListener(string type, EventListener listener);
  void AddStyle(Style &style) { AddClassName(style.GetIdentifierClass()); }
  void AddClassName(string className) { _classNames.push_back(className); }
  void SetText(string text) { _text = text; }

 private:
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

  // Text inside the element.
  string _text = "";

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
  virtual bool CanUpdateUsing(shared_ptr<Node> newConfiguration);
  virtual void Update(shared_ptr<Node> newConfiguration, ElementUpdate& update);
  virtual void DispatchEvent(string type, string baristaId);

  static void DangerouslyResetBaristaIdCounterForTesting() { _bidCounter = 1; }
 private:
  // Monotonically increasing element ID counter.
  static int64_t _bidCounter;
};

// A little boilerplate-reducing DSL
shared_ptr<Element> El(string tag);
shared_ptr<Element> Tx(string value);

}

#endif //BARISTA2_HTML_H
