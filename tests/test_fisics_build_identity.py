#!/usr/bin/env python3
"""Prove actual fisiCs dependency invalidation in a marked disposable copy."""
import argparse
import json
from pathlib import Path
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    root = args.source.resolve()
    if not (root/'.l4-contained-copy').is_file() or (root/'.git').exists():
        parser.error('requires a marked disposable copy without Git metadata')
    args.evidence.mkdir(parents=True, exist_ok=True)
    target = 'build/targets/macOS-arm64/toolchains/fisics/obj/src/domain/drawing_program_color_model.c.o'
    obj = root/target
    results = []

    def build(name, settings=(), expected=0):
        with (args.evidence/(name+'.log')).open('w') as log:
            result = subprocess.run(['make', '-j4', 'BUILD_TOOLCHAIN=fisics',
                'CODEWORK_WORKSPACE_ROOT='+str(args.workspace),
                'TARGET_CONTRACT_HELPER='+str(args.workspace/'bin/desktop_release_target_contract.sh'), *settings, target],
                cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=120)
        assert result.returncode == expected, (name, result.returncode)

    def check(name, mutation=None, settings=(), changed=True):
        time.sleep(1.1)
        before = obj.stat().st_mtime_ns
        old = mutation.read_bytes() if mutation else None
        if mutation:
            mutation.write_bytes(old + (b'\n/* L4 header probe */\n' if mutation.suffix == '.h'
                                        else b'\n# L4 identity probe\n'))
        try:
            build(name, settings)
            actual = obj.stat().st_mtime_ns != before
            results.append(dict(case=name, rebuilt=actual, expected=changed))
            (args.evidence/'results.json').write_text(json.dumps(results, indent=2)+'\n')
            assert actual == changed, name
        finally:
            if mutation:
                time.sleep(1.1)
                mutation.write_bytes(old)
                build(name+'-restore')

    build('prime')
    check('unchanged', changed=False)
    header = root/'include/drawing_program/drawing_program_color_model.h'
    check('direct-header', header)
    check('build-rule', root/'make/rules-build.mk')
    check('adapter-content', root/'tools/fisics_compile.py')
    flags=(root/'build/targets/macOS-arm64/toolchains/fisics/compiler.stamp').read_text().splitlines()[1]
    check('compiler-flags', settings=('PROGRAM_CFLAGS='+flags+' -DL4_PROBE=1',))
    build('flags-restore')
    original = header.read_bytes()
    nested = header.parent/'l4_nested.h'
    assert not nested.exists()
    try:
        nested.write_text('/* temporary transitive include */\n')
        time.sleep(1.1)
        header.write_bytes(original + b'\n#include "l4_nested.h"\n')
        build('transitive-prime')
        check('transitive-header', nested)
        old_obj, old_dep = obj.read_bytes(), obj.with_suffix('.d').read_bytes()
        nested.unlink()
        build('missing-header-refusal', expected=2)
        assert obj.read_bytes() == old_obj and obj.with_suffix('.d').read_bytes() == old_dep
        nested.write_text('/* restored transitive include */\n')
        build('missing-header-retry')
    finally:
        time.sleep(1.1)
        header.write_bytes(original)
        nested.unlink(missing_ok=True)
        build('transitive-restore')
    check('final-unchanged', changed=False)
    print('passed', len(results), 'incremental cases plus missing-header refusal/retry')


if __name__ == '__main__':
    main()
