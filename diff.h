//
// Created by Yegor Jbanov on 10/22/16.
//

#ifndef BARISTA2_DIFF_H_H
#define BARISTA2_DIFF_H_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <sstream>

using namespace std;

namespace barista {

class ChildListDiff;

// https://docs.google.com/document/d/1S2LtypuvJ_l1U_SJ2T2Iod8Jm_Ih0Ti6Zb587_0nbJM
class HtmlDiff {
 public:
  HtmlDiff() { };

  // Ops.
  void UpdateChildren(ChildListDiff* childListDiff);

  // Lifecycle.
  void AssertReady();

  // While locked, the diff cannot be written to.
  void Lock() { _isLocked = true; };
  void Unlock() { _isLocked = false; };
  bool IsLocked() { return _isLocked; };
  bool IsNotLocked() { return !_isLocked; };

  string ComputeDiff();
  string Finalize();

 private:
  void _AddString(string s);
  void _AddNumber(signed long n);

  static HtmlDiff* _instance;

  bool _isLocked = false;
  vector<string> _ops;

  friend HtmlDiff* GetHtmlDiff();
};

HtmlDiff* GetHtmlDiff();

class Insertion {
 public:
  Insertion(bool isNewChild, int moveFrom, string html) :
      _isNewChild(isNewChild), _moveFrom(moveFrom), _html(html) { };

  const bool IsNewChild() { return _isNewChild; }
  // TODO: assert that methods below are called with accordance with the value of IsNewChild.
  const int MoveFrom() { return _moveFrom; }
  const string Html() { return _html; }
 private:
  const bool _isNewChild;
  // Index into the original child list pointing at the child to be moved.
  const int _moveFrom;
  // HTML for the child to be inserted.
  const string _html;
};

class ChildListDiff {
 public:
  void Remove(int index) { _removes.push_back(index); }

  void Move(int insertionIndex, int moveFrom) {
    _insertions.push_back({insertionIndex, new Insertion(false, moveFrom, _emptyString)});
  }

  void NewChild(int insertionIndex, string html) {
    _insertions.push_back({insertionIndex, new Insertion(true, -1, html)});
  }

  void UpdateChild(int index) {
    _updates.push_back(index);
  }

  void Render() {
    auto htmlDiff = GetHtmlDiff();
    htmlDiff->UpdateChildren(this);
  }

  ~ChildListDiff() {
    for (auto t : _insertions) {
      delete get<1>(t);
    }
  }

 private:
  static const string _emptyString;
  vector<int> _removes;
  vector<tuple<int, Insertion*>> _insertions;
  vector<int> _updates;

  friend void HtmlDiff::UpdateChildren(ChildListDiff* childListDiff);
};

}

#endif //BARISTA2_DIFF_H_H
