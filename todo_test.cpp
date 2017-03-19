#include <iostream>
#include <sstream>
#include <vector>

#include "api.h"
#include "test.h"
#include "todo_widgets.h"

using namespace std;
using namespace barista;

TEST(TestTodoApp)
  auto tree = make_shared<Tree>(make_shared<TodoApp>());
  auto html = tree->RenderFrame();
END_TEST

int main() {
  cout << "Start tests" << endl;
  TestTodoApp();
  cout << "End tests" << endl;
  return 0;
}
