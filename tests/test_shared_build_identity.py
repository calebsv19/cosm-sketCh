#!/usr/bin/env python3
"""Challenge real sketCh build inputs in a marked, disposable source copy."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--workspace',type=Path,required=True)
    p.add_argument('--evidence',type=Path,required=True)
    a=p.parse_args();root=a.source.resolve();a.evidence.mkdir(parents=True,exist_ok=True)
    if not (root/'.l4-contained-copy').is_file() or (root/'.git').exists():
        p.error('requires marked disposable copy without Git metadata')
    targets=list((root/'build/targets').iterdir());assert len(targets)==1
    target=targets[0];lib=target/'shared/libcore_base.a'
    obj=target/'toolchains/clang/obj/src/domain/drawing_program_color_model.c.o'
    host=target/'host/tests/obj/src/domain/drawing_program_color_model.c.o'
    binary=target/'toolchains/clang/bin/drawing-program-bin'
    base=['make','-j4','TARGET_CONTRACT_HELPER='+str(a.workspace/'bin/desktop_release_target_contract.sh'),
          'CODEWORK_WORKSPACE_ROOT='+str(a.workspace)]
    results=[];manifest=root/'config/shared_adoption.json'

    def run(name,goals,settings=(),expected=0):
        with (a.evidence/(name+'.log')).open('w') as log:
            result=subprocess.run(base+list(settings)+[str(g.relative_to(root)) if isinstance(g,Path) else g for g in goals],
                cwd=root,stdout=log,stderr=subprocess.STDOUT,timeout=600)
        assert result.returncode==expected,(name,result.returncode)

    def check(name,watched,goals=None,settings=(),mutation=None,changed=True):
        time.sleep(1.1);before=[f.stat().st_mtime_ns for f in watched]
        old=mutation.read_bytes() if mutation else None;pin=manifest.read_bytes()
        if mutation:
            mutation.write_bytes(old+(b'\n/* L4 include probe */\n' if mutation.suffix=='.h' else b'\n# L4 rule probe\n'))
            vendor=root/'third_party/codework_shared'
            if mutation.is_relative_to(vendor):
                doc=json.loads(pin);doc['files'][str(mutation.relative_to(vendor))]=hashlib.sha256(mutation.read_bytes()).hexdigest()
                manifest.write_text(json.dumps(doc))
        try:
            run(name,goals or watched,settings)
            actual=[f.stat().st_mtime_ns!=t for f,t in zip(watched,before)]
            results.append({'case':name,'rebuilt':actual,'expected':changed})
            (a.evidence/'results.json').write_text(json.dumps(results,indent=2)+'\n')
            assert all(v==changed for v in actual),name
        finally:
            if mutation:
                time.sleep(1.1);mutation.write_bytes(old);manifest.write_bytes(pin);run(name+'-restore',goals or watched)

    run('identity-prime',[lib,obj])
    check('unchanged',[lib,obj],changed=False)
    check('compiler-command',[lib,obj],settings=('HOST_CC=cc -DL4_COMPILER_ID=1',))
    run('compiler-restore',[lib,obj])
    check('consumer-rule',[lib,obj],mutation=root/'make/rules-build.mk')
    check('shared-header',[lib],mutation=root/'third_party/codework_shared/core/core_base/include/core_base.h')
    check('module-rule',[lib],mutation=root/'third_party/codework_shared/core/core_base/Makefile')
    run('host-prime',[host]);check('host-compiler',[host],settings=('HOST_CC=cc -DL4_HOST_ID=1',));run('host-restore',[host])
    # Include a temporary outside-module header to test captured transitive inputs.
    source=root/'third_party/codework_shared/core/core_base/src/core_base.c';old=source.read_bytes();pin=manifest.read_bytes()
    external=root/'l4_external.h';assert not external.exists();external.write_text('/* outside module */\n')
    try:
        time.sleep(1.1);source.write_bytes(old+('\n#include "'+str(external)+'"\n').encode())
        doc=json.loads(pin);doc['files']['core/core_base/src/core_base.c']=hashlib.sha256(source.read_bytes()).hexdigest();manifest.write_text(json.dumps(doc))
        run('external-prime',[lib]);check('external-header',[lib],mutation=external)
    finally:
        time.sleep(1.1);source.write_bytes(old);manifest.write_bytes(pin);run('external-restore',[lib]);external.unlink()
    run('binary-prime',['build'])
    overlay=a.evidence.resolve()/'link.mk';linkmap=a.evidence.resolve()/'application.map'
    overlay.write_text('APP_LDLIBS += -Wl,-map,'+str(linkmap)+'\n')
    check('link-options',[binary],['build'],('-f','Makefile','-f',str(overlay)))
    assert linkmap.is_file();run('link-restore',['build']);check('binary-noop',[binary],['build'],changed=False)
    header=root/'third_party/codework_shared/core/core_authored_texture/include/core_authored_texture.h';saved=header.read_bytes()
    try:
        header.unlink();run('missing-vendor-refusal',['build'],('SHARED_WORKSPACE_DIR='+str(a.workspace/'shared'),),expected=2)
    finally:header.write_bytes(saved)
    run('missing-vendor-retry',['build'])
    for module in json.loads(manifest.read_text())['modules']:
        marker=root/'third_party/codework_shared'/module/'build/l4-retained-marker'
        assert marker.read_text()=='preserved unrelated output',module
    print('passed',len(results),'incremental cases, missing-input refusal/retry and retained-output preservation')


if __name__=='__main__':main()
