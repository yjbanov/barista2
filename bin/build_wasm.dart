import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';
import 'common.dart';

Future<Null> main(List<String> rawArgs) async {
  var argParser = new ArgParser()
    ..addOption('optimizer-level', abbr: 'O', callback: (String v) {
      if (v != null) {
        optimizerLevel = v;
        const validOptmizerLevels = const ['0', '1', '2', '3', 's'];
        if (!validOptmizerLevels.contains(optimizerLevel)) {
          throw 'Invalid --optimizer-level. Must be one of ${validOptmizerLevels.join(', ')}';
        }
      }
    });
  argParser.parse(rawArgs);

  print('Building using ${await eval('which', ['emcc'])}');

  // Compiler libs
  await cc('lib/json/src/json.hpp', 'json.bc');
  await cc('sync.cpp', 'sync.bc');
  await cc('api.cpp', 'api.bc');
  await cc('style.cpp', 'style.bc');
  await cc('html.cpp', 'html.bc');

  // Compile sample app
  await cc('main.cpp', 'main.bc');
  await cc(
    [
      'json.bc',
      'sync.bc',
      'api.bc',
      'style.bc',
      'html.bc',
      'main.bc',
    ],
    'main.js',
    exportedFunctions: [
      '_RenderFrame',
      '_DispatchEvent',
      '_main',
    ],
  );

  await gzip('main.asm.js');
  await gzip('main.wasm');

  if (new File('giant.cpp').existsSync()) {
    // Compile sample app
    await cc('giant.cpp', 'giant.bc');
    await cc(
      [
        'json.bc',
        'sync.bc',
        'api.bc',
        'style.bc',
        'html.bc',
        'giant.bc',
      ],
      'giant.js',
      exportedFunctions: [
        '_RenderFrame',
        '_DispatchEvent',
        '_main',
      ],
    );

    await gzip('giant.asm.js');
    await gzip('giant.wasm');
  } else {
    print('SKIPPING: giant.cpp (file not generated)');
  }

  // Compile tests
  await cc('test.cpp', 'test.bc');
  await cc('test_all.cpp', 'test_all.bc');
  await cc(
    [
      'json.bc',
      'sync.bc',
      'api.bc',
      'style.bc',
      'html.bc',
      'test.bc',
      'test_all.bc'
    ],
    'test_all.js',
  );

  await gzip('test_all.asm.js');
  await gzip('test_all.wasm');
}