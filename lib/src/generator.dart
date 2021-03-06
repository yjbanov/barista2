import 'dart:math' as math;

import 'package:meta/meta.dart';

int multiplier = 5;

final math.Random rnd = new math.Random(123456);

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

enum ValueSource { state, input }

class Variable {
  // bools are used to control visibility of the UI (e.g. using `if`).
  // string are used to display data.
  final VariableType type;
  final String name;
  final initialValue;

  String get typeName => '$type'.substring('$type'.indexOf('.') + 1);

  Variable({
    @required this.type,
    @required this.name,
    @required this.initialValue,
  });
}

class Field {
  static int fieldCount = 0;

  final Variable variable;
  final ValueSource source;

  Field({
    @required this.variable,
    @required this.source,
  }) {
    fieldCount++;
  }
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
  final List<Field> fields;

  Iterable<Variable> get states =>
      fields.where((p) => p.source == ValueSource.state).map((p) => p.variable);

  Iterable<Variable> get inputs =>
      fields.where((p) => p.source == ValueSource.input).map((p) => p.variable);

  bool get isStateful => states.isNotEmpty;

  ComponentMetadata({
    @required this.name,
    @required this.fields,
  });

  @override
  String toString() {
    var buf = new StringBuffer();
    if (inputs.isNotEmpty) {
      buf.writeln('// Inputs');
    }
    for (Variable input in inputs) {
      buf.writeln('${input.name}: ${input.typeName}');
    }
    if (states.isNotEmpty) {
      buf.writeln('// States');
    }
    for (Variable state in states) {
      buf.writeln('${state.name}: ${state.typeName}');
    }
    return buf.toString();
  }
}

abstract class Binding {
  final VariableType type;
  Binding(this.type);
  List<Field> get references;
}

class Literal extends Binding {
  final dynamic value;

  Literal.bool(bool value)
      : value = value,
        super(VariableType.bool);

  Literal.string(String value)
      : value = value,
        super(VariableType.string);

  List<Field> get references => const [];

  @override
  String toString() => type == VariableType.string ? "'$value'" : '$value';
}

class Expression extends Binding {
  final Field field;

  Expression(Field field)
      : field = field,
        super(field.variable.type) {
    check(field != null, 'field is null');
  }

  List<Field> get references => [field];

  @override
  String toString() => field.variable.name;
}

class Attribute {
  final String name;
  final Binding binding;
  Attribute(this.name, this.binding);
}

abstract class TemplateNode {
  static int totalNodeCount = 0;
  static int conditionalCount = 0;
  static final Map<Type, int> nodeTypeCounts = {};

  static int _keyCounter = 0;
  final String key = '${_keyCounter++}';

  List<TemplateNode> children;
  Expression conditional;

  TemplateNode({@required this.children, @required this.conditional}) {
    totalNodeCount++;
    if (conditional != null) conditionalCount++;
    nodeTypeCounts[this.runtimeType] ??= 0;
    nodeTypeCounts[this.runtimeType]++;
  }

  List<Field> get references =>
      conditional != null ? [conditional.field] : const [];

  void scan(void callback(TemplateNode node)) {
    callback(this);
    children.forEach((c) {
      c.scan(callback);
    });
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
  List<Field> get references {
    var result = <Field>[];
    result.addAll(super.references);
    for (Attribute attr in attrs) {
      result.addAll(attr.binding.references);
    }
    return result;
  }

  @override
  String toString() {
    var buf = new StringBuffer();
    buf.write('<$tag _bkey="$key"');
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
      conditionBlock.writeln('if (${conditional.field.variable.name}) {');
      conditionBlock.writeln(indent(buf));
      conditionBlock.writeln('}');
      return conditionBlock.toString();
    }
  }
}

class WidgetNode extends TemplateNode {
  /// The widet this node instantiates.
  final Widget widget;

  /// Arguments passed to the widget as inputs.
  final List<Attribute> args;
  final String text;

  WidgetNode({
    @required this.widget,
    @required this.args,
    @required List<TemplateNode> children,
    @required Expression conditional,
  })
      : text =
            children.isEmpty && widget.hasContent ? randomShortString() : null,
        super(children: children, conditional: conditional) {
    for (Attribute arg in args) {
      bool foundMatchingInput = false;
      for (Variable input in widget.metadata.inputs) {
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
  List<Field> get references {
    var result = <Field>[];
    result.addAll(super.references);
    for (Attribute arg in args) {
      result.addAll(arg.binding.references);
    }
    return result;
  }

  @override
  String toString() {
    var buf = new StringBuffer();
    String tag = widget.metadata.name;
    buf.write('<$tag _bkey="$key"');
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
      conditionBlock.writeln('if (${conditional.field.variable.name}) {');
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
  String toString() => '<content _bkey="$key">';
}

// A button that flips a boolean field upon click.
class ToggleButtonNode extends TemplateNode {
  final Field controls;

  String get text => 'Flip ${controls.variable.name}';

  ToggleButtonNode({@required Field this.controls})
      : super(
          children: const [],
          conditional: null,
        );

  @override
  String toString() {
    var buf = new StringBuffer();
    buf.write('<button _bkey="$key"');
    buf.writeln('>');
    if (text != null) {
      buf.writeln(indent(text));
    }
    buf.writeln(indent(super.toString()));
    buf.writeln('</button>');

    return buf.toString();
  }
}

// A component with a child.
class Widget {
  static int _componentCounter = 0;
  static final adHocWidgets = <Widget>[];
  static int nextReusableWidgetIndex = 0;

  /// The count is incremented prior to adding a reusable widget in the list.
  ///
  /// This is because we want to use the count to stop generating new widgets
  /// when we have reached the limit, however, we do not want to add a widget
  /// in the list during generation to avoid cycles.
  static int reusableWidgetCount = 0;
  static final reusableWidgets = <Widget>[];

  final bool hasContent;
  final ComponentMetadata metadata;
  final TemplateNode template;

  Widget({
    @required String namePrefix,
    @required this.hasContent,
    @required TemplateNode template,
  })
      : template = template,
        metadata = _generateComponentMetadata(
          '${namePrefix}${_componentCounter++}',
          from: template,
        ) {
    if (_componentCounter > 5000) {
      throw 'Too many widgets generated: ${_componentCounter} widgets';
    }

    if (hasContent) {
      reusableWidgets.add(this);
    } else {
      adHocWidgets.add(this);
    }

    // Check template structure
    bool hasContentNode = false;
    template.scan((TemplateNode node) {
      if (node is WidgetNode && node.widget.hasContent) {
        if (node.children.length != 1) {
          throw 'Reusable widget must have content of 1';
        }

        var content = node.children.single;
        if (content.conditional != null) {
          throw 'Reusable widget content must be unconditional';
        }
      }

      if (node is ContentNode) {
        hasContentNode = true;
      }
    });

    if (hasContent && !hasContentNode) {
      throw 'Widget declares to have content but its template is missing a content node';
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

  // TODO: remove the need for separate componentmetadata. Instead, make it a
  // function of TemplateNode.
  static ComponentMetadata _generateComponentMetadata(String name,
      {@required TemplateNode from}) {
    final fields = <Field>[];

    from.scan((TemplateNode node) {
      fields.addAll(node.references);
    });

    return new ComponentMetadata(
      name: name,
      fields: fields,
    );
  }
}

// Picks a random widget from [reusableWidgets], or generates a new one.
Widget nextWidget() {
  // Reuse only if we have at least 10 reusable widgets and either we have
  // maxed out on widgets or at 20% rate.
  bool shouldReuse = Widget.reusableWidgetCount > 2 * multiplier &&
      (Widget.reusableWidgetCount >= 20 * multiplier || rnd.nextInt(10) < 2);
  Widget widget;
  if (shouldReuse) {
    widget = Widget.reusableWidgets[Widget.nextReusableWidgetIndex];
    Widget.nextReusableWidgetIndex =
        (Widget.nextReusableWidgetIndex + 1) % Widget.reusableWidgets.length;
  } else {
    widget = new WidgetGenerator(
      namePrefix: 'Widget',
      generatingStatefulWidget: rnd.nextBool(),
      withContent: true,
    )
        .generate(depth: 3);
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
  return new List.generate(4 * multiplier, (int _) {
    return new WidgetNode(
      widget: new WidgetGenerator(
        namePrefix: 'Tab',
        withContent: false,
        generatingStatefulWidget: false,
      )
          .generate(depth: 5),
      children: [],
      args: [],
      conditional: null,
    );
  });
}

List<WidgetNode> randomScreens() {
  return new List.generate(4 * multiplier, (int _) {
    ElementNode screenTemplate = new ElementNode(
      tag: 'div',
      attrs: [],
      classes: [],
      children: randomTabs(),
      conditional: null,
    );

    Widget screen = new Widget(
      namePrefix: 'Screen',
      hasContent: false,
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

  Widget rootWidget = new Widget(
    namePrefix: 'Root',
    hasContent: false,
    template: appTemplate,
  );

  return new App(
    widgets: []..addAll(Widget.reusableWidgets)..addAll(Widget.adHocWidgets),
    rootWidget: rootWidget,
  );
}

class WidgetGenerator {
  final bool generatingStatefulWidget;
  final String namePrefix;
  final bool withContent;

  WidgetGenerator({
    @required this.generatingStatefulWidget,
    @required this.namePrefix,
    @required this.withContent,
  });

  Widget generate({@required int depth}) {
    Widget.reusableWidgetCount++;
    return new Widget(
      namePrefix: namePrefix,
      hasContent: withContent,
      template: randomTemplate(
        depth: depth,
      ),
    );
  }

  String randomTag() {
    const tags = const ['div', 'span'];
    return tags[rnd.nextInt(tags.length)];
  }

  static String randomDimension() => '${5 + rnd.nextInt(10)}px';

  static const Map<String, Function> _attrGenerators = const {
    'id': randomShortString,
    'title': randomShortString,
    'href': randomShortString,
    'width': randomDimension,
    'height': randomDimension,
    'alt': randomShortString,
  };

  Map<String, String> randomAttrNamesAndDefaults() {
    List<String> attrs = new List.from(_attrGenerators.keys);
    attrs.shuffle(rnd);
    int count = rnd.nextInt(5);
    attrs = attrs.take(count).toList();
    return new Map.fromIterable(
      attrs,
      key: (k) => k,
      value: (k) => _attrGenerators[k](),
    );
  }

  int fieldNameCounter = 1;

  Variable randomVariable({@required VariableType ofType}) {
    var initialValue = ofType == VariableType.bool
      ? rnd.nextBool()
      : randomShortString();
    return new Variable(
      name: 'field${fieldNameCounter++}',
      type: ofType,
      initialValue: initialValue,
    );
  }

  Field randomField({@required VariableType ofType}) {
    return new Field(
      variable: randomVariable(ofType: ofType),
      source: generatingStatefulWidget && rnd.nextBool()
          ? ValueSource.state
          : ValueSource.input,
    );
  }

  List<Attribute> randomAttrs() {
    var attrs = <Attribute>[];
    randomAttrNamesAndDefaults().forEach((String name, String defaultValue) {
      // Expression or literal?
      if (rnd.nextBool()) {
        attrs.add(new Attribute(
          name,
          new Expression(randomField(ofType: VariableType.string)),
        ));
      } else {
        attrs.add(new Attribute(
          name,
          new Literal.string(defaultValue),
        ));
      }
    });
    return attrs;
  }

  List<String> randomClasses() {
    return new List.generate(
      rnd.nextInt(2),
      (int _) => randomShortString(),
    );
  }

  TemplateNode randomChild({@required int depth, bool unconditional: false}) {
    if (depth <= 1) {
      return randomElementNode(depth: depth, unconditional: unconditional);
    } else {
      return randomTemplateNode(depth: depth, unconditional: unconditional);
    }
  }

  List<TemplateNode> randomChildren({@required int depth}) {
    int childCount = rnd.nextInt(depth);
    var children = new List.generate(childCount, (int _) {
      return randomChild(depth: depth - 1);
    });
    return children;
  }

  Expression randomConditional(bool unconditional) {
    if (!unconditional && rnd.nextInt(10) < 4) {
      return new Expression(randomField(ofType: VariableType.bool));
    }
    return null;
  }

  ElementNode randomElementNode(
      {@required int depth, bool unconditional: false}) {
    return new ElementNode(
      tag: randomTag(),
      attrs: randomAttrs(),
      classes: randomClasses(),
      children: randomChildren(depth: depth),
      conditional: randomConditional(unconditional),
    );
  }

  Literal randomLiteral({@required VariableType ofType}) {
    return ofType == VariableType.string
        ? new Literal.string('${rnd.nextInt(100)}')
        : new Literal.bool(rnd.nextBool());
  }

  WidgetNode randomWidgetNode(
      {@required int depth, bool unconditional: false}) {
    Widget widget = nextWidget();
    var args = <Attribute>[];

    for (Variable input in widget.metadata.inputs) {
      VariableType inputType = input.type;
      bool shouldSet = rnd.nextInt(10) < 7;

      // Should we set it?
      if (shouldSet) {
        bool shouldSetFromLiteral = rnd.nextBool();

        if (shouldSetFromLiteral) {
          args.add(new Attribute(
            input.name,
            randomLiteral(ofType: inputType),
          ));
        } else {
          args.add(new Attribute(
            input.name,
            new Expression(randomField(ofType: inputType)),
          ));
        }
      }
    }

    return new WidgetNode(
      widget: widget,
      children: [randomChild(depth: depth, unconditional: true)],
      args: args,
      conditional: randomConditional(unconditional),
    );
  }

  TemplateNode randomTemplate({@required int depth}) {
    List<TemplateNode> children = [];

    TemplateNode template = randomTemplateNode(depth: depth - 1);
    children.add(template);

    if (withContent) {
      children.add(new ContentNode());
    }

    template.scan((node) {
      if (node.conditional != null && node.conditional.field.source == ValueSource.state) {
        if (node.conditional.field.variable.type != VariableType.bool) {
          throw 'Conditional expression must be bool';
        }
        children.add(new ToggleButtonNode(controls: node.conditional.field));
      }
    });

    return new ElementNode(
      tag: 'div',
      attrs: randomAttrs()..add(new Attribute('widget', new Literal.string('${namePrefix}${Widget._componentCounter}'))),
      classes: randomClasses(),
      children: children,
      conditional: null,
    );
  }

  TemplateNode randomTemplateNode({
    @required int depth,
    bool unconditional: false,
  }) {
    return rnd.nextBool()
        ? randomElementNode(depth: depth, unconditional: unconditional)
        : randomWidgetNode(depth: depth, unconditional: unconditional);
  }
}

void check(bool c, String message) {
  if (!c) {
    throw 'Check failed: $message';
  }
}
