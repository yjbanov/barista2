import 'dart:async';

import 'dart:convert';
import 'dart:io';
import 'package:args/args.dart';
import 'package:process/process.dart';

final pm = new LocalProcessManager();
bool compileToWasm = false;
int optimizerLevel = 3;

Future<Null> main(List<String> rawArgs) async {
  var argParser = new ArgParser()
    ..addFlag('wasm')
    ..addOption('optimizer-level', abbr: 'O', callback: (String v) {
      if (v != null) {
        optimizerLevel = int.parse(v);
      }
    });
  var args = argParser.parse(rawArgs);

  compileToWasm = args['wasm'];

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

  await gzip('main.js');
  if (compileToWasm) {
    await gzip('main.wasm');
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

  await gzip('test_all.js');
  if (compileToWasm) {
    await gzip('test_all.wasm');
  }
}

Future<Null> gzip(String file) async {
  await exec('gzip', ['-k', '-f', file]);
}

Future<Null> cc(dynamic source, String output,
    {List<String> exportedFunctions: const <String>[]}) async {
  var args = <String>[
    '-std=c++11',
    '-O${optimizerLevel}',
    '-s',
    'ASSERTIONS=1',
    '-s',
    'NO_EXIT_RUNTIME=1',
  ];

  if (compileToWasm) {
    args.addAll([
      '-s',
      'WASM=1',
    ]);
  }

  if (source is String) {
    args.add(source);
  } else if (source is Iterable) {
    args.addAll(source);
  } else {
    throw '`source` must be either String or Iterable';
  }

  args.addAll([
    '-o',
    output,
  ]);

  if (exportedFunctions.isNotEmpty) {
    args.add('-s');
    var fmtExportedFunctions = exportedFunctions.map((f) => "'$f'").join(', ');
    args.add('EXPORTED_FUNCTIONS=[${fmtExportedFunctions}]');
  }

  await exec('emcc', args);
}

Future<Process> startProcess(
  String executable,
  List<String> arguments, {
  Map<String, String> environment,
  String workingDirectory,
}) async {
  String command = '$executable ${arguments?.join(" ") ?? ""}';
  print('Executing: $command');
  environment ??= <String, String>{};
  environment['BOT'] = 'true';
  Process process = await Process.start(
    executable,
    arguments,
    environment: environment,
    workingDirectory: workingDirectory,
  );

  return process;
}

/// Executes a command and returns its exit code.
Future<int> exec(
  String executable,
  List<String> arguments, {
  Map<String, String> environment,
  bool canFail: false,
}) async {
  Process process =
      await startProcess(executable, arguments, environment: environment);

  process.stdout
      .transform(UTF8.decoder)
      .transform(const LineSplitter())
      .listen(print);
  process.stderr
      .transform(UTF8.decoder)
      .transform(const LineSplitter())
      .listen(stderr.writeln);

  int exitCode = await process.exitCode;

  if (exitCode != 0 && !canFail)
    throw 'Executable failed with exit code $exitCode.';

  return exitCode;
}

/// Executes a command and returns its standard output as a String.
///
/// Standard error is redirected to the current process' standard error stream.
Future<String> eval(
  String executable,
  List<String> arguments, {
  Map<String, String> environment,
  bool canFail: false,
}) async {
  Process process =
      await startProcess(executable, arguments, environment: environment);
  process.stderr.listen((List<int> data) {
    stderr.add(data);
  });
  String output = await UTF8.decodeStream(process.stdout);
  int exitCode = await process.exitCode;

  if (exitCode != 0 && !canFail)
    throw 'Executable failed with exit code $exitCode.';

  return output.trimRight();
}
