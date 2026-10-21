"""Exercise ordinary goal dispatch and refusals without a native build."""
from pathlib import Path
import shutil
import os
import subprocess
import tempfile
import unittest

ENV = {k: v for k, v in os.environ.items() if k in ("PATH", "HOME", "TMPDIR", "SDKROOT", "DEVELOPER_DIR", "PYTHONDONTWRITEBYTECODE")}

ROOT = Path(__file__).resolve().parents[1]

class DispatchTest(unittest.TestCase):
    def test_dispatch_and_refusals(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp).resolve()
            repo = base / 'repo'
            repo.mkdir()
            subprocess.run(['git', 'init', '-q', str(repo)], check=True)
            for name in ('Makefile', 'make/preparation.mk', 'tools/packaging/prepare_shared_copy.py'):
                target = repo / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(ROOT / name, target)
            shared = repo / 'third_party/codework_shared'
            shared.mkdir(parents=True)
            (shared / 'input.c').write_text('source')
            output = base / 'owned build'
            args = ['make', '-C', str(repo), 'MAKE=/bin/echo',
                    'BUILD_DIR='+str(output), 'PREPARATION_ROOT='+str(output)]
            result = subprocess.run(args+['run-headless', 'release-package-self-test'], capture_output=True, text=True, env=ENV)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertIn('PREPARATION_CHILD=1', result.stdout)
            self.assertIn('run-headless release-package-self-test', result.stdout)
            self.assertEqual((output/'preparation-shared/input.c').read_text(), 'source')
            result = subprocess.run(args+['run-headless'], capture_output=True, text=True, env=ENV)
            self.assertNotEqual(result.returncode, 0)
            for goal in ('package-desktop-refresh', 'clean'):
                result = subprocess.run(args+[goal], capture_output=True, text=True, env=ENV)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Unsupported contained preparation goal', result.stderr)
            result = subprocess.run(args+['release-notarize', 'run-headless'], capture_output=True, text=True, env=ENV)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('Unsupported contained preparation goal', result.stderr)
            mismatch = base/'mismatch'
            result = subprocess.run(args+['BUILD_DIR='+str(mismatch), 'run-headless'], capture_output=True, text=True, env=ENV)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(mismatch.exists())

if __name__ == '__main__':
    unittest.main()
