"""Reject unsafe output roots before any package command executes."""
import pathlib
import shutil
import subprocess
import tempfile
import sys
sys.dont_write_bytecode = True
import unittest

SOURCE = pathlib.Path(__file__).resolve().parents[1]

class DisposableRootTests(unittest.TestCase):
    def test_invalid_roots_leave_existing_bytes_untouched(self):
        with tempfile.TemporaryDirectory(prefix="sketch-package-root-") as temporary:
            root = pathlib.Path(temporary)
            shutil.copyfile(SOURCE / "make/release-disposable.mk", root / "Makefile")
            helper = root / "tools/packaging/macos"
            helper.mkdir(parents=True)
            shutil.copyfile(SOURCE / "tools/packaging/macos/prepare_release_root.py",
                            helper / "prepare_release_root.py")
            parent = root / "build/release-authenticated"
            parent.mkdir(parents=True)
            existing = parent / "existing-job"
            existing.mkdir()
            (existing / "sentinel").write_text("keep")
            (parent / "linked-job").symlink_to(existing, target_is_directory=True)
            cases = ["", "/tmp/forbidden", "../outside", "build/release",
                     "build/release-authenticated/ab", "build/release-authenticated/../escape",
                     "build/release-authenticated/existing-job", "build/release-authenticated/linked-job"]
            for value in cases:
                with self.subTest(root=value):
                    result = subprocess.run(["make", "RELEASE_ROOT=" + value,
                        "release-artifact-disposable"], cwd=root, capture_output=True, text=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("RELEASE_ROOT", result.stdout + result.stderr)
                    self.assertEqual((existing / "sentinel").read_text(), "keep")
                    self.assertFalse((root / "dist").exists())
            shutil.rmtree(parent)
            parent.symlink_to(root, target_is_directory=True)
            result = subprocess.run(["make", "RELEASE_ROOT=build/release-authenticated/ancestor-job",
                "release-artifact-disposable"], cwd=root, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("ancestor must not be a symlink", result.stdout + result.stderr)
            self.assertFalse((root / "ancestor-job").exists())

    def test_absolute_target_and_signed_roots_are_create_only(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location(
            "sketch_release_root", SOURCE / "tools/packaging/macos/prepare_release_root.py")
        helper = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(helper)
        with tempfile.TemporaryDirectory(prefix="sketch-data-root-") as temporary:
            base = pathlib.Path(temporary).resolve()
            source = base / "source"
            source.mkdir()
            data = base / "data"
            data.mkdir()
            target = data / "drawing_program/build/release-authenticated/raor_exact/targets/rapt_exact"
            self.assertEqual(helper.prepare_root(str(target), source, data), target)
            (target / "sentinel").write_text("keep")
            with self.assertRaises(ValueError):
                helper.prepare_root(str(target), source, data)
            self.assertEqual((target / "sentinel").read_text(), "keep")
            signed = data / "drawing_program/build/release-authenticated/rapcj_exact"
            self.assertEqual(helper.prepare_root(str(signed), source, data), signed)
            for bad in (base / "outside/job", data / "drawing_program/build/release-authenticated/ab",
                        data / "drawing_program/build/release-authenticated/job/extra"):
                with self.assertRaises(ValueError):
                    helper.prepare_root(str(bad), source, data)
                self.assertFalse(bad.exists())
            link = data / "drawing_program/build/release-authenticated/link"
            link.symlink_to(signed, target_is_directory=True)
            with self.assertRaises(ValueError):
                helper.prepare_root(str(link / "targets/target"), source, data)

    def test_signed_entrypoint_passes_only_fresh_root_to_child(self):
        with tempfile.TemporaryDirectory(prefix="sketch-signed-root-") as temporary:
            root = pathlib.Path(temporary).resolve()
            helper = root / "tools/packaging/macos"
            helper.mkdir(parents=True)
            shutil.copyfile(SOURCE / "tools/packaging/macos/prepare_release_root.py",
                            helper / "prepare_release_root.py")
            text = (SOURCE / "make/release.mk").read_text()
            start = text.index("release-artifact:\n")
            end = text.index(".PHONY: release-artifact-internal", start)
            (root / "Makefile").write_text(text[start:end] +
                "\nrelease-artifact-internal:\n\t@test \"$(RELEASE_DIR)\" = \"$(DIST_DIR)\"\n"
                "\t@echo proof > \"$(RELEASE_DIR)/proof\"\n")
            path = root / "build/release-authenticated/signed-job"
            cmd = ["make", "release-artifact", "RELEASE_ROOT=" + str(path)]
            self.assertEqual(subprocess.run(cmd, cwd=root, capture_output=True).returncode, 0)
            self.assertEqual((path / "proof").read_text().strip(), "proof")
            self.assertNotEqual(subprocess.run(cmd, cwd=root, capture_output=True).returncode, 0)
            self.assertFalse((root / "dist").exists())

    def test_self_test_uses_and_removes_private_runtime_on_success_and_failure(self):
        for exit_code in (0, 7):
            with self.subTest(exit_code=exit_code), tempfile.TemporaryDirectory(prefix="sketch-runtime-") as temporary:
                root = pathlib.Path(temporary)
                (root / "build").mkdir()
                (root / "Makefile").write_text(
                    (SOURCE / "make/release-disposable.mk").read_text()
                    + "\npackage-desktop-self-test:\n\t@VULKAN_ROLLOUT_PACKAGE_DIR=\"$(VULKAN_ROLLOUT_PACKAGE_DIR)\" ./mock-package-check\n")
                launcher = root / "mock-package-check"
                launcher.write_text(
                    '#!/bin/sh\nset -eu\n'
                    'printf "%s" "$DRAWING_PROGRAM_RUNTIME_DIR" > observed-runtime\n'
                    'mkdir -p "$DRAWING_PROGRAM_RUNTIME_DIR"\n'
                    'test "$(dirname "$DRAWING_PROGRAM_RUNTIME_DIR")" = "$(dirname "$DRAWING_PROGRAM_LOG_DIR")"\n'
                    'test "$(dirname "$DRAWING_PROGRAM_RUNTIME_DIR")" = "$(dirname "$VULKAN_ROLLOUT_PACKAGE_DIR")"\n'
                    'echo seeded > "$DRAWING_PROGRAM_RUNTIME_DIR/sentinel"\n'
                    f'exit {exit_code}\n')
                launcher.chmod(0o755)
                result = subprocess.run([
                    "make", "release-package-self-test", "PACKAGE_MACOS_DIR=" + str(root)
                ], cwd=root, capture_output=True, text=True)
                self.assertEqual(result.returncode == 0, exit_code == 0)
                runtime = pathlib.Path((root / "observed-runtime").read_text())
                self.assertEqual(runtime.parent.parent, (root / "build").resolve())
                self.assertFalse(runtime.exists())

if __name__ == "__main__":
    unittest.main()
