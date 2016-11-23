//
// Created by Yegor Jbanov on 11/18/16.
//

#include "diff.h"

#include <string>
#include <vector>

using namespace std;

namespace barista {

HtmlDiff* HtmlDiff::_instance = new HtmlDiff();

HtmlDiff* GetHtmlDiff() {
  if (HtmlDiff::_instance == nullptr) {
    throw "HtmlDiff instance is null.";
  }
  return HtmlDiff::_instance;
};

void HtmlDiff::AssertReady() {
  if (_ops.size() > 0) {
    throw "Empty ops list expected but found " + to_string(_ops.size()) + " ops.";
  }
}

string HtmlDiff::Finalize() {
  stringstream ss;
  ss << "[";
  for (auto op = _ops.begin(); op != _ops.end(); op++) {
    if (op != _ops.begin()) {
      ss << ",";
    }
    ss << *op;
  }
  ss << "]";
  _instance = new HtmlDiff();
  return ss.str();
}

void HtmlDiff::Move(int position) {
  _AddString("move");
  _AddNumber(position);
}

void HtmlDiff::Push() {
  _AddString("push");
}

void HtmlDiff::Pop() {
  _AddString("pop");
}

void HtmlDiff::Element(string tag) {
  _AddString("element");
  _AddString(tag);
}

void HtmlDiff::_AddString(string s) {
  _ops.push_back("\"" + s + "\"");
}

void HtmlDiff::_AddNumber(int n) {
  _ops.push_back(to_string(n));
}

}