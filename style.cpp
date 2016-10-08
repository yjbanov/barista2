//
// Created by Yegor Jbanov on 10/3/16.
//

#include <vector>
#include "style.h"

namespace barista {

Style style(vector<StyleAttribute> &attributes) {
  string buf = "";
  for (auto attr : attributes) {
    buf += attr._name + ": " + attr._value + ";\n";
  }
  return Style(buf);
}

uint64_t Style::_idCounter = 1;

} // namespace barista
