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
  shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> tree);
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
  RenderElement(shared_ptr<Tree> tree) : RenderMultiChildParent(tree) {}
  void Update(shared_ptr<Node> newConfiguration);

  virtual void PrintHtml(string &buf) {
    auto conf = static_pointer_cast<Element>(GetConfiguration());
    buf += "<" + conf->_tag + ">";
    RenderMultiChildParent::PrintHtml(buf);
    buf += "</" + conf->_tag + ">";
  }
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
  virtual void PrintHtml(string &buf) {
    auto conf = static_pointer_cast<Text>(GetConfiguration());
    buf += conf->GetValue();
  }
};

}

#endif //BARISTA2_HTML_H
