//
// Created by Yegor Jbanov on 8/28/16.
//

#ifndef BARISTA2_API_H
#define BARISTA2_API_H

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

using namespace std;

namespace barista {

/// A cast that's dynamic in debug mode and static in release mode.
#ifdef RELEASE_MODE
#define sdcast static_cast
#else
#define sdcast dynamic_cast
#endif // RELEASE_MODE

/// Immutable reference.
template<typename T>
using ref = shared_ptr<T>;

template<typename T>
using List = std::vector<ref<T>>;

class Tree;

class RenderNode;
class RenderParent;
class Event;

class Key {
 public:
  Key() {}
  virtual ~Key() = 0;
};

template<typename T>
class ValueKey final : public Key {
 public:
  ValueKey(ref<T> value) : _value(value) {};
  ~ValueKey() {};

  virtual ref<T> GetValue() {
    return _value;
  }

 private:
  ref<T> _value;
};

class Node {
 public:
  Node(ref<Key> key) : _key(key) {};
  virtual ~Node() = 0;

  virtual ref<Key> GetKey() {
    return _key;
  }

  virtual ref<RenderNode> Instantiate(ref<Tree> t) = 0;

 private:
  ref<Key> _key;
};

typedef function<void(ref<RenderNode>)> RenderNodeVisitor;

class RenderNode {
 public:
  RenderNode(ref<Tree> tree, ref<Node> configuration) : _tree(tree), _configuration(configuration) {
    Update(configuration);
  }

  virtual ~RenderNode() = 0;

  virtual ref<Node> GetConfiguration() {
    return _configuration;
  }

  virtual ref<RenderParent> GetParent() {
    return _parent;
  }

  virtual ref<Tree> GetTree() {
    return _tree;
  }

  virtual void DispatchEvent(ref<Event> event);

  virtual void VisitChildren(RenderNodeVisitor visitor) = 0;

  /// Remove this node from the tree.
  ///
  /// This operation can be used to temporarily remove nodes in order to move
  /// them around.
  virtual void Detach() {
    _parent = NULL;
  }

  /// Attached this node to a [newParent].
  virtual void Attach(ref<RenderParent> newParent) {
    _parent = newParent;
  }

  /// Updates this node and its children.
  ///
  /// Implementations of this class must override this method and ensure that
  /// all necessary updates to `this` node and its children (if any) happen
  /// correctly. The overridden method must call `super.update` to finalize the
  /// update.
  virtual void Update(ref<Node> newConfiguration) {
    assert(newConfiguration != NULL);
    _configuration = newConfiguration;
  }

 private:
  ref<Tree> _tree;
  ref<Node> _configuration;
  ref<RenderParent> _parent;
};

/// A node that has children.
class RenderParent : public RenderNode {
 public:

  RenderParent(ref<Tree> tree, ref<Node> configuration);

  virtual ~RenderParent() = 0;

  /// Whether any of this node's descentant nodes need to be updated.
  virtual bool GetHasDescendantsNeedingUpdate() { return _hasDescendantsNeedingUpdate; }

  virtual void ScheduleUpdate();

  /// Updates this node and its children.
  ///
  /// Implementations of this class must override this method and ensure that
  /// all necessary updates to `this` node and its children (if any) happen
  /// correctly. The overridden method must call `super.update` to finalize the
  /// update.
  virtual void Update(ref<Node> newConfiguration);

 private:
  bool _hasDescendantsNeedingUpdate = true;
};

class MultiChildNode : Node {
 public:
  MultiChildNode(ref<Key> key, List<Node> children)
      : Node(key), _children(children) {}
  virtual ~MultiChildNode() = 0;

 private:
  List<Node> _children;
};

}  // namespace barista

#endif //BARISTA2_API_H
