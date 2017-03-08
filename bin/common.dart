import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as path;
import 'package:process/process.dart';

final pm = new LocalProcessManager();
String optimizerLevel = '3';

File f(String path) => new File(path);
Directory d(String path) => new Directory(path);

/// Deletes a directory (if exists), and create a new empty one.
///
/// Deleting and creation are done recursively.
Future<Directory> recreateDir(String path) async {
  var dir = new Directory(path);
  if (await dir.exists()) {
    await dir.delete(recursive: true);
  }
  await dir.create(recursive: true);
  return dir;
}

/// Non-recursively lists files in the given directory.
Future<List<File>> lsFiles(String dir) async {
  return (await new Directory(dir).list().toList()).where((e) => e is File).toList();
}

Future<File> cp(String file, String dir) async {
  var f = new File(file);
  var d = new Directory(dir);
  if (!(await d.exists())) {
    throw 'Directory does not exist $dir';
  }
  var target = '${d.path}/${path.basename(f.path)}';
  await f.copy(target);
  return new File(target);
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
  String workingDirectory,
}) async {
  Process process =
  await startProcess(
    executable,
    arguments,
    environment: environment,
    workingDirectory: workingDirectory,
  );

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
    throw 'Executable failed with exit code $exitCode: $executable ${arguments?.join(" ") ?? ""}';

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
