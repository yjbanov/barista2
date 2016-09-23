//
// Created by Yegor Jbanov on 9/5/16.
//

#ifndef BARISTA2_HTML_H
#define BARISTA2_HTML_H

#include <map>
#include <memory>

#include "api.h"

namespace barista {

using namespace std;

class Element : public MultiChildNode, public enable_shared_from_this<Element> {
 public:
  Element(string tag) : _tag(tag), MultiChildNode() {}
  shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> t);
  string GetTag() { return _tag; }
  map<string, string>::iterator GetAttributes() { return _attributes.begin(); };
  void SetAttribute(string name, string value) { _attributes[name] = value; }
  void RemoveAttribute(string name) { _attributes.erase(name); }

 private:
  string _tag;
  map<string, string> _attributes;
  friend class RenderElement;
};

class RenderElement : public RenderMultiChildParent {
 public:
  RenderElement(shared_ptr<Tree> tree, shared_ptr<Element> configuration)
    : RenderMultiChildParent(tree, configuration) {}
  void Update(shared_ptr<Element> newConfiguration);

  virtual void PrintHtml(string &buf) {
    auto conf = static_pointer_cast<Element>(GetConfiguration());
    buf += "<" + conf->_tag + ">";
    RenderMultiChildParent::PrintHtml(buf);
    buf += "</" + conf->_tag + ">";
  }
};

}

#endif //BARISTA2_HTML_H
