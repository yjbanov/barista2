library app_generator;

import 'dart:async';
import 'dart:io';
import 'dart:math' as math;

import 'package:args/args.dart';
import 'package:meta/meta.dart';
import 'package:process/process.dart';

part 'cpp_generator.dart';

final math.Random rnd = new math.Random(123456);
final LocalProcessManager pm = new LocalProcessManager();
int optimizerLevel = 3;
int multiplier = 5;

Future<Null> main(List<String> rawArgs) async {
  var argParser = new ArgParser()
    ..addOption('optimizer-level', abbr: 'O', callback: (String v) {
      if (v != null) {
        optimizerLevel = int.parse(v);
      }
    })
    ..addOption('multiplier', abbr: 'm', callback: (String v) {
      if (v != null) {
        multiplier = int.parse(v);
      }
    });
  argParser.parse(rawArgs);

  App app = generateApp();
  print('----------------- GENERATED APP ------------------');
  print(app.toString());
  print('--------------------------------------------------');
  print('Generated:');
  print('  ${reusableWidgets.length} reusable widgets');
  print('  ${adHocWidgets.length} ad hoc widgets');
  print('  ${TemplateNode.totalNodeCount} template nodes');
  print('  ${TemplateNode.conditionalCount} conditionals');

  var code = new CppCodeEmitter().render(app);
  var cppFile = new File("giant.cpp");
  await cppFile.writeAsString(code.toString());
}

String randomShortString([int maxLength = 10]) {
  const chars = 'abcdefghijklmnopqrstuvqxyz';
  int minLength = maxLength ~/ 2;
  return new String.fromCharCodes(new List.generate(
    minLength + rnd.nextInt(maxLength - minLength),
    (_) => chars.codeUnitAt(rnd.nextInt(chars.length)),
  ));
}

enum VariableType { bool, string }

VariableType randomVariableType() {
  return rnd.nextBool() ? VariableType.bool : VariableType.string;
}

enum ParameterSource { state, input }

class Parameter {
  // bools are used to control visibility of the UI (e.g. using `if`).
  // string are used to display data.
  final VariableType type;
  final String name;
  final ParameterSource source;

  String get typeName => '$type'.substring('$type'.indexOf('.') + 1);

  Parameter({
    @required this.type,
    @required this.name,
    @required this.source,
  });
}

/// An app.
class App {
  /// All widgets defined in that app.
  final List<Widget> widgets;

  /// Top-level app widget.
  final Widget rootWidget;

  App({
    @required this.rootWidget,
    @required this.widgets,
  });

  @override
  String toString() {
    var buf = new StringBuffer();
    buf.writeln('App {');
    buf.writeln('  root: ${rootWidget.metadata.name}');
    for (Widget widget in widgets) {
      buf.writeln(indent(widget));
    }
    buf.writeln('}');
    return buf.toString();
  }
}

String indent(Object o) {
  return '$o'.split('\n').map((l) => '  $l').join('\n');
}

class ComponentMetadata {
  final String name;
  final List<Parameter> parameters;

  Iterable<Parameter> get states =>
      parameters.where((p) => p.source == ParameterSource.state);

  Iterable<Parameter> get inputs =>
      parameters.where((p) => p.source == ParameterSource.input);

  bool get isStateful => states.isNotEmpty;

  bool get hasStringParameters =>
      parameters.any((p) => p.type == VariableType.string);

  bool get hasBoolParameters =>
      parameters.any((p) => p.type == VariableType.bool);

  Parameter randomParameter({@required VariableType ofType}) {
    var params = parameters.where((p) => p.type == ofType).toList();
    if (params.isEmpty) {
      return null;
    }
    return params[rnd.nextInt(params.length)];
  }

  ComponentMetadata({
    @required this.name,
    @required this.parameters,
  });

  @override
  String toString() {
    var buf = new StringBuffer();
    if (inputs.isNotEmpty) {
      buf.writeln('// Inputs');
    }
    for (Parameter input in inputs) {
      buf.writeln('${input.name}: ${input.typeName}');
    }
    if (states.isNotEmpty) {
      buf.writeln('// States');
    }
    for (Parameter state in states) {
      buf.writeln('${state.name}: ${state.typeName}');
    }
    return buf.toString();
  }
}

abstract class Binding {
  final VariableType type;
  Binding(this.type);
}

class Literal extends Binding {
  final dynamic value;

  Literal.bool(bool value)
      : value = value,
        super(VariableType.bool);

  Literal.string(String value)
      : value = value,
        super(VariableType.string);

  @override
  String toString() => type == VariableType.string ? "'$value'" : '$value';
}

class Expression extends Binding {
  final Parameter field;

  Expression(Parameter field) : field = field, super(field.type) {
    check(field != null, 'field is null');
  }

  @override
  String toString() => field.name;
}

class Attribute {
  final String name;
  final Binding binding;
  Attribute(this.name, this.binding);
}

abstract class TemplateNode {
  static int totalNodeCount = 0;
  static int conditionalCount = 0;

  List<TemplateNode> children;
  Expression conditional;

  TemplateNode({@required this.children, @required this.conditional}) {
    totalNodeCount++;
    conditionalCount++;
  }

  @override
  String toString() {
    var buf = new StringBuffer();
    for (var child in children) {
      buf.writeln(child);
    }
    return buf.toString();
  }
}

class ElementNode extends TemplateNode {
  final String tag;
  final List<Attribute> attrs;
  final List<String> classes;
  final String text;

  ElementNode({
    @required this.tag,
    @required this.attrs,
    @required this.classes,
    @required List<TemplateNode> children,
    @required Expression conditional,
  })
      : text = children.isEmpty ? randomShortString() : null,
        super(children: children, conditional: conditional);

  @override
  String toString() {
    var buf = new StringBuffer();
    buf.write('<$tag');
    for (Attribute attr in attrs) {
      buf.write(' ${attr.name}="${attr.binding}"');
    }
    buf.writeln('>');
    if (text != null) {
      buf.writeln(indent(text));
    }
    buf.writeln(indent(super.toString()));
    buf.writeln('</$tag>');

    if (conditional == null) {
      return buf.toString();
    } else {
      var conditionBlock = new StringBuffer();
      conditionBlock.writeln('if (${conditional.field.name}) {');
      conditionBlock.writeln(indent(buf));
      conditionBlock.writeln('}');
      return conditionBlock.toString();
    }
  }
}

class WidgetNode extends TemplateNode {
  final Widget widget;
  final List<Attribute> args;
  final String text;

  WidgetNode({
    @required this.widget,
    @required this.args,
    @required List<TemplateNode> children,
    @required Expression conditional,
  })
      : text = children.isEmpty && widget.hasContent ? randomShortString() : null,
        super(children: children, conditional: conditional) {
    for (Attribute arg in args) {
      bool foundMatchingInput = false;
      for (Parameter input in widget.metadata.inputs) {
        if (input.name == arg.name) {
          if (input.type != arg.binding.type) {
            throw 'Argument type ${arg.binding.type} != input type ${input.type}';
          }
          foundMatchingInput = true;
          break;
        }
      }
      if (!foundMatchingInput) {
        throw 'Invalid argument ${arg.name} passed to widget ${widget.metadata.name}';
      }
    }
  }

  @override
  String toString() {
    var buf = new StringBuffer();
    String tag = widget.metadata.name;
    buf.write('<$tag');
    for (Attribute arg in args) {
      buf.write(' ${arg.name}="${arg.binding}"');
    }
    buf.writeln('>');
    if (text != null) {
      buf.writeln(indent(text));
    }
    buf.writeln(indent(super.toString()));
    buf.writeln('</$tag>');

    if (conditional == null) {
      return buf.toString();
    } else {
      var conditionBlock = new StringBuffer();
      conditionBlock.writeln('if (${conditional.field.name}) {');
      conditionBlock.writeln(indent(buf));
      conditionBlock.writeln('}');
      return conditionBlock.toString();
    }
  }
}

// Placeholder for children passed by parent (a la <ng-content>).
class ContentNode extends TemplateNode {
  ContentNode() : super(children: const [], conditional: null);

  @override
  String toString() => '<content>';
}

// A component with a child.
class Widget {
  final bool hasContent;
  final ComponentMetadata metadata;
  final TemplateNode template;

  Widget({
    @required this.hasContent,
    @required this.metadata,
    @required this.template,
  }) {
    if (hasContent) {
      reusableWidgets.add(this);
    } else {
      adHocWidgets.add(this);
    }
  }

  bool get isStateful => metadata.isStateful;

  @override
  String toString() {
    var buf = new StringBuffer();
    buf.writeln('${metadata.name} {');
    buf.writeln(indent(metadata));
    buf.writeln('  template:');
    buf.writeln(indent(indent(template)));
    buf.writeln('}');
    return buf.toString();
  }
}

abstract class CodeEmitter {
  String render(App app);
}

final adHocWidgets = <Widget>[];
final reusableWidgets = <Widget>[];
int nextWidgetIndex = 0;

// Picks a random widget from [widgets], or generates a new one.
Widget nextWidget() {
  // Reuse only if we have at least 10 reusable widgets and either we have
  // maxed out on widgets or at 20% rate.
  bool shouldReuse = reusableWidgets.length > 2 * multiplier &&
      (reusableWidgets.length >= 20 * multiplier || rnd.nextInt(10) < 2);
  Widget widget;
  if (shouldReuse) {
    widget = reusableWidgets[nextWidgetIndex];
    nextWidgetIndex = (nextWidgetIndex + 1) % reusableWidgets.length;
  } else {
    var metadata = randomComponentMetadata('Widget');
    widget = new Widget(
      hasContent: true,
      metadata: metadata,
      template: randomTemplateNode(
        depth: 4,
        metadata: metadata,
        withContent: true,
      ),
    );
  }
  return widget;
}

List<Widget> chooseRandomChildWidgets(int min, int max) {
  return new List<Widget>.generate(
    min + rnd.nextInt(max - min),
    (int _) => nextWidget(),
  );
}

List<WidgetNode> randomTabs() {
  return new List.generate(2 * multiplier, (int _) {
    var metadata = randomComponentMetadata('Tab');

    Widget tab = new Widget(
      hasContent: false,
      metadata: metadata,
      template: randomTemplateNode(
        depth: 7,
        metadata: metadata,
        withContent: false,
      ),
    );

    return new WidgetNode(
      widget: tab,
      children: [],
      args: [],
      conditional: null,
    );
  });
}

List<WidgetNode> randomScreens() {
  return new List.generate(2 * multiplier, (int _) {
    ElementNode screenTemplate = new ElementNode(
      tag: 'div',
      attrs: [],
      classes: [],
      children: randomTabs(),
      conditional: null,
    );

    Widget screen = new Widget(
      hasContent: false,
      metadata: randomComponentMetadata('Screen'),
      template: screenTemplate,
    );

    return new WidgetNode(
      widget: screen,
      children: [],
      args: [],
      conditional: null,
    );
  });
}

App generateApp() {
  ElementNode appTemplate = new ElementNode(
    tag: 'div',
    attrs: [],
    classes: [],
    children: randomScreens(),
    conditional: null,
  );

  ComponentMetadata metadata = randomComponentMetadata('Root');
  Widget rootWidget = new Widget(
    hasContent: false,
    metadata: metadata,
    template: appTemplate,
  );

  return new App(
    widgets: []..addAll(reusableWidgets)..addAll(adHocWidgets),
    rootWidget: rootWidget,
  );
}

String randomTag() {
  const tags = const ['div', 'span', 'p', 'a'];
  return tags[rnd.nextInt(tags.length)];
}

String randomAttrName() {
  const attrs = const ['id', 'title', 'href', 'width', 'height', 'alt'];
  return attrs[rnd.nextInt(attrs.length)];
}

List<Attribute> randomAttrs(ComponentMetadata metadata) {
  var attrs = <Attribute>[];
  var count = rnd.nextInt(5);
  for (int i = 0; i < count; i++) {
    String name = randomAttrName();
    if (rnd.nextBool() && metadata.hasStringParameters) {
      attrs.add(new Attribute(
        name,
        new Expression(metadata.randomParameter(ofType: VariableType.string)),
      ));
    } else {
      attrs.add(new Attribute(
        name,
        new Literal.string('${rnd.nextInt(100)}'),
      ));
    }
  }
  return attrs;
}

List<String> randomClasses() {
  return new List.generate(
    rnd.nextInt(2),
    (int _) => randomShortString(),
  );
}

List<TemplateNode> randomChildren(int level, ComponentMetadata metadata) {
  int childCount = rnd.nextInt(level);
  var children = new List.generate(childCount, (int _) {
    if (level <= 1) {
      return randomElementNode(level - 1, metadata);
    } else {
      return randomTemplateNode(
          depth: level - 1, metadata: metadata, withContent: false);
    }
  });
  return children;
}

Expression randomConditional(ComponentMetadata metadata) {
  if (rnd.nextInt(10) < 3) {
    Parameter boolParam = metadata.randomParameter(ofType: VariableType.bool);
    if (boolParam != null) {
      return new Expression(boolParam);
    }
  }
  return null;
}

ElementNode randomElementNode(int level, ComponentMetadata metadata) {
  return new ElementNode(
    tag: randomTag(),
    attrs: randomAttrs(metadata),
    classes: randomClasses(),
    children: randomChildren(level, metadata),
    conditional: randomConditional(metadata),
  );
}

WidgetNode randomWidgetNode(int level, ComponentMetadata metadata) {
  Widget widget = nextWidget();
  var args = <Attribute>[];
  for (Parameter input in widget.metadata.inputs) {
    VariableType inputType = input.type;

    // Should we set it?
    if (rnd.nextInt(10) < 7) {
      // Should we set it from a parameter expression
      if (rnd.nextBool() && inputType == VariableType.bool && metadata.hasBoolParameters) {
        args.add(new Attribute(
          input.name,
          new Expression(metadata.randomParameter(ofType: VariableType.bool)),
        ));
      } else if (rnd.nextBool() && inputType == VariableType.string && metadata.hasStringParameters) {
        args.add(new Attribute(
          input.name,
          new Expression(metadata.randomParameter(ofType: VariableType.string)),
        ));
      } else {
        args.add(new Attribute(
          input.name,
          inputType == VariableType.string
            ? new Literal.string('${rnd.nextInt(100)}')
            : new Literal.bool(rnd.nextBool()),
        ));
      }
    }
  }
  return new WidgetNode(
    widget: widget,
    children: randomChildren(level, metadata),
    args: args,
    conditional: randomConditional(metadata),
  );
}

TemplateNode randomTemplateNode({
  @required int depth,
  @required ComponentMetadata metadata,
  @required bool withContent,
}) {
  TemplateNode template;

  if (withContent) {
    template = randomElementNode(depth, metadata);
    template.children.add(new ContentNode());
  } else {
    template = rnd.nextBool()
        ? randomElementNode(depth, metadata)
        : randomWidgetNode(depth, metadata);
  }

  return template;
}

int _componentCounter = 1;

ComponentMetadata randomComponentMetadata(String namePrefix) {
  if (_componentCounter > 5000) {
    throw 'Too many components generated: >1000';
  }
  int i = _componentCounter++;

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

  final parameters = <Parameter>[];
  for (int i = 0; i < stateCount; i++) {
    parameters.add(new Parameter(
      type: randomVariableType(),
      name: 'state${i}',
      source: ParameterSource.state,
    ));
  }

  for (int i = 0; i < inputCount; i++) {
    parameters.add(new Parameter(
      type: randomVariableType(),
      name: 'input${i}',
      source: ParameterSource.input,
    ));
  }

  return new ComponentMetadata(
    name: '${namePrefix}${i}',
    parameters: parameters,
  );
}

void check(bool c, String message) {
  if (!c) {
    throw 'Check failed: $message';
  }
}
