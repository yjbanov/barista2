import 'dart:async';
import 'dart:io';
import 'dart:math' as math;

import 'package:args/args.dart';
import 'package:meta/meta.dart';
import 'package:process/process.dart';

final rnd = new math.Random(123456);
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

  final components = <String, ComponentMetadata>{};

  for (int i = 0; i < componentCount; i++) {
    final component = generateComponent(i);
    components[component.name] = component;
    code.writeln(component);
  }

  code.writeln(stateHeader);

  for (final component in components.values) {
    final params = component.inputs.map((p) => p.type == VariableType.bool ? rnd.nextBool().toString() : '"${randomShortString()}"');
    code.writeln("container->AddChild(make_shared<${component.name}>(${params.join(', ')}));");
  }

  code.writeln(stateFooter);
  code.writeln(footer);

  var cppFile = new File("giant.cpp");
  await cppFile.writeAsString(code.toString());
}

String randomShortString() {
  const chars = 'abcdefghijklmnopqrstuvqxyz';
  return new String.fromCharCodes(new List.generate(3 + rnd.nextInt(10), (_) => chars.codeUnitAt(rnd.nextInt(chars.length))));
}

enum VariableType {
  bool, string
}

VariableType randomVariableType() {
  return rnd.nextBool() ? VariableType.bool : VariableType.string;
}

class Parameter {
  // bools are used to control visibility of the UI (e.g. using `if`).
  // string are used to display data.
  final VariableType type;
  final String name;

  String get typeName => '$type'.substring('$type'.indexOf('.') + 1);

  Parameter({
    @required this.type,
    @required this.name,
  });
}

class ComponentMetadata {
  final String name;
  final List<Parameter> inputs;

  // Parameters created and controlled by the component. A component that has
  // at least one state parameter must be a StatefulWidget.
  final List<Parameter> states;

  ComponentMetadata({
    @required this.name,
    @required this.inputs,
    @required this.states,
  });

  String _generateStateFields() {
    return states.map((i) => '  ${i.typeName} _${i.name} = ${i.type == VariableType.bool ? "false" : '"${i.name}"'};\n').join();
  }

  String _generateInputFields() {
    return inputs.map((i) => '  ${i.typeName} _${i.name};\n').join();
  }

  String _generateInputParameters() {
    return inputs.map((i) => '${i.typeName} ${i.name}').join(', ');
  }

  String _generateInputInitializers() {
    return inputs.map((i) => '_${i.name}(${i.name}),').join(' ');
  }

  String _generateStateControlledChildren() {
    final buf = new StringBuffer();
    for (Parameter state in states) {
      if (state.type == VariableType.string) {
        buf.writeln('container->AddChild(Tx(_${state.name}));');
      } else {
        buf.writeln('if (_${state.name}) { container->AddChild(Tx("${state.name}")); }');
      }
    }
    return buf;
  }

  String _generateInputControlledChildren(bool stateless) {
    final buf = new StringBuffer();
    for (Parameter input in inputs) {
      final accessor = '${stateless ? '' : 'config->'}_${input.name}';
      if (input.type == VariableType.string) {
        buf.writeln('    container->AddChild(Tx("${input.name}"));');
      } else {
        buf.writeln('    if (${accessor}) { container->AddChild(Tx("${input.name}")); }');
      }
    }
    return buf;
  }

  String _renderStatefulComponent() {
    return '''
class ${name} : public StatefulWidget {
 public:
  ${name}(${_generateInputParameters()})
    : ${_generateInputInitializers()}
      StatefulWidget() { }

  virtual shared_ptr<State> CreateState();

${_generateInputFields()}
};

class ${name}State : public State {
 public:
  virtual shared_ptr<Node> Build() {
    shared_ptr<${name}> config = static_pointer_cast<${name}>(GetConfig());
    auto container = El("div");
    // state-based control
${_generateStateControlledChildren()}
    // input-based control
${_generateInputControlledChildren(false)}
    return container;
  }

${_generateStateFields()}
};

shared_ptr<State> ${name}::CreateState() {
  return make_shared<${name}State>();
}
''';
  }

  String _renderStatelessComponent() {
    return '''
class ${name} : public StatelessWidget {
 public:
   ${name}(${_generateInputParameters()})
     : ${_generateInputInitializers()}
       StatelessWidget() { }

  virtual shared_ptr<Node> Build() {
    auto container = El("div");
${_generateInputControlledChildren(true)}
    return container;
  }

${_generateInputFields()}
};
''';
  }

  @override
  String toString() {
    if (states != null && states.isNotEmpty) {
      return _renderStatefulComponent();
    } else {
      return _renderStatelessComponent();
    }
  }
}

ComponentMetadata generateComponent(int i) {
  // Decide how many parameters we want and how many of those are inputs and
  // how many are state variables.
  int paramCount = 2 + rnd.nextInt(5);

  int inputCount;
  int stateCount;
  if (rnd.nextBool()) {
    // Stateless
    inputCount = paramCount;
    stateCount = 0;
  } else {
    // Stateful
    inputCount = rnd.nextInt(paramCount);
    stateCount = paramCount - inputCount;
  }

  final states = <Parameter>[];
  for (int i = 0; i < stateCount; i++) {
    states.add(new Parameter(
      type: randomVariableType(),
      name: 'param${i}',
    ));
  }

  final inputs = <Parameter>[];
  for (int i = 0; i < inputCount; i++) {
    inputs.add(new Parameter(
      type: randomVariableType(),
      name: 'param${i}',
    ));
  }

  return new ComponentMetadata(
    name: 'Component${i}',
    inputs: inputs,
    states: states,
  );
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
