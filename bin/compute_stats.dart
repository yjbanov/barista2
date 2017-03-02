import 'dart:async';
import 'dart:io';

import 'common.dart';

StringBuffer buf = new StringBuffer();

Future<Null> main(List<String> rawArgs) async {
  await stats('ng2dart', 'web/giant_widgets.dart', 'build/web/giant.dart.js');
  await stats('ng2ts', 'others/ng2ts/giant/src/app/app.component.ts', 'others/ng2ts/giant/dist/', ls: true);
  await stats('inferno', 'others/inferno/src/components/components.tsx', 'others/inferno/dist/', ls: true);
  await stats('wasm', 'giant_widgets.h', 'giant.wasm');
  print(buf);
}

Future<Null> stats(String name, String source, String path, {bool ls: false}) async {
  List<File> files = [];
  if (ls) {
    new Directory(path).listSync().where((f) => f is File && f.path.endsWith('.js')).forEach((f) => files.add(f as File));
  } else {
    files.add(new File(path));
  }

  int sourceSize = new File(source).lengthSync();
  int compiledSize = 0;
  int gzippedSize = 0;
  for (File compiled in files) {
    await gzip(compiled.path);
    var gzipped = new File('${compiled.path}.gz');
    compiledSize += compiled.lengthSync();
    gzippedSize += gzipped.lengthSync();
  }
  buf.writeln('$name: ${sourceSize} source, ${compiledSize} compiled, ${gzippedSize} compressed');
}
