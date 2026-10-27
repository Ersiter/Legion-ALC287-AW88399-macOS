#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / '.build'
SOURCES = BUILD / 'sources'
LOCK = json.loads((ROOT / 'dependencies.lock.json').read_text())


def run(*args, cwd=None):
    subprocess.run([str(a) for a in args], cwd=cwd, check=True)


def checkout(name):
    dep = LOCK[name]
    path = SOURCES / name
    if not path.exists():
        run('git', 'init', path)
        run('git', '-C', path, 'remote', 'add', 'origin', dep['repository'])
    head = subprocess.run(['git', '-C', str(path), 'rev-parse', 'HEAD'], capture_output=True, text=True)
    if head.returncode or head.stdout.strip() != dep['commit']:
        run('git', '-C', path, 'fetch', '--depth=1', 'origin', dep['commit'])
        run('git', '-C', path, 'checkout', '--detach', 'FETCH_HEAD')
    assert subprocess.check_output(['git', '-C', str(path), 'rev-parse', 'HEAD'], text=True).strip() == dep['commit']
    if name == 'VoodooI2C':
        run('git', '-C', path, 'submodule', 'update', '--init', '--recursive')
        patch = ROOT / dep['patch']
        applied = subprocess.run(['git', '-C', str(path), 'apply', '--reverse', '--check', str(patch)], capture_output=True)
        if applied.returncode:
            run('git', '-C', path, 'apply', '--check', patch)
            run('git', '-C', path, 'apply', patch)
    return path


def link(target, destination):
    if not destination.exists():
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.symlink_to(target, target_is_directory=True)
    assert destination.resolve() == target.resolve(), destination


def build(name, source, configuration='Release', **settings):
    derived = BUILD / 'products' / name
    args = ['xcodebuild', '-jobs', '4', '-configuration', configuration, '-arch', 'x86_64',
            '-derivedDataPath', derived, 'CODE_SIGNING_ALLOWED=NO',
            'MACOSX_DEPLOYMENT_TARGET=26.0']
    if name == 'VoodooI2C':
        args += ['-workspace', 'VoodooI2C.xcworkspace', '-scheme', 'VoodooI2C']
    else:
        args += ['-project', f'{name}.xcodeproj', '-scheme', name]
    args += [f'{key}={value}' for key, value in settings.items()]
    run(*args, 'build', cwd=source)
    return derived / 'Build/Products' / configuration


def main():
    if sys.platform != 'darwin':
        raise SystemExit('Build on macOS with full Xcode selected (see docs/BUILD.md).')
    SOURCES.mkdir(parents=True, exist_ok=True)
    deps = {name: checkout(name) for name in LOCK}
    sdk = deps['MacKernelSDK']
    for name in ['Lilu', 'AppleALC', 'VoodooI2C', 'VoodooInput']:
        link(sdk, deps[name] / 'MacKernelSDK')
    lilu_debug = build('Lilu', deps['Lilu'], 'Debug')
    link(lilu_debug / 'Lilu.kext', deps['AppleALC'] / 'Lilu.kext')
    lilu = build('Lilu', deps['Lilu'])
    alc = build('AppleALC', deps['AppleALC'])
    for config in ['Debug', 'Release']:
        product = build('VoodooInput', deps['VoodooInput'], config)
        link(product, deps['VoodooI2C'] / 'Dependencies/VoodooInput' / config)
    i2c = build('VoodooI2C', deps['VoodooI2C'])
    hid = build('VoodooI2CHID', deps['VoodooI2C'] / 'VoodooI2C Satellites/VoodooI2CHID')
    aw = build('AW88399Driver', ROOT / 'AW88399Driver', MAC_KERNEL_SDK_DIR=sdk,
               KERNEL_EXTENSION_HEADER_SEARCH_PATHS=sdk / 'Headers',
               KERNEL_FRAMEWORK_HEADERS=sdk / 'Headers')
    dist = ROOT / 'dist'
    # Refuse to merge with an owner-imported bundle or a previous release.
    dist.mkdir()
    kexts = dist / 'Kexts'
    kexts.mkdir()
    for product, name in [(lilu, 'Lilu'), (alc, 'AppleALC'), (i2c, 'VoodooI2C'),
                          (hid, 'VoodooI2CHID'), (aw, 'AW88399Driver')]:
        target = kexts / f'{name}.kext'
        shutil.copytree(product / f'{name}.kext', target, symlinks=True)
        run('codesign', '--force', '--deep', '--sign', '-', target)
        run('codesign', '--verify', '--deep', '--strict', target)
    for name in ['README.md', 'docs', 'tools/import_firmware.py', 'examples', 'LICENSES',
                 'dependencies.lock.json', 'THIRD_PARTY.md']:
        target = dist / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if (ROOT / name).is_dir():
            shutil.copytree(ROOT / name, target)
        else:
            shutil.copyfile(ROOT / name, target)
    aw_info = plistlib.loads((kexts / 'AW88399Driver.kext/Contents/Info.plist').read_bytes())
    assert 'AW88399FirmwareData' not in aw_info['IOKitPersonalities']['AW88399Driver']
    license_sources = {name: path for name, path in deps.items()}
    i2c_source = deps['VoodooI2C']
    license_sources['VoodooGPIO'] = i2c_source / 'Dependencies/VoodooGPIO'
    license_sources['VoodooI2CHID'] = i2c_source / 'VoodooI2C Satellites/VoodooI2CHID'
    for name, path in license_sources.items():
        for license_file in path.glob('LICENSE*'):
            if license_file.is_file():
                target = dist / 'LICENSES' / name / license_file.name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(license_file, target)
    manifest = {'projectCommit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                'dependencies': LOCK, 'xcode': subprocess.check_output(['xcodebuild', '-version'], text=True),
                'sdk': subprocess.check_output(['xcrun', '--show-sdk-version'], text=True),
                'firmwareIncluded': False, 'files': {}}
    checksums = []
    for file in sorted(dist.rglob('*')):
        if file.is_file():
            digest = hashlib.sha256(file.read_bytes()).hexdigest()
            relative = file.relative_to(dist).as_posix()
            manifest['files'][relative] = digest
            checksums.append(f'{digest}  {relative}\n')
    (dist / 'build-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    checksums.append(f"{hashlib.sha256((dist / 'build-manifest.json').read_bytes()).hexdigest()}  build-manifest.json\n")
    (dist / 'SHA256SUMS').write_text(''.join(checksums))
    run('ditto', '-c', '-k', '--keepParent', dist, BUILD / 'Legion-ALC287-AW88399-macOS-0.1.1.zip')
    print('Built firmware-free release. Import the qualified ACF locally before installing AW88399Driver.')


if __name__ == '__main__':
    main()
