#include <iostream>
#include <sstream>
#include <vector>

#include "api.h"
#include "test.h"
#include "giant_widgets.h"

using namespace std;
using namespace barista;

TEST(TestBootstrapGiantApp)
  auto tree = make_shared<Tree>(make_shared<SampleApp>());
  tree->RenderFrame();
END_TEST

int main() {
  cout << "Start tests" << endl;
  TestBootstrapGiantApp();
  cout << "End tests" << endl;
  return 0;
}
