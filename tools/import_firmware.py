#!/usr/bin/env python3
"""Import an owner-supplied ACF into a release bundle; no disk installation."""
import argparse
import hashlib
from pathlib import Path
import plistlib
import subprocess
import sys

EXPECTED_SHA256 = '1e927c9bca76d868181c0f81df2bccef3cf19c7d0910219f229360c87babd42c'
EXPECTED_LENGTH = 152416


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('acf', type=Path)
    parser.add_argument('kext', type=Path)
    args = parser.parse_args()
    data = args.acf.read_bytes()
    if len(data) != EXPECTED_LENGTH or hashlib.sha256(data).hexdigest() != EXPECTED_SHA256:
        parser.error('ACF does not match the qualified 83F5 firmware; bundle was not changed')
    info_path = args.kext / 'Contents/Info.plist'
    info = plistlib.loads(info_path.read_bytes())
    if info.get('CFBundleIdentifier') != 'org.codex.AW88399Driver':
        parser.error('target is not AW88399Driver.kext')
    personality = info['IOKitPersonalities']['AW88399Driver']
    if personality.get('IOClass') != 'AW88399Driver':
        parser.error('unexpected driver personality')
    personality['AW88399FirmwareData'] = data
    info_path.write_bytes(plistlib.dumps(info, sort_keys=False))
    if sys.platform == 'darwin':
        subprocess.run(['codesign', '--force', '--sign', '-', str(args.kext)], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(args.kext)], check=True)
    print(f'Imported {len(data)} bytes into {args.kext}; SHA256={EXPECTED_SHA256}')
    print('This bundle now contains owner-supplied firmware. Do not upload or redistribute it.')


if __name__ == '__main__':
    main()
