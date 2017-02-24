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

TEST(TestWidget0)
  auto root = make_shared<Widget0>();
  root->AddChild(El("div"));
  auto tree = make_shared<Tree>(root);
  cout << tree->RenderFrame(1) << endl;
  tree->DispatchEvent("click", "24");
  cout << tree->RenderFrame(2) << endl;
END_TEST

int main() {
  cout << "Start tests" << endl;
  TestBootstrapGiantApp();
  TestWidget0();
  cout << "End tests" << endl;
  return 0;
}
