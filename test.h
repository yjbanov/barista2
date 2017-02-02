#ifndef BARISTA2_TEST_H
#define BARISTA2_TEST_H

#include <memory>
#include "api.h"

using namespace std;
using namespace barista;

template<typename T> void Expect(T actual, T expected);
template<typename E> void ExpectVector(vector<E> actual, vector<E> expected);

void ExpectHtml(shared_ptr<Tree>, string);
void ExpectTreeUpdate(shared_ptr<Tree> tree, TreeUpdate& expected);

#define TEST(FunctionName) \
  void FunctionName() { \
    cout << #FunctionName << endl;

#define END_TEST \
    cout << "Success" << endl; \
  }

#endif //BARISTA2_TEST_H
