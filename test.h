#ifndef BARISTA2_TEST_H
#define BARISTA2_TEST_H

#include <memory>
#include "api.h"

using namespace std;
using namespace barista;

template<typename T> void Expect(T actual, T expected);
void ExpectHtml(shared_ptr<Tree>, string);
void ExpectDiff(shared_ptr<Tree> tree, HtmlDiff& expected);

#endif //BARISTA2_TEST_H
