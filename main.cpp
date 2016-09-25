#include <iostream>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;

class SampleApp : public StatelessWidget {
 public:
  SampleApp() : StatelessWidget() {}
  shared_ptr<Node> Build() {
    auto elem = make_shared<Element>("div");
    auto childDiv = make_shared<Element>("div");
    childDiv->AddChild(make_shared<Text>("hello"));
    elem->AddChild(childDiv);
    elem->AddChild(make_shared<Element>("span"));
    return elem;
  }
};

extern "C" {

const char* RenderApp() {
  auto tree = make_shared<Tree>(make_shared<SampleApp>());
  tree->RenderFrame();
  auto html = new string("");
  tree->PrintHtml(*html);
  return html->c_str();
}

}

int main() {
  auto tree = make_shared<Tree>(make_shared<SampleApp>());
  tree->RenderFrame();
  auto html = new string("");
  tree->PrintHtml(*html);
  cout << *html << endl;
}
