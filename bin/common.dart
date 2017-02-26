import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:process/process.dart';

final pm = new LocalProcessManager();
String optimizerLevel = '3';

Future<Null> gzip(String file) async {
  await exec('gzip', ['-k', '-f', file]);
}

Future<Null> cc(dynamic source, String output,
    {List<String> exportedFunctions: const <String>[]}) async {
  var args = <String>[
    '-std=c++11',
    '-O${optimizerLevel}',
    '-s',
    'NO_EXIT_RUNTIME=1',
    '-s',
    'WASM=1',
    '-s',
    'TOTAL_MEMORY=${16777216 * 32}', // ~500MB
  ];

  if (optimizerLevel == '0' || optimizerLevel == '1') {
    args.addAll([
      '-s',
      'DEMANGLE_SUPPORT=1',
      '-s',
      'ASSERTIONS=2',
    ]);
  }

  if (source is String) {
    args.add(source);
  } else if (source is Iterable) {
    source.forEach(args.add);
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
