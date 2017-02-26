import 'package:meta/meta.dart';
import 'generator.dart';

class CodeEmitter {
  CodeEmitter();

  Map<String, String> render(App app) {
    return {
      'others/ng2ts/giant/src/app/app.component.ts': _widgetCode(app),
      'others/ng2ts/giant/src/app/app.module.ts': _moduleCode(app),
    };
  }

  String _widgetCode(App app) {
    var code = new StringBuffer();
    code.writeln('''import { Component, Input } from '@angular/core';\n''');

    for (Widget widget in app.widgets) {
      code.writeln(new _WidgetCodeEmitter(widget)
          .render());
    }

    code.writeln(
'''
@Component({
  selector: 'app-root',
  template: `
<button (click)="toggleVisibility()">Toggle Visibility</button>
<div *ngIf="visible">
  <${app.rootWidget.metadata.name.toLowerCase()}></${app.rootWidget.metadata.name.toLowerCase()}>
</div>
`
})
export class GiantApp {
  visible: boolean = true;

  toggleVisibility() {
    let before = window.performance.now();
    this.visible = !this.visible;
    setTimeout(() => {
      let after = window.performance.now();
      console.log(`>>> Render frame: \${after - before} ms`);
    }, 10);
  }
}
'''
    );
    return code.toString();
  }

  String _moduleCode(App app) {
    var declarations = <String>[];
    for (Widget widget in app.widgets) {
      declarations.add(widget.metadata.name);
    }

    return '''
import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';

import * as c from './app.component';

@NgModule({
  declarations: [
    c.GiantApp,
    ${declarations.map((d) => 'c.${d}').join(',\n')}
  ],
  imports: [
    BrowserModule,
  ],
  providers: [],
  bootstrap: [c.GiantApp]
})
export class GiantAppModule { }
''';
  }
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
    listeners.add('''  ${listenerName}(): void { this.${node.controls.variable.name} = !this.${node.controls.variable.name}; }''');
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
      classFields.add('''  ${classField}: Array<String> = ['${node.classes.join("', '")}'];''');
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
    var template = _renderTemplate();
    return """
@Component({
  selector: '${widget.metadata.name.toLowerCase()}',
  template: `
${template}`
})
export class ${metadata.name} {
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
            '  ${i.name}: ${_typeNameOf(i)} = ${_initialValue(i)};\n')
        .join();
  }

  String _generateInputFields() {
    var buf = new StringBuffer();

    void addField(String name, String type, String initialValue) {
      buf.writeln('  @Input()');
      buf.writeln('  ${name}: ${type} = ${initialValue};');
    }

    for (Variable input in metadata.inputs) {
      addField(input.name, _typeNameOf(input), _initialValue(input));
    }

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
