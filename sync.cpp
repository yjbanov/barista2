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

  if (_bid != "") {
    js["bid"] = _bid;
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
      js["update-elements"] = jsUpdates;
      wroteData = true;
    }
  }

  if (!_attributes.empty()) {
    auto jsAttrUpdates = nlohmann::json::object();
    for (tuple<string, string> attrUpdate : _attributes) {
      jsAttrUpdates[get<0>(attrUpdate)] = get<1>(attrUpdate);
    }
    js["attrs"] = jsAttrUpdates;
    wroteData = true;
  }

  if (wroteData) {
    js["index"] = _index;
  }

  return wroteData;
}

void ElementUpdate::PrintHtml(stringstream &buf) {
  if (_index != -1) {  // we don't print host tag.
    buf << "<" << _tag;

    if (_key != "") {
      buf << " _bkey=\"" << _key << "\"";
    }

    if (_attributes.size() > 0) {
      auto first = _attributes.begin();
      auto last = _attributes.end();
      for (auto attr = first; attr != last; attr++) {
        buf << " " << get<0>(*attr) << "=\"" << get<1>(*attr) << "\"";
      }
    }

    if (_bid != "") {
      buf << " _bid=\"" << _bid << "\"";
    }

    buf << ">";
  }

  if (_text != "") {
    buf << _text;
  }

  for(auto childElement = _childElementInsertions.begin(); childElement != _childElementInsertions.end(); childElement++) {
    childElement->PrintHtml(buf);
  }

  if (_index != -1) {
    buf << "</" << _tag << ">";
  }
}

}  // namespace barista
