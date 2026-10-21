"""Frozen provenance must preserve the complete reviewed shared source set."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("vulkan_verify", ROOT / "tools/verify-vulkan-rollout.py")
verify = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify)

class FrozenProvenanceTest(unittest.TestCase):
    def test_valid_and_refusals(self):
        manifest = ROOT / "tools/vulkan_shared_manifest.json"
        shared = ROOT / "third_party/codework_shared"
        self.assertTrue(verify.verify_frozen_shared(shared, manifest))
        original = json.loads(manifest.read_text())
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "manifest.json"
            for case in ("missing", "changed", "traversal", "commit"):
                data = json.loads(json.dumps(original))
                key = next(iter(data["files"]))
                if case == "missing": del data["files"][key]
                elif case == "changed": data["files"][key] = "0" * 64
                elif case == "traversal": data["files"]["../outside"] = "0" * 64
                else: data["commit"] = "0" * 40
                path.write_text(json.dumps(data))
                with self.subTest(case=case), self.assertRaises(SystemExit):
                    verify.verify_frozen_shared(shared, path)

if __name__ == "__main__": unittest.main()
