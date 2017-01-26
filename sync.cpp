//
// Created by Yegor Jbanov on 9/4/16.
//

#include "sync.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <iostream>

using namespace std;

namespace barista {

bool ElementUpdate::Render(stringstream &buf) {
  buf << "{";

  buf << "\"index\":\"" << _index << "\",";

  bool wroteData = false;
  if (_tag != "") {
    buf << "\"tag\":\"" << _tag << "\",";
    wroteData = true;
  }

  if (_updateText) {
    buf << "\"text\":\"" << _text << "\",";
    wroteData = true;
  }

  if (!_removes.empty()) {
    buf << "\"remove\": [";
    for (int index : _removes) {
      buf << index << ",";
    }
    buf << "],";
    wroteData = true;
  }

  if (!_moves.empty()) {
    buf << "\"move\":[";
    for (Move move : _moves) {
      buf << move.GetInsertionIndex() << "," << move.GetMoveFromIndex() << ",";
    }
    buf << "],";
    wroteData = true;
  }

  if (!_childElementInsertions.empty()) {
    buf << "\"insert\":[";
    for (ElementUpdate& insertion : _childElementInsertions) {
      buf << "{";
      buf << "\"index\":" << insertion._index << ",";
      buf << "\"html\":\"";
      insertion.PrintHtml(buf);
      buf << "\",";
      buf << "},";
    }
    buf << "],";
    wroteData = true;
  }

  if (!_childElementUpdates.empty()) {
    bool hasChildUpdates = false;
    stringstream subBuf;
    for (ElementUpdate& update : _childElementUpdates) {
      bool nonTrivialUpdate = update.Render(subBuf);
      hasChildUpdates = hasChildUpdates || nonTrivialUpdate;
      subBuf << ",";
    }
    if (hasChildUpdates) {
      buf << "\"update-elements\":[" << subBuf.str() << "],";
      wroteData = true;
    }
  }

  if (!_childTextInsertions.empty()) {
    buf << "\"insert-text\":[";
    for (tuple<int, string> textInsertion : _childTextInsertions) {
      buf << "{";
      buf << "\"index\":" << get<0>(textInsertion);
      buf << "\"text\":\"" << get<1>(textInsertion) << "\",";
      buf << "},";
    }
    buf << "],";
    wroteData = true;
  }

  if (!_childTextUpdates.empty()) {
    buf << "\"update-text\":[";
    for (tuple<int, string> textUpdate : _childTextInsertions) {
      buf << "{";
      buf << "\"index\":" << get<0>(textUpdate);
      buf << "\"text\":\"" << get<1>(textUpdate) << "\",";
      buf << "},";
    }
    buf << "],";
    wroteData = true;
  }

  buf << "}";

  return wroteData;
}

void ElementUpdate::PrintHtml(stringstream &buf) {
  if (_index != -1) {  // we don't print host tag.
    // if (_tag == "") {
    //   cout << "Attempted to print empty HTML tag" << endl;
    //   exit(1);
    // }
    buf << "<" << _tag;

    if (_key != "") {
      buf << " _bkey=\"" << _key << "\"";
    }

    // TODO: attributes
    // TODO: _bid
    buf << ">";
  }

  if (_text != "") {
    buf << _text;
  }

  vector<ElementUpdate>::iterator childElement = _childElementInsertions.begin();
  vector<tuple<int, string>>::iterator childText = _childTextInsertions.begin();

  // Merge assuming insertion indices are ordered
  while(childElement != _childElementInsertions.end() && childText != _childTextInsertions.end()) {
    int childElementIndex = childElement->_index;
    int childTextIndex = get<0>(*childText);
    if (childElementIndex < childTextIndex) {
      childElement->PrintHtml(buf);
      childElement++;
    } else {
      buf << get<1>(*childText);
      childText++;
    }
  }

  if (_index != -1) {
    buf << "</" << _tag << ">";
  }
}

}  // namespace barista
