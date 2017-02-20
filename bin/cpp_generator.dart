part of app_generator;

class CppCodeEmitter implements CodeEmitter {
  @override
  String render(App app) {
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
}

class CppTemplateNodeGenerator {
  StringBuffer buf;
  int localVariableCounter = 1;
  bool forStatefulWidget;

  CppTemplateNodeGenerator({@required this.forStatefulWidget});

  void write(String s) {
    buf.write('    $s');
  }

  void writeln(String s) {
    buf.writeln('    $s');
  }

  String render(TemplateNode node, {@required String parent}) {
    buf = new StringBuffer();
    localVariableCounter = 1;
    _renderChild(node, parent: parent);
    return buf.toString();
  }

  String nextVariableName() => 'child${localVariableCounter++}';

  void _renderChild(TemplateNode node, {@required String parent}) {
    if (node is ElementNode) {
      _renderElementNode(node, parent);
    } else if (node is WidgetNode) {
      _renderWidgetNode(node, parent);
    } else if (node is ContentNode) {
      _renderContentNode(parent);
    } else {
      throw 'oops';
    }
  }

  void _renderElementNode(ElementNode node, String parent) {
    if (node.conditional != null) {
      var expression = toCppExpression(node.conditional, forStatefulWidget: forStatefulWidget);
      writeln('if (${expression}) {');
    }
    var variableName = nextVariableName();
    writeln('auto ${variableName} = El("${node.tag}");');
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
    writeln('${parent}->AddChild(${variableName});');

    for (var child in node.children) {
      _renderChild(child, parent: variableName);
    }

    if (node.conditional != null) {
      writeln('}');
    }
  }

  void _renderWidgetNode(WidgetNode node, String parent) {
    if (node.conditional != null) {
      var expression = toCppExpression(node.conditional, forStatefulWidget: forStatefulWidget);
      writeln('if (${expression}) {');
    }
    var variableName = nextVariableName();
    writeln(
        'auto ${variableName} = make_shared<${node.widget.metadata.name}>();');
    for (Attribute attr in node.args) {
      var expression = toCppExpression(attr.binding, forStatefulWidget: forStatefulWidget);
      writeln('${variableName}->Set${capitalize(attr.name)}(${expression});');
    }
    if (node.text != null) {
      writeln('${variableName}->AddChild(Tx("${node.text}"));');
    }
    writeln('${parent}->AddChild(${variableName});');

    for (var child in node.children) {
      _renderChild(child, parent: variableName);
    }

    if (node.conditional != null) {
      writeln('}');
    }
  }

  void _renderContentNode(String parent) {
    if (forStatefulWidget) {
      writeln('${parent}->AddChild(config->GetContent());');
    } else {
      writeln('${parent}->AddChild(content);');
    }
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
      if (b.field.source == ParameterSource.input) {
        return 'config->Get${capitalize(b.field.name)}()';
      } else {
        return b.field.name;
      }
    } else {
      // Stateless widgets always access fields from `this`.
      return 'Get${capitalize(b.field.name)}()';
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
    writeln('auto container = El("div");');
    writeln(new CppTemplateNodeGenerator(forStatefulWidget: widget.isStateful)
        .render(widget.template, parent: 'container'));
    writeln('return container;');
    return buf.toString();
  }

  String _generateStateFields() {
    return metadata.states
        .map((i) =>
            '  ${i.typeName} ${i.name} = ${i.type == VariableType.bool ? "false" : '"${i.name}"'};\n')
        .join();
  }

  String _generateInputFields() {
    var buf = new StringBuffer();

    void addField(String name, String type) {
      buf.writeln('  ${type} ${name};');
      buf.writeln('  ${type} Get${capitalize(name)}() { return ${name};}');
      buf.writeln('  void Set${capitalize(name)}(${type} v) { ${name} = v;}');
      buf.writeln();
    }

    for (Parameter input in metadata.inputs) {
      addField(input.name, input.typeName);
    }

    if (widget.hasContent) {
      addField('content', 'shared_ptr<Node>');
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
