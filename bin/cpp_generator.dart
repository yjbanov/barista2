part of app_generator;

class CppCodeEmitter {
  final String prefix;

  CppCodeEmitter(this.prefix);

  Map<String, String> render(App app) {
    return {
      '${prefix}.cpp': _mainCode(),
      '${prefix}_widgets.h': _widgetCode(app),
    };
  }

  String _mainCode() {
    return '''
#include <emscripten/emscripten.h>
#include "api.h"
#include "${prefix}_widgets.h"

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
''';
  }

  String _widgetCode(App app) {
    var code = new StringBuffer();
    code.writeln(header);

    for (Widget widget in app.widgets) {
      code.writeln(new CppWidgetCodeEmitter(widget)
          .render());
    }

    code.writeln(stateHeader);

    code.writeln("container->AddChild(make_shared<${app.rootWidget.metadata.name}>());");

    code.writeln(stateFooter);
    code.writeln(footer);
    return code.toString();
  }

  static const header = """
#ifndef GIANT_APP_H
#define GIANT_APP_H

#include <iostream>
#include <list>
#include <locale>
#include <codecvt>
#include <string>

#include "api.h"
#include "html.h"

using namespace std;
using namespace barista;
""";

  static const stateHeader = """
class SampleAppState : public State, public enable_shared_from_this<SampleAppState>{
 public:
  SampleAppState() {
  };

  virtual shared_ptr<Node> Build() {
    auto container = El("div");
""";

  static const stateFooter = """
    return container;
  }
};
""";

  static const footer = """

class SampleApp : public StatefulWidget {
 public:
  SampleApp() : StatefulWidget() {}

  virtual shared_ptr<State> CreateState() {
    return make_shared<SampleAppState>();
  }
};

#endif //GIANT_APP_H
""";
}

class CppTemplateNodeGenerator {
  final TemplateNode template;

  StringBuffer buf;
  int localVariableCounter = 1;
  bool forStatefulWidget;

  CppTemplateNodeGenerator({@required this.template, @required this.forStatefulWidget});

  void write(String s) {
    buf.write('    $s');
  }

  void writeln(String s) {
    buf.writeln('    $s');
  }

  String render() {
    buf = new StringBuffer();
    localVariableCounter = 1;
    var childVariableName = _renderChild(template, parent: null);
    buf.writeln('return ${childVariableName};');
    return buf.toString();
  }

  String nextVariableName() => 'child${localVariableCounter++}';

  String _renderChild(TemplateNode node, {@required String parent}) {
    if (node is ElementNode) {
      return _renderElementNode(node, parent);
    } else if (node is WidgetNode) {
      return _renderWidgetNode(node, parent);
    } else if (node is ContentNode) {
      return _renderContentNode(parent);
    } else if (node is ToggleButtonNode) {
      return _renderToggleButtonNode(node, parent);
    } else {
      throw 'oops';
    }
  }

  String _renderToggleButtonNode(ToggleButtonNode node, String parent) {
    var variableName = nextVariableName();
    writeln('auto ${variableName} = El("button");');
    writeln('${variableName}->SetKey("${node.key}");');
    writeln('${variableName}->SetText("${node.text}");');
    writeln('''
      ${variableName}->AddEventListener("click", [&]() {
        ${node.controls.variable.name} = !${node.controls.variable.name};
        ScheduleUpdate();
      });
    '''.trim());
    writeln('${parent}->AddChild(${variableName});');
    return variableName;
  }

  String _renderElementNode(ElementNode node, String parent) {
    if (node.conditional != null) {
      var expression = toCppExpression(node.conditional, forStatefulWidget: forStatefulWidget);
      writeln('if (${expression}) {');
    }
    var variableName = nextVariableName();
    writeln('auto ${variableName} = El("${node.tag}");');
    writeln('${variableName}->SetKey("${node.key}");');
    for (Attribute attr in node.attrs) {
      var expression = toCppExpression(attr.binding, forStatefulWidget: forStatefulWidget);
      writeln('${variableName}->SetAttribute("${attr.name}", ${expression});');
    }
    for (String clazz in node.classes) {
      writeln(
          '${variableName}->AddClassName("${clazz}");');
    }
    if (node.text != null) {
      writeln('${variableName}->SetText("${node.text}");');
    }
    if (parent != null) {
      writeln('${parent}->AddChild(${variableName});');
    }

    for (var child in node.children) {
      _renderChild(child, parent: variableName);
    }

    if (node.conditional != null) {
      writeln('}');
    }
    return variableName;
  }

  String _renderWidgetNode(WidgetNode node, String parent) {
    if (node.conditional != null) {
      var expression = toCppExpression(node.conditional, forStatefulWidget: forStatefulWidget);
      writeln('if (${expression}) {');
    }
    var variableName = nextVariableName();
    writeln(
        'auto ${variableName} = make_shared<${node.widget.metadata.name}>();');
    writeln('${variableName}->SetKey("${node.key}");');
    for (Attribute attr in node.args) {
      var expression = toCppExpression(attr.binding, forStatefulWidget: forStatefulWidget);
      writeln('${variableName}->Set${capitalize(attr.name)}(${expression});');
    }
    if (node.text != null) {
      writeln('${variableName}->AddChild(Tx("${node.text}"));');
    }
    if (parent != null) {
      writeln('${parent}->AddChild(${variableName});');
    }

    for (var child in node.children) {
      if (child == null) {
        throw 'null child not allowed';
      }
      _renderChild(child, parent: variableName);
    }

    if (node.conditional != null) {
      writeln('}');
    }
    return variableName;
  }

  bool _debugIsContentNodeRendered = false;

  String _renderContentNode(String parent) {
    if (parent == null) {
      throw 'Content node must have a parent';
    }
    if (_debugIsContentNodeRendered) {
      throw 'Multiple content nodes generated:\n${template}';
    }
    _debugIsContentNodeRendered = true;
    if (forStatefulWidget) {
      writeln('${parent}->AddChild(config->GetContent());');
    } else {
      writeln('${parent}->AddChild(content);');
    }

    return null;
  }
}

String capitalize(String s) {
  return s[0].toUpperCase() + s.substring(1);
}

String toCppExpression(Binding b, {@required bool forStatefulWidget}) {
  if (b is Literal) {
    return b.value is String ? '"${b.value}"' : '${b.value}';
  } else if (b is Expression) {
    if (forStatefulWidget) {
      if (b.field.source == ValueSource.input) {
        return 'config->Get${capitalize(b.field.variable.name)}()';
      } else {
        return b.field.variable.name;
      }
    } else {
      // Stateless widgets always access fields from `this`.
      return 'Get${capitalize(b.field.variable.name)}()';
    }
  } else {
    throw 'oops';
  }
}

class CppWidgetCodeEmitter {
  final Widget widget;

  CppWidgetCodeEmitter(this.widget);

  ComponentMetadata get metadata => widget.metadata;

  String render() {
    if (metadata.states != null && metadata.states.isNotEmpty) {
      return _renderStatefulComponent();
    } else {
      return _renderStatelessComponent();
    }
  }

  String _renderTemplate() {
    var buf = new StringBuffer();
    void writeln(s) {
      buf.writeln('    $s');
    }
    writeln(new CppTemplateNodeGenerator(template: widget.template, forStatefulWidget: widget.isStateful)
        .render());
    return buf.toString();
  }

  String _initialValue(Variable v) => v.type == VariableType.bool
      ? v.initialValue.toString()
      : '"${v.initialValue}"';

  String _generateStateFields() {
    return metadata.states
        .map((i) =>
            '  ${i.typeName} ${i.name} = ${_initialValue(i)};\n')
        .join();
  }

  String _generateInputFields() {
    var buf = new StringBuffer();

    void addField(String name, String type, String initialValue) {
      buf.writeln('  ${type} ${name} = ${initialValue};');
      buf.writeln('  ${type} Get${capitalize(name)}() { return ${name};}');
      buf.writeln('  void Set${capitalize(name)}(${type} v) { ${name} = v;}');
      buf.writeln();
    }

    for (Variable input in metadata.inputs) {
      addField(input.name, input.typeName, _initialValue(input));
    }

    if (widget.hasContent) {
      addField('content', 'shared_ptr<Node>', 'nullptr');
      buf.writeln('  void AddChild(shared_ptr<Node> v) { content = v;}');
    }

    return buf.toString();
  }

  String _renderStatefulComponent() {
    return '''
class ${metadata.name} : public StatefulWidget {
 public:
  ${metadata.name}() : StatefulWidget() { }

  shared_ptr<State> CreateState();

${_generateInputFields()}
};

class ${metadata.name}State : public State {
 public:
  virtual shared_ptr<Node> Build() {
    auto config = static_pointer_cast<${widget.metadata.name}>(GetConfig());
${_renderTemplate()}
  }

${_generateStateFields()}
};

shared_ptr<State> ${metadata.name}::CreateState() {
  return make_shared<${metadata.name}State>();
}
''';
  }

  String _renderStatelessComponent() {
    return '''
class ${metadata.name} : public StatelessWidget {
 public:
   ${metadata.name}() : StatelessWidget() { }

  virtual shared_ptr<Node> Build() {
${_renderTemplate()}
  }

${_generateInputFields()}
};
''';
  }
}
