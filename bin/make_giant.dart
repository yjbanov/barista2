import 'dart:async';

import 'dart:convert';
import 'dart:io';
import 'package:args/args.dart';
import 'package:process/process.dart';

final pm = new LocalProcessManager();
int optimizerLevel = 3;
int componentCount = 500;

Future<Null> main(List<String> rawArgs) async {
  var argParser = new ArgParser()
    ..addOption('optimizer-level', abbr: 'O', callback: (String v) {
      if (v != null) {
        optimizerLevel = int.parse(v);
      }
    })
    ..addOption('components', abbr: 'c', callback: (String v) {
      if (v != null) {
        componentCount = int.parse(v);
      }
    });
  argParser.parse(rawArgs);

  var code = new StringBuffer();
  code.writeln(header);
  code.writeln(stateHeader);
  code.writeln(stateFooter);
  code.writeln(footer);

  var cppFile = new File("giant.cpp");
  await cppFile.writeAsString(code.toString());
}

const header = """
#include <iostream>
#include <emscripten/emscripten.h>
#include <list>
#include <locale>
#include <codecvt>
#include <string>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;
""";

const stateHeader = """
class SampleAppState : public State, public enable_shared_from_this<SampleAppState>{
 public:
  SampleAppState() {
  };

  virtual shared_ptr<Node> Build() {
    auto container = El("div");
    container->SetText("I am GIANT!");
""";

const stateFooter = """
    return container;
  }
};
""";

const footer = """

class SampleApp : public StatefulWidget {
 public:
  SampleApp() : StatefulWidget() {}

  virtual shared_ptr<State> CreateState() {
    return make_shared<SampleAppState>();
  }
};

shared_ptr<Tree> tree;

// This is intentionally static so that the string is not
// destroyed after it is returned to the JS side.
string lastDiff;

extern "C" {

const char* RenderFrame() {
  lastDiff = tree->RenderFrame(2);
  return lastDiff.c_str();
}

void DispatchEvent(char* type, char* baristaId) {
  tree->DispatchEvent(type, baristaId);
}

int main() {
  EM_ASM(
      enteredMain();
  );
  tree = make_shared<Tree>(make_shared<SampleApp>());
  EM_ASM(
    allReady();
  );
}

}
""";
