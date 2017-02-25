import 'dart:async';
import 'dart:html' as html;
import 'dart:js' as js;

import 'package:angular2/platform/browser.dart';
import 'giant_widgets.dart';

main() async {
  js.context.callMethod('inMain');
  await new Future.delayed(new Duration(milliseconds: 50));

  var bootStart = html.window.performance.now();
  await bootstrap(GiantApp);
  var bootEnd = html.window.performance.now();
  print('>>> Bootstrap time: ${bootEnd - bootStart} ms');
}
