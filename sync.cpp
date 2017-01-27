//
// Created by Yegor Jbanov on 9/4/16.
//

#include "sync.h"
#include "lib/json/src/json.hpp"

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

bool ElementUpdate::Render(nlohmann::json& js) {
  bool wroteData = false;

  if (_tag != "") {
    js["tag"] = _tag;
    wroteData = true;
  }

  if (_updateText) {
    js["text"] = _text;
    wroteData = true;
  }

  if (!_removes.empty()) {
    auto jsRemoves = nlohmann::json::array();
    for (int index : _removes) {
      jsRemoves.push_back(index);
    }
    js["remove"] = jsRemoves;
    wroteData = true;
  }

  if (!_moves.empty()) {
    auto jsMoves = nlohmann::json::array();
    for (Move move : _moves) {
      jsMoves.push_back(move.GetInsertionIndex());
      jsMoves.push_back(move.GetMoveFromIndex());
    }
    js["move"] = jsMoves;
    wroteData = true;
  }

  if (!_childElementInsertions.empty()) {
    auto jsInsertions = nlohmann::json::array();
    for (ElementUpdate& insertion : _childElementInsertions) {
      auto jsInsertion = nlohmann::json::object();
      jsInsertion["index"] = insertion._index;
      stringstream buf;
      insertion.PrintHtml(buf);
      jsInsertion["html"] = buf.str();
      jsInsertions.push_back(jsInsertion);
    }
    js["insert"] = jsInsertions;
    wroteData = true;
  }

  if (!_childElementUpdates.empty()) {
    auto jsUpdates = nlohmann::json::array();
    for (ElementUpdate& update : _childElementUpdates) {
      auto childUpdate = nlohmann::json::object();
      if (update.Render(childUpdate)) {
        jsUpdates.push_back(childUpdate);
      }
    }

    if (jsUpdates.size() > 0) {
      jsUpdates["update-elements"] = jsUpdates;
      wroteData = true;
    }
  }

  if (!_childTextInsertions.empty()) {
    auto jsInsertions = nlohmann::json::array();
    for (tuple<int, string> textInsertion : _childTextInsertions) {
      auto jsInsertion = nlohmann::json::object();
      jsInsertion["index"] = get<0>(textInsertion);
      jsInsertion["text"] = get<1>(textInsertion);
      jsInsertions.push_back(jsInsertion);
    }
    js["insert-text"] = jsInsertions;
    wroteData = true;
  }

  if (!_childTextUpdates.empty()) {
    auto jsTextUpdates = nlohmann::json::array();
    for (tuple<int, string> textUpdate : _childTextInsertions) {
      auto jsInsertion = nlohmann::json::object();
      jsInsertion["index"] = get<0>(textUpdate);
      jsInsertion["text"] = get<1>(textUpdate);
      jsTextUpdates.push_back(jsInsertion);
    }
    js["update-text"] = jsTextUpdates;
    wroteData = true;
  }

  if (wroteData) {
    js["index"] = _index;
  }

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
