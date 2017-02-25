import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';

import 'package:barista2/src/generator.dart';
import 'package:barista2/src/cpp_generator.dart' as cpp;
import 'package:barista2/src/ng2d_generator.dart' as ng2d;

int optimizerLevel = 3;

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
  int totalWidgets = Widget.reusableWidgets.length + Widget.adHocWidgets.length;
  print('Generated:');
  print('  ${totalWidgets} widgets, of which:');
  print('    ${Widget.reusableWidgets.length} reusable');
  print('    ${Widget.adHocWidgets.length} ad hoc');
  print('  ${TemplateNode.totalNodeCount} template nodes, of which:');
  TemplateNode.nodeTypeCounts.forEach((type, count) {
    print('    ${count} ${type} nodes');
  });
  print('  Average nodes/template: ${TemplateNode.totalNodeCount / totalWidgets}');
  print('  ${TemplateNode.conditionalCount} conditionals');
  print('  Average conditionals/template: ${TemplateNode.conditionalCount / totalWidgets}');
  print('  ${Field.fieldCount} fields');
  print('  Average fields/widget: ${Field.fieldCount / totalWidgets}');

  new cpp.CodeEmitter().render(app).forEach((String file, String code) {
    var cppFile = new File(file);
    cppFile.writeAsStringSync(code.toString());
  });

  new ng2d.CodeEmitter().render(app).forEach((String file, String code) {
    var cppFile = new File(file);
    cppFile.writeAsStringSync(code.toString());
  });
}
