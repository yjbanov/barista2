//
// Created by Yegor Jbanov on 8/28/16.
//

#ifndef BARISTA2_API_H
#define BARISTA2_API_H

#include "sync.h"

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

using namespace std;

namespace barista {

/// Computes the longest increasing subsequence of a list of numbers.
vector<int> ComputeLongestIncreasingSubsequence(vector<int> & sequence);

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
  string _value {""};
  friend bool operator==(const shared_ptr<Key> a, const shared_ptr<Key> b);
};

bool operator==(const shared_ptr<Key> a, const shared_ptr<Key> b);

class Node {
 public:
  Node() { }
  virtual shared_ptr<Key> GetKey() { return _key; }
  virtual void SetKey(shared_ptr<Key> key) { _key = key; }
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> t) = 0;

 private:
  shared_ptr<Key> _key = nullptr;
};

typedef function<void(shared_ptr<RenderNode>)> RenderNodeVisitor;

class RenderNode {
 public:
  RenderNode(shared_ptr<Tree> tree);
  virtual shared_ptr<Node> GetConfiguration() { return _configuration; }
  virtual shared_ptr<RenderParent> GetParent() { return _parent; }
  virtual shared_ptr<Tree> GetTree() { return _tree; }
  virtual void Detach() { _parent = nullptr; }
  virtual void Attach(shared_ptr<RenderParent> newParent) { _parent = newParent; }

  /// Returns `true` iff the new configuration is compatible with this node and
  /// therefore it is legal to call [Update] with this configuration.
  virtual bool CanUpdateUsing(shared_ptr<Node> newConfiguration) = 0;

  /// Updates this render node using [newConfiguration].
  virtual void Update(shared_ptr<Node> newConfiguration, ElementUpdate& update);

  virtual void VisitChildren(RenderNodeVisitor visitor) = 0;
  virtual void DispatchEvent(string type, string baristaId) = 0;
  virtual void PrintHtml(string &buf) = 0;

 private:
  shared_ptr<Tree> _tree = nullptr;
  shared_ptr<Node> _configuration = nullptr;
  shared_ptr<RenderParent> _parent = nullptr;
};

class Tree : public enable_shared_from_this<Tree> {
 public:
  Tree(shared_ptr<Node> topLevelWidget) : _topLevelWidget(topLevelWidget) {}
  void VisitChildren(RenderNodeVisitor visitor);
  virtual void DispatchEvent(string type, string baristaId);
  // Applies state changes and produces a serialize HTML diff.
  string RenderFrame();
  void PrintHtml(string &buf);

 private:
  shared_ptr<Node> _topLevelWidget = nullptr;
  shared_ptr<RenderNode> _topLevelNode = nullptr;
};

class RenderParent : public RenderNode {
 public:
  RenderParent(shared_ptr<Tree> tree);
  virtual bool GetHasDescendantsNeedingUpdate() { return _hasDescendantsNeedingUpdate; }
  virtual void ScheduleUpdate();
  virtual void Update(shared_ptr<Node> newConfiguration, ElementUpdate& update);

 private:
  bool _hasDescendantsNeedingUpdate = true;
};

class Widget : public Node {
 public:
  Widget() : Node() {}
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> tree) = 0;
};

class StatelessWidget : public Widget, public enable_shared_from_this<StatelessWidget> {
 public:
  StatelessWidget() : Widget() {}
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> tree);
  virtual shared_ptr<Node> Build() = 0;
};

class StatefulWidget : public Widget, public enable_shared_from_this<StatefulWidget> {
 public:
  StatefulWidget() : Widget() {}
  virtual shared_ptr<RenderNode> Instantiate(shared_ptr<Tree> t);
  virtual shared_ptr<State> CreateState() = 0;
};

class State {
 public:
  shared_ptr<StatefulWidget> GetConfig() { return _config; }
  void ScheduleUpdate();
  virtual shared_ptr<Node> Build() = 0;

 private:
  shared_ptr<RenderStatefulWidget> _node = nullptr;
  shared_ptr<StatefulWidget> _config = nullptr;
  friend class RenderStatefulWidget;
  friend void internalSetStateNode(shared_ptr<State> state, shared_ptr<RenderStatefulWidget> node);
};

void internalSetStateNode(shared_ptr<State> state, shared_ptr<RenderStatefulWidget> node);

class RenderStatelessWidget : public RenderParent, public enable_shared_from_this<RenderStatelessWidget> {
 public:
  RenderStatelessWidget(shared_ptr<Tree> tree) : RenderParent(tree) {}
  virtual void VisitChildren(RenderNodeVisitor visitor) { visitor(_child); }
  virtual void DispatchEvent(string type, string baristaId);
  virtual bool CanUpdateUsing(shared_ptr<Node> newConfiguration);
  virtual void Update(shared_ptr<Node> newConfiguration, ElementUpdate& update);
  virtual void PrintHtml(string &buf) { if (_child != nullptr) { _child->PrintHtml(buf); } }

 private:
  shared_ptr<RenderNode> _child = nullptr;
};

class MultiChildNode : public Node {
 public:
  MultiChildNode() : Node() {}
  vector<shared_ptr<Node>> GetChildren() { return _children; }
  void AddChild(shared_ptr<Node> child) { _children.push_back(child); }

 private:
  vector<shared_ptr<Node>> _children;
};

class RenderStatefulWidget : public RenderParent, public enable_shared_from_this<RenderStatefulWidget> {
 public:
  RenderStatefulWidget(shared_ptr<Tree> tree) : RenderParent(tree) {}
  virtual void VisitChildren(RenderNodeVisitor visitor);
  virtual void DispatchEvent(string type, string baristaId);
  virtual void ScheduleUpdate();
  virtual bool CanUpdateUsing(shared_ptr<Node> newConfiguration);
  virtual void Update(shared_ptr<Node> newConfiguration, ElementUpdate& update);
  virtual shared_ptr<State> GetState() { return _state; }
  virtual void PrintHtml(string &buf) { if (_child != nullptr) { _child->PrintHtml(buf); } }

 private:
  shared_ptr<State> _state = nullptr;
  shared_ptr<RenderNode> _child = nullptr;
  bool _isDirty = false;
};

class RenderMultiChildParent : public RenderParent, public enable_shared_from_this<RenderMultiChildParent> {
 public:
  RenderMultiChildParent(shared_ptr<Tree> tree) : RenderParent(tree) {}

  virtual void VisitChildren(RenderNodeVisitor visitor);
  virtual void Update(shared_ptr<Node> newConfiguration, ElementUpdate& update);
  virtual void PrintHtml(string &buf) {
    for (auto child : _currentChildren) {
      child->PrintHtml(buf);
    }
  }

 private:
  vector<shared_ptr<RenderNode>> _currentChildren;
};

}  // namespace barista

#endif //BARISTA2_API_H
