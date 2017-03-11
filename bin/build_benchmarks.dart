import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';
import 'package:meta/meta.dart';

import 'common.dart';

int minMultiplier = 4;
int maxMultiplier = 4;

ArgParser argParser = new ArgParser()
  ..addOption('min', callback: (v) {
    if (v == null) return;
    minMultiplier = int.parse(v);
  })
  ..addOption('max', callback: (v) {
    if (v == null) return;
    maxMultiplier = int.parse(v);
  });

Future<Null> main(List<String> rawArgs) async {
  argParser.parse(rawArgs);

  Directory rootDist = await recreateDir('dist');
  await cp('web/main.css', rootDist.path);
  await cp('sync.js', rootDist.path);
  await cp('util.js', rootDist.path);

  var sourceSizeTrend = new Trend('Source size');
  var compiledSizeTrend = new Trend('Compiled size');
  var compressedSizeTrend = new Trend('Compressed size');

  for (int m = minMultiplier; m <= maxMultiplier; m++) {
    Directory dist = await recreateDir('${rootDist.path}/$m');

    print('Building benchmark with multiplier $m');
    await exec('dart', ['bin/make_giant.dart', '-m', '$m']);

    List<BuildResult> builds = await Future.wait(<Future<BuildResult>>[
      _buildWasm(),
      _buildNative(),
      _buildNg2Dart(),
      _buildNg2Ts(),
      _buildInferno(),
    ]);

    var statsOut = f('${dist.path}/stats.txt').openWrite();
    statsOut.writeln('\tSource\tCompiled\tCompressed');
    for (var build in builds) {
      Directory frameworkDir = await recreateDir('${dist.path}/${build.framework}');
      for (File artifact in build.files) {
        await cp(artifact.path, frameworkDir.path);
      }
      statsOut.writeln('${build.framework}\t${build.sourceSize}\t${build.compiledSize}\t${build.compressedSize}');
      sourceSizeTrend.add(build.framework, build.sourceSize);
      compiledSizeTrend.add(build.framework, build.compiledSize);
      compressedSizeTrend.add(build.framework, build.compressedSize);
    }
    await statsOut.close();
  }
  var globalStats = f('${rootDist.path}/stats.txt').openWrite();
  sourceSizeTrend.print(globalStats);
  compiledSizeTrend.print(globalStats);
  compressedSizeTrend.print(globalStats);
  await globalStats.close();
}

class Trend {
  final String title;
  final Map<String, List<int>> _line = {};

  Trend(this.title);

  void add(String framework, int value) {
    _line.putIfAbsent(framework, () => <int>[]);
    _line[framework].add(value);
  }

  void print(IOSink destination) {
    destination.writeln(title);
    _line.forEach((String framework, List<int> values) {
      destination
        ..write(framework)
        ..write('\t');
      for (int value in values) {
        destination
          ..write(value)
          ..write('\t');
      }
      destination.writeln();
    });
  }
}

class BuildResult {
  final String framework;
  final List<File> files;
  final int compiledSize;
  final int compressedSize;
  final int sourceSize;

  BuildResult({
    @required this.framework,
    @required this.files,
    @required this.compiledSize,
    @required this.compressedSize,
    @required this.sourceSize,
  });
}

Future<BuildResult> _buildWasm() async {
  await exec('dart', ['bin/build_wasm.dart', '--optimizer-level=3']);
  var wasm = f('giant.wasm');
  return new BuildResult(
    framework: 'wasm',
    files: [
      f('giant.js'),
      wasm,
      f('index.html'),
    ],
    sourceSize: await computeSourceSize(f('giant_widgets.h')),
    compiledSize: await wasm.length(),
    compressedSize: await computeCompressedSize(wasm),
  );
}

Future<BuildResult> _buildNative() async {
  await exec('cmake', ['-DCMAKE_BUILD_TYPE=Release']);
  await exec('make', ['test_giant']);
  var executable = f('test_giant');
  return new BuildResult(
    framework: 'native',
    files: [
      executable,
    ],
    sourceSize: await computeSourceSize(f('giant_widgets.h')),
    compiledSize: await executable.length(),
    compressedSize: await computeCompressedSize(executable),
  );
}

Future<BuildResult> _buildNg2Dart() async {
  await exec('pub', ['build']);
  var jsFile = f('build/web/giant.dart.js');
  return new BuildResult(
    framework: 'ng2dart',
    files: await lsFiles('build/web'),
    sourceSize: await computeSourceSize(f('web/giant_widgets.dart')),
    compiledSize: await jsFile.length(),
    compressedSize: await computeCompressedSize(jsFile),
  );
}

Future<BuildResult> _buildNg2Ts() async {
  await exec('./build', [], workingDirectory: 'others/ng2ts/giant');
  var artifacts = await lsFiles('others/ng2ts/giant/dist');
  return new BuildResult(
    framework: 'ng2ts',
    files: artifacts,
    sourceSize: await sum(computeSourceSize, [
      f('others/ng2ts/giant/src/app/app.component.ts'),
      f('others/ng2ts/giant/src/app/app.module.ts'),
    ]),
    compiledSize: await sum(fileSize, artifacts),
    compressedSize: await sum(computeCompressedSize, artifacts),
  );
}

Future<int> fileSize(File f) => f.length();

Future<BuildResult> _buildInferno() async {
  await recreateDir('others/inferno/dist');
  await exec('webpack', ['-p'], workingDirectory: 'others/inferno');
  var bundle = f('others/inferno/dist/bundle.js');
  return new BuildResult(
    framework: 'inferno',
    files: await lsFiles('others/inferno/dist'),
    sourceSize: await computeSourceSize(f('others/inferno/src/components/components.tsx')),
    compiledSize: await bundle.length(),
    compressedSize: await computeCompressedSize(bundle),
  );
}

Future<int> sum(Future<int> computeFn(File f), List<File> files) async {
  int total = 0;
  for (var f in files) {
    total += await computeFn(f);
  }
  return total;
}

Future<int> computeCompressedSize(File f) async {
  return new GZipCodec(level: ZLibOption.MAX_LEVEL)
      .encode(await f.readAsBytes())
      .length;
}

Future<int> computeSourceSize(File f) async {
  return (await f.readAsString())
      .split('\n')
      .map((l) => l.trim())
      .where((l) => l.isNotEmpty)
      .fold(0, (int prev, String line) => prev + line.length);
}
