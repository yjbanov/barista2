//
// Created by Yegor Jbanov on 11/18/16.
//

#include "diff.h"

#include <string>
#include <vector>

using namespace std;

namespace barista {

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
