import 'package:meta/meta.dart';
import 'generator.dart';

class CodeEmitter {
  CodeEmitter();

  Map<String, String> render(App app) {
    return {
      'others/inferno/src/components/components.tsx': _widgetCode(app),
    };
  }

  String _widgetCode(App app) {
    var code = new StringBuffer();
    code.writeln('''import Component from 'inferno-component';\n''');

    for (Widget widget in app.widgets) {
      code.writeln(new _WidgetCodeEmitter(widget)
          .render());
    }

    code.writeln(
'''
interface GiantAppProps {}

export class GiantApp extends Component<GiantAppProps, any> {

	constructor(props, context) {
		super(props, context);
	}

	render() {
		return (
			<${app.rootWidget.metadata.name}></${app.rootWidget.metadata.name}>
		);
	}
}
'''
    );
    return code.toString();
  }
}

class _TemplateNodeGenerator {
  final TemplateNode template;
  final List<String> listeners = [];
  final Map<String, List<String>> classFields = {};

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
    listeners.add('''
    ${listenerName}(): void {
      this.setState({
        ${node.controls.variable.name}: !this.state.${node.controls.variable.name}
      });
    }
    ''');
    write('<button _bkey="${node.key}" onClick="this.${listenerName}">${node.text}</button>');
  }

  void _renderElementNode(ElementNode node) {
    if (node.conditional != null) {
      var expression = toTSExpression(node.conditional, forStatefulWidget: forStatefulWidget);
      write('{${expression} &&\n');
    }
    write('<${node.tag}');
    write(' _bkey="${node.key}"');
    for (Attribute attr in node.attrs) {
      Binding b = attr.binding;
      if (b is Literal) {
        write(' ${attr.name}="${b.value}"');
      } else {
        write(' ${attr.name}={${toTSExpression(b, forStatefulWidget: forStatefulWidget)}}');
      }
    }
    if (node.classes.isNotEmpty) {
      var classField = nextClassFieldName();
      write(' class={this.state.${classField}}');
      classFields[classField] = node.classes;
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
    if (node.conditional != null) {
      write('}');
    }
  }

  void _renderWidgetNode(WidgetNode node) {
    String tag = node.widget.metadata.name;
    if (node.conditional != null) {
      var expression = toTSExpression(node.conditional, forStatefulWidget: forStatefulWidget);
      write('{${expression} &&\n');
    }
    write('<${tag}');
    write(' _bkey="${node.key}"');

    for (Attribute arg in node.args) {
      var expression = toTSExpression(arg.binding, forStatefulWidget: forStatefulWidget);
      write(' ${arg.name}={${expression}}');
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
    if (node.conditional != null) {
      write('}');
    }
  }

  bool _debugIsContentNodeRendered = false;

  void _renderContentNode() {
    if (_debugIsContentNodeRendered) {
      throw 'Multiple content nodes generated:\n${template}';
    }
    _debugIsContentNodeRendered = true;
    writeln('{this.props.children}');
  }
}

String capitalize(String s) {
  return s[0].toUpperCase() + s.substring(1);
}

String toTSExpression(Binding b, {@required bool forStatefulWidget}) {
  if (b is Literal) {
    return b.value is String ? "'${b.value}'" : '${b.value}';
  } else if (b is Expression) {
    if (forStatefulWidget) {
      if (b.field.source == ValueSource.input) {
        return 'this.props.${b.field.variable.name}';
      } else {
        return 'this.state.${b.field.variable.name}';
      }
    } else {
      // Stateless widgets always access fields from props.
      return 'this.props.${b.field.variable.name}';
    }
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
    var template = _renderTemplate();
    return """
${_generatePropsInterface()}

${_generateStateInterface()}

export class ${metadata.name} extends Component<${metadata.name}Props, ${metadata.name}State> {
  ${_generateListeners()}

  ${_generatePropDefaults()}

  constructor(props, context) {
		super(props, context);
    ${_generateInitState()}
	}

  render() {
		return (
			${template}
		);
	}
}
""";
  }

  String _generateListeners() {
    return templateGenerator.listeners.join('\n\n');
  }

  String _renderTemplate() {
    var buf = new StringBuffer();
    buf.writeln(templateGenerator.render());
    return buf.toString();
  }

  String _initialValue(Variable v) => v.type == VariableType.bool
      ? v.initialValue.toString()
      : '"${v.initialValue}"';

  String _generateStateInterface() {
    var buf = new StringBuffer();
    buf.writeln('interface ${metadata.name}State {');
    for (Variable s in metadata.states) {
      buf.writeln('  ${s.name}: ${_typeNameOf(s)};');
    }
    templateGenerator.classFields.forEach((String fieldName, _) {
      buf.writeln('  ${fieldName}: Array<String>;');
    });
    buf.write('}');
    return buf.toString();
  }

  String _generateInitState() {
    var buf = new StringBuffer();
    buf.writeln('this.state = {');
    var initializers = <String>[];
    for (Variable s in metadata.states) {
      initializers.add('${s.name}: ${_initialValue(s)}');
    }
    templateGenerator.classFields.forEach((String fieldName, List<String> classNames) {
      initializers.add('${fieldName}: [${classNames.map((c) => '"$c"').join(', ')}]');
    });
    buf.writeln(initializers.map((i) => '  ${i}').join(',\n'));
    buf.write('};');
    return buf.toString();
  }

  String _generatePropsInterface() {
    var buf = new StringBuffer();
    buf.writeln('interface ${metadata.name}Props {');
    buf.writeln('  children: any;');
    void addField(String name, String type, String initialValue) {
      buf.writeln('  ${name}: ${type};');
    }

    for (Variable input in metadata.inputs) {
      addField(input.name, _typeNameOf(input), _initialValue(input));
    }

    buf.write('}');
    return buf.toString();
  }

  String _generatePropDefaults() {
    var buf = new StringBuffer();
    buf.writeln('static defaultProps = {');

    bool first = true;
    for (Variable input in metadata.inputs) {
      if (!first) {
        buf.write(',');
      }
      buf.writeln('  ${input.name}: ${_initialValue(input)}');
      first = false;
    }

    buf.writeln('};');
    return buf.toString();
  }
}

String _typeNameOf(Variable v) {
  switch(v.type) {
    case VariableType.bool: return 'boolean';
    case VariableType.string: return 'String';
    default: throw 'Unrecognized VariableType ${v.type}';
  }
}
