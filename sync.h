//
// Created by Yegor Jbanov.
//

#ifndef BARISTA2_SYNC_H_H
#define BARISTA2_SYNC_H_H

#include "common.h"
#include "lib/json/src/json.hpp"

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <sstream>

using namespace std;

namespace barista {

class Move {
public:
  Move(int insertionIndex, int moveFromIndex)
    : _insertionIndex(insertionIndex),
      _moveFromIndex(moveFromIndex) { };

  int GetInsertionIndex() { return _insertionIndex; }
  int GetMoveFromIndex() { return _moveFromIndex; }
private:
  int _insertionIndex;
  int _moveFromIndex;
};

class ElementUpdate {
public:
  /// Appends the JSON representation of this update into [buffer].
  bool Render(nlohmann::json& js);

  /// Assumes that this element update is exlusively made of insertions and
  /// renders it as a plain HTML into the given [buffer].
  void PrintHtml(stringstream &buffer);

  void RemoveChild(int index) { _removes.push_back(index); }

  void MoveChild(int insertionIndex, int moveFrom) {
    _moves.push_back({insertionIndex, moveFrom});
  }

  ElementUpdate& InsertChildElement(int insertionIndex) {
    _childElementInsertions.push_back(ElementUpdate(insertionIndex));
    return _childElementInsertions.back();
  }

  ElementUpdate& UpdateChildElement(int index) {
    _childElementUpdates.push_back(ElementUpdate(index));
    return _childElementUpdates.back();
  }

  void SetTag(string tag) { _tag = tag; }
  void SetKey(string key) { _key = key; }
  void SetText(string text) { _text = text; _updateText = true; }
  void SetAttribute(string name, string value) {
    _attributes.push_back({name, value});
  }
  void SetBaristaId(string bid) {
    _bid = bid;
  }

private:
  ElementUpdate(int index) : _index(index) { };

  // insert-before index if this is being inserted.
  // child index if this is being updated.
  int _index;

  string _tag = "";
  string _key = "";
  string _bid = "";

  bool _updateText = false;
  string _text = "";

  vector<int> _removes;
  vector<Move> _moves;

  vector<ElementUpdate> _childElementInsertions;
  vector<ElementUpdate> _childElementUpdates;
  vector<tuple<string, string>> _attributes;

  PRIVATE_COPY_AND_ASSIGN(ElementUpdate);

  friend class allocator<ElementUpdate>;
  friend class TreeUpdate;
};

class TreeUpdate {
 public:
  TreeUpdate() :
    _rootUpdate(ElementUpdate(0)) { };

  ElementUpdate& CreateRootElement() {
    _createMode = true;
    return _rootUpdate;
  }

  ElementUpdate& UpdateRootElement() {
    _createMode = false;
    return _rootUpdate;
  }

  string Render() {
    return Render(0);
  }

  string Render(int indent) {
    nlohmann::json js;
    if (_createMode) {
      stringstream html;
      _rootUpdate.PrintHtml(html);
      js["create"] = html.str();
    } else {
      nlohmann::json jsRootUpdate;
      if (_rootUpdate.Render(jsRootUpdate)) {
        js["update"] = jsRootUpdate;
      }
    }
    return js.dump(indent);
  }

 private:
  bool _createMode = false;
  ElementUpdate _rootUpdate;
};

} // namespace barista

#endif // BARISTA2_SYNC_H_H
