//
// Created by Yegor Jbanov on 8/28/16.
//

#ifndef BARISTA2_API_H
#define BARISTA2_API_H

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

using namespace std;

namespace barista {

/// A cast that's dynamic in debug mode and static in release mode.
#ifdef RELEASE_MODE
#define sdcast static_cast
#else
#define sdcast dynamic_cast
#endif // RELEASE_MODE

/// Framework class pre-declarations.
class Tree;
class Widget;
class StatelessWidget;
class RenderStatelessWidget;
class StatefulWidget;
class State;
class RenderStatefulWidget;
class RenderNode;
class RenderParent;
class Event;

class Key final {
 public:
  Key(string value) : _value(value) {};
  string GetValue() { return _value; }

 private:
  string _value;
  friend bool operator==(const shared_ptr<Key> a, const shared_ptr<Key> b);
};

bool operator==(const shared_ptr<Key> a, const shared_ptr<Key> b);

class Node {
 public:
  Node(shared_ptr<Key> key) : _key(key) {};
  virtual shared_ptr<Key> GetKey() { return _key; }
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> t) = 0;

 private:
  shared_ptr<Key> _key;
};

typedef function<void(shared_ptr<RenderNode>)> RenderNodeVisitor;

class RenderNode {
 public:
  RenderNode(shared_ptr<Tree> tree, shared_ptr<Node> configuration);
  virtual shared_ptr<Node> GetConfiguration() { return _configuration; }
  virtual shared_ptr<RenderParent> GetParent() { return _parent; }
  virtual shared_ptr<Tree> GetTree() { return _tree; }
  virtual void Detach() { _parent = NULL; }
  virtual void Attach(shared_ptr<RenderParent> newParent) { _parent = newParent; }
  virtual void Update(shared_ptr<Node> newConfiguration);
  virtual void VisitChildren(RenderNodeVisitor visitor) = 0;

 private:
  shared_ptr<Tree> _tree;
  shared_ptr<Node> _configuration;
  shared_ptr<RenderParent> _parent;
};

class Tree : public enable_shared_from_this<Tree> {
 public:
  Tree(shared_ptr<Widget> topLevelWidget) : _topLevelWidget(topLevelWidget) {}
  void VisitChildren(RenderNodeVisitor visitor);
  void RenderFrame();

 private:
  shared_ptr<Widget> _topLevelWidget;
  shared_ptr<RenderNode> _topLevelNode;
};

class RenderParent : public RenderNode {
 public:
  RenderParent(shared_ptr<Tree> tree, shared_ptr<Node> configuration);
  virtual bool GetHasDescendantsNeedingUpdate() { return _hasDescendantsNeedingUpdate; }
  virtual void ScheduleUpdate();
  virtual void Update(shared_ptr<Node> newConfiguration);

 private:
  bool _hasDescendantsNeedingUpdate = true;
};

class Widget : public Node {
 public:
  Widget(shared_ptr<Key> key) : Node(key) {}
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> tree) = 0;
};

class StatelessWidget : public Widget, public enable_shared_from_this<StatelessWidget> {
 public:
  StatelessWidget(shared_ptr<Key> key) : Widget(key) {}
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> tree);
  virtual shared_ptr<Node> Build() = 0;
};

class StatefulWidget : public Widget, public enable_shared_from_this<StatefulWidget> {
 public:
  StatefulWidget(shared_ptr<Key> key) : Widget(key) {}
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> t);
  virtual shared_ptr<State> CreateState() = 0;
};

class State {
 public:
  shared_ptr<StatefulWidget> GetConfig() { return _config; }
  void ScheduleUpdate();
  virtual shared_ptr<Node> Build() = 0;

 private:
  shared_ptr<RenderStatefulWidget> _node;
  shared_ptr<StatefulWidget> _config;
  friend class RenderStatefulWidget;
  friend void internalSetStateNode(shared_ptr<State> state, shared_ptr<RenderStatefulWidget> node);
};

void internalSetStateNode(shared_ptr<State> state, shared_ptr<RenderStatefulWidget> node);

class RenderStatelessWidget : public RenderParent, public enable_shared_from_this<RenderStatelessWidget> {
 public:
  RenderStatelessWidget(shared_ptr<Tree> tree, shared_ptr<StatelessWidget> configuration) : RenderParent(tree, configuration) {}
  virtual void VisitChildren(RenderNodeVisitor visitor) { visitor(_child); }
  virtual void Update(shared_ptr<StatelessWidget> newConfiguration);

 private:
  shared_ptr<RenderNode> _child;
};

class MultiChildNode : Node {
 public:
  MultiChildNode(shared_ptr<Key> key, vector<shared_ptr<Node>> children) : Node(key), _children(children) {}

 private:
  vector<shared_ptr<Node>> _children;
};

class RenderStatefulWidget : public RenderParent, public enable_shared_from_this<RenderStatefulWidget> {
 public:
  RenderStatefulWidget(shared_ptr<Tree> tree, shared_ptr<StatefulWidget> configuration) : RenderParent(tree, configuration) {}
  virtual void VisitChildren(RenderNodeVisitor visitor);
  virtual void ScheduleUpdate();
  virtual void Update(shared_ptr<StatefulWidget> newConfiguration);
  virtual shared_ptr<State> GetState() { return _state; }

 private:
  shared_ptr<State> _state;
  shared_ptr<RenderNode> _child;
  bool _isDirty = false;
};

}  // namespace barista

#endif //BARISTA2_API_H
