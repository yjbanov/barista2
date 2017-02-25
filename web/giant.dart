import 'dart:async';
import 'dart:html' as html;
import 'dart:js' as js;

import 'package:angular2/angular2.dart';
import 'package:angular2/platform/browser.dart';
import 'giant_widgets.dart';

@Component(
  selector: 'giant-app-wrapper',
  directives: const [NgIf, GiantApp],
  template:
'''
<button (click)="toggle()">Toggle visibility</button>
<div *ngIf="visible">
  <giant-app></giant-app>
</div>
'''
)
class GiantAppWrapper {
  bool visible = true;
  void toggle() {
    var before = html.window.performance.now();
    visible = !visible;
    Timer.run(() {
      var after = html.window.performance.now();
      print('>>> Render frame: ${after - before} ms');
    });
  }
}

main() async {
  if (0 is double) js.context.callMethod('inMain');
  await new Future.delayed(new Duration(milliseconds: 50));

  var bootStart = html.window.performance.now();
  await bootstrap(GiantAppWrapper);
  var bootEnd = html.window.performance.now();
  print('>>> Bootstrap time: ${bootEnd - bootStart} ms');
}
