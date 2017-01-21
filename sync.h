//
// Created by Yegor Jbanov.
//

#ifndef BARISTA2_SYNC_H_H
#define BARISTA2_SYNC_H_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <sstream>

using namespace std;

namespace barista {
namespace sync {

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
  ElementUpdate(int index) : _index(index) { };

  void RemoveChild(int index) { _removes.push_back(index); }

  void MoveChild(int insertionIndex, int moveFrom) {
    _moves.push_back({insertionIndex, moveFrom});
  }

  ElementUpdate& InsertChildElement(int insertionIndex) {
    _childElementInsertions.push_back(ElementUpdate(insertionIndex));
    return _childElementInsertions.back();
  }

  ElementUpdate& UpdateChild(int index) {
    _childElementUpdates.push_back(ElementUpdate(index));
    return _childElementUpdates.back();
  }

  void InsertChildText(int insertionIndex, string text) {
    _childTextInsertions.push_back({insertionIndex, text});
  }

  void UpdateChildText(int index, string text) {
    _childTextUpdates.push_back({index, text});
  }

  void SetTag(string tag) { _tag = tag; }

private:
  static string _emptyString;

  // insert-before index if this is being inserted.
  // child index if this is being updated.
  int _index;

  string _tag;

  vector<int> _removes;
  vector<Move> _moves;

  vector<ElementUpdate> _childElementInsertions;
  vector<ElementUpdate> _childElementUpdates;

  //           insert-before index
  //           |
  vector<tuple<int, string>> _childTextInsertions;

  //           child index
  //           |
  vector<tuple<int, string>> _childTextUpdates;
};

} // namespace sync
} // namespace barista

#endif // BARISTA2_SYNC_H_H
