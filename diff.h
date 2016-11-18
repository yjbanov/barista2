//
// Created by Yegor Jbanov on 10/22/16.
//

#ifndef BARISTA2_DIFF_H_H
#define BARISTA2_DIFF_H_H

#include <string>
#include <vector>

using namespace std;

namespace barista {

// https://docs.google.com/document/d/1S2LtypuvJ_l1U_SJ2T2Iod8Jm_Ih0Ti6Zb587_0nbJM
class HtmlDiff {
 public:
  HtmlDiff() {}

  void Move(int position);
  void Push();
  void Pop();
  void Element(string tag);

 private:
  void _AddString(string s);
  void _AddNumber(int n);

  vector<string> _ops;
};

}

#endif //BARISTA2_DIFF_H_H
