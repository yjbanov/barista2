import 'package:meta/meta.dart';
import 'generator.dart';

class CodeEmitter {
  CodeEmitter();

  Map<String, String> render(App app) {
    return {
      'web/giant_widgets.dart': _widgetCode(app),
    };
  }

  String _widgetCode(App app) {
    var code = new StringBuffer();
    code.writeln(header);

    for (Widget widget in app.widgets) {
      code.writeln(new _WidgetCodeEmitter(widget)
          .render());
    }

    code.writeln(appComponentHeader([app.rootWidget.metadata.name]));
    code.writeln("<${app.rootWidget.metadata.name.toLowerCase()}></${app.rootWidget.metadata.name.toLowerCase()}>");
    code.writeln(appComponentFooter);
    return code.toString();
  }

  static const header = """
import 'package:angular2/angular2.dart';
""";

  String appComponentHeader(List<String> directives) => """
@Component(
  selector: 'giant-app',
  directives: const [${directives.join(',')}],
  template:
'''
""";

  static const appComponentFooter = """
'''
)
class GiantApp {
}
""";
}

class _TemplateNodeGenerator {
  final TemplateNode template;
  final List<String> listeners = [];
  final List<String> classFields = [];

  StringBuffer buf;
  int localVariableCounter = 1;
  bool forStatefulWidget;

  _TemplateNodeGenerator({@required this.template, @required this.forStatefulWidget});

  void write(String s) {
    buf.write(s);
  }

  void writeln(String s) {
    buf.writeln(s);
  }

  String render() {
    buf = new StringBuffer();
    localVariableCounter = 1;
    _renderChild(template);
    return buf.toString();
  }

  String nextListenerName() => 'handle${localVariableCounter++}';
  String nextClassFieldName() => 'classList${localVariableCounter++}';

  void _renderChild(TemplateNode node) {
    if (node is ElementNode) {
      return _renderElementNode(node);
    } else if (node is WidgetNode) {
      return _renderWidgetNode(node);
    } else if (node is ContentNode) {
      return _renderContentNode();
    } else if (node is ToggleButtonNode) {
      return _renderToggleButtonNode(node);
    } else {
      throw 'oops';
    }
  }

  void _renderToggleButtonNode(ToggleButtonNode node) {
    var listenerName = nextListenerName();
    listeners.add('''  void ${listenerName}() { ${node.controls.variable.name} = !${node.controls.variable.name}; }''');
    write('<button _bkey="${node.key}" (click)="${listenerName}()">${node.text}</button>');
  }

  void _renderElementNode(ElementNode node) {
    write('<${node.tag}');
    if (node.conditional != null) {
      var expression = toDartExpression(node.conditional);
      write(' *ngIf="${expression}"');
    }
    write(' _bkey="${node.key}"');
    for (Attribute attr in node.attrs) {
      Binding b = attr.binding;
      if (b is Literal) {
        write(' ${attr.name}="${b.value}"');
      } else {
        write(' [attr.${attr.name}]="${toDartExpression(b)}"');
      }
    }
    if (node.classes.isNotEmpty) {
      var classField = nextClassFieldName();
      write(' [ngClass]="${classField}"');
      classFields.add('''  List<String> ${classField} = ['${node.classes.join("', '")}'];''');
    }

    write('>');

    if (node.text != null) {
      write(node.text);
    }

    for (var child in node.children) {
      writeln('');
      _renderChild(child);
    }

    writeln('</${node.tag}>');
  }

  void _renderWidgetNode(WidgetNode node) {
    String tag = node.widget.metadata.name.toLowerCase();
    write('<${tag}');
    if (node.conditional != null) {
      var expression = toDartExpression(node.conditional);
      write(' *ngIf="${expression}"');
    }
    write(' _bkey="${node.key}"');

    for (Attribute arg in node.args) {
      var expression = toDartExpression(arg.binding);
      write(' [${arg.name}]="${expression}"');
    }

    write('>');

    if (node.text != null) {
      write(node.text);
    }

    for (var child in node.children) {
      if (child == null) {
        throw 'null child not allowed';
      }
      writeln('');
      _renderChild(child);
    }

    writeln('</${tag}>');
  }

  bool _debugIsContentNodeRendered = false;

  void _renderContentNode() {
    if (_debugIsContentNodeRendered) {
      throw 'Multiple content nodes generated:\n${template}';
    }
    _debugIsContentNodeRendered = true;
    writeln('<ng-content></ng-content>');
  }
}

String capitalize(String s) {
  return s[0].toUpperCase() + s.substring(1);
}

String toDartExpression(Binding b) {
  if (b is Literal) {
    return b.value is String ? "'${b.value}'" : '${b.value}';
  } else if (b is Expression) {
    return b.field.variable.name;
  } else {
    throw 'oops';
  }
}

class _WidgetCodeEmitter {
  final Widget widget;
  _TemplateNodeGenerator templateGenerator;

  _WidgetCodeEmitter(this.widget) {
    templateGenerator = new _TemplateNodeGenerator(
      template: widget.template,
      forStatefulWidget: widget.isStateful,
    );
  }

  ComponentMetadata get metadata => widget.metadata;

  String render() {
    List<String> directives = ['NgIf', 'NgClass'];
    widget.template.scan((node) {
      if (node is WidgetNode) {
        directives.add(node.widget.metadata.name);
      }
    });
    var template = _renderTemplate();
    return """
@Component(
  selector: '${widget.metadata.name.toLowerCase()}',
  directives: const [${directives.join(', ')}],
  template:
'''
${template}
'''
)
class ${metadata.name} {
  ${_generateInputFields()}
  ${_generateStateFields()}
  ${_generateClassFields()}
  ${_generateListeners()}
}
""";
  }

  String _generateListeners() {
    return templateGenerator.listeners.join('\n\n');
  }

  String _generateClassFields() {
    return templateGenerator.classFields.join('\n\n');
  }

  String _renderTemplate() {
    var buf = new StringBuffer();
    buf.writeln(templateGenerator.render());
    return buf.toString();
  }

  String _initialValue(Variable v) => v.type == VariableType.bool
      ? v.initialValue.toString()
      : '"${v.initialValue}"';

  String _generateStateFields() {
    return metadata.states
        .map((i) =>
            '  ${_typeNameOf(i)} ${i.name} = ${_initialValue(i)};\n')
        .join();
  }

  String _generateInputFields() {
    var buf = new StringBuffer();

    void addField(String name, String type, String initialValue) {
      buf.writeln('  @Input()');
      buf.writeln('  ${type} ${name} = ${initialValue};');
    }

    for (Variable input in metadata.inputs) {
      addField(input.name, _typeNameOf(input), _initialValue(input));
    }

    return buf.toString();
  }
}

String _typeNameOf(Variable v) {
  switch(v.type) {
    case VariableType.bool: return 'bool';
    case VariableType.string: return 'String';
    default: throw 'Unrecognized VariableType ${v.type}';
  }
}
