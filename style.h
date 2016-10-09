//
// Created by Yegor Jbanov on 10/3/16.
//

#ifndef BARISTA2_STYLE_H
#define BARISTA2_STYLE_H

#include <string>
#include <vector>

using namespace std;

namespace barista {

class Style;

class StyleAttribute {
 public:
  StyleAttribute(string name, string value)
      : _name(name), _value(value) {}

  string GetName() { return _name; }
  string GetValue() { return _value; }

 private:
  string _name;
  string _value;

  friend Style style(vector<StyleAttribute> &attributes);
};

class Style {
 public:
  Style(string css)
      : _css(css),
        _identifierClass("_s" + to_string(_idCounter++)) { }

  string GetCss() { return _css; }
  bool GetIsRegistered() { return _isRegistered; }
  string GetIdentifierClass() { return _identifierClass; }
  static void DangerouslyResetIdCounterForTesting() { _idCounter = 1; }

 private:
  static uint64_t _idCounter;
  string _css;
  string _identifierClass;
  bool _isRegistered = false;
};

Style style(vector<StyleAttribute> &attributes);

} // namespace barista

#endif //BARISTA2_STYLE_H
