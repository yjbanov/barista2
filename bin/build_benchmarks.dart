import 'dart:async';
import 'dart:io';

import 'package:args/args.dart';

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

  for (int m = minMultiplier; m <= maxMultiplier; m++) {
    Directory dist = await recreateDir('${rootDist.path}/$m');

    Future<Null> copyArtifacts(String framework, Future<List<File>> artifacts) async {
      Directory frameworkDir = await recreateDir('${dist.path}/$framework');
      for (File artifact in await artifacts) {
        await cp(artifact.path, frameworkDir.path);
      }
    }

    print('Building benchmark with multiplier $m');
    await exec('dart', ['bin/make_giant.dart', '-m', '$m']);
    await Future.wait(<Future>[
      copyArtifacts('wasm', _buildWasm()),
      copyArtifacts('ng2dart', _buildNg2Dart()),
      copyArtifacts('ng2ts', _buildNg2Ts()),
      copyArtifacts('inferno', _buildInferno()),
    ]);
  }
}

Future<List<File>> _buildWasm() async {
  await exec('dart', ['bin/build_wasm.dart', '--optimizer-level=3']);
  return [
    new File('giant.js'),
    new File('giant.wasm'),
    new File('index.html'),
  ];
}

Future<List<File>> _buildNg2Dart() async {
  await exec('pub', ['build']);
  return lsFiles('build/web');
}

Future<List<File>> _buildNg2Ts() async {
  await exec('./build', [], workingDirectory: 'others/ng2ts/giant');
  return await lsFiles('others/ng2ts/giant/dist');
}

Future<List<File>> _buildInferno() async {
  await recreateDir('others/inferno/dist');
  await exec('webpack', ['-p'], workingDirectory: 'others/inferno');
  return await lsFiles('others/inferno/dist');
}
