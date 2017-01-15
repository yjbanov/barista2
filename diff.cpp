//
// Created by Yegor Jbanov on 11/18/16.
//

#include "diff.h"

#include <string>
#include <vector>

using namespace std;

namespace barista {

const string ChildListDiff::_emptyString = "";

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

string HtmlDiff::ComputeDiff() {
  stringstream ss;
  ss << "[";
  for (auto op = _ops.begin(); op != _ops.end(); op++) {
    if (op != _ops.begin()) {
      ss << ",";
    }
    ss << *op;
  }
  ss << "]";
  return ss.str();
}

string HtmlDiff::Finalize() {
  auto diff = ComputeDiff();
  _instance = new HtmlDiff();
  return diff;
}

void HtmlDiff::UpdateChildren(ChildListDiff* childListDiff) {
  _AddString("diff");

  auto removes = childListDiff->_removes;
  _AddNumber(removes.size());
  for (auto r = removes.begin(); r != removes.end(); r++) {
    _AddNumber(*r);
  }

  auto insertions = childListDiff->_insertions;
  _AddNumber(insertions.size());
  for (auto i = insertions.begin(); i != insertions.end(); i++) {
    _AddNumber(get<0>(*i));
    Insertion* insertion = get<1>(*i);
    if (insertion->IsNewChild()) {
      _AddString(insertion->Html());
    } else {
      _AddNumber(insertion->MoveFrom());
    }
  }

  auto updates = childListDiff->_updates;
  _AddNumber(updates.size());
  for (auto i = updates.begin(); i != updates.end(); i++) {
    _AddNumber(*i);
  }
}

// TODO: assert that _ops is only mutated when the diff is not locked.

void HtmlDiff::_AddString(string s) {
  _ops.push_back("\"" + s + "\"");
}

void HtmlDiff::_AddNumber(signed long n) {
  _ops.push_back(to_string(n));
}

}
