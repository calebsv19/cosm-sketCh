#!/usr/bin/env python3
import argparse
import hashlib
import os
import re
import struct
import subprocess
from pathlib import Path

EXPECTED_SHARED_COMMIT = "ddc0c2b1420d95132ef089e68e2ce7728fbc53a4"


def output(command: list[str], cwd: Path) -> str:
    return subprocess.run(command, cwd=cwd, check=True, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout.strip()


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_shared(adopted: Path, canonical: Path) -> str:
    commit = output(["git", "rev-parse", "HEAD"], canonical)
    ancestor = subprocess.run(
        ["git", "merge-base", "--is-ancestor", EXPECTED_SHARED_COMMIT, commit],
        cwd=canonical).returncode
    if ancestor:
        raise SystemExit(
            f"canonical shared history does not contain {EXPECTED_SHARED_COMMIT}: {commit}")
    tracked = output(["git", "ls-tree", "-r", "--name-only",
                      EXPECTED_SHARED_COMMIT, "--", "vk_runtime", "vk_renderer"],
                     canonical).splitlines()
    manifest = hashlib.sha256()
    mismatches = []
    for relative in tracked:
        target = adopted / relative
        source_bytes = subprocess.run(
            ["git", "show", f"{EXPECTED_SHARED_COMMIT}:{relative}"],
            cwd=canonical, check=True, stdout=subprocess.PIPE).stdout
        source_digest = hashlib.sha256(source_bytes).hexdigest()
        if not target.is_file() or digest(target) != source_digest:
            mismatches.append(relative)
            continue
        manifest.update(relative.encode())
        manifest.update(b"\0")
        manifest.update(source_digest.encode())
        manifest.update(b"\n")
    if mismatches:
        raise SystemExit("adopted Vulkan source mismatch: " + ", ".join(mismatches[:12]))
    return manifest.hexdigest()


def verify_adoption(repo: Path) -> None:
    loop = (repo / "src/app/drawing_program_app_visual_runtime_loop.c").read_text()
    backend = (repo / "src/runtime/render/drawing_program_render_backend_lifecycle.c").read_text()
    parser = (repo / "src/runtime/render/drawing_program_render_backend.c").read_text()
    if "drawing_program_render_backend_create(window, backend_kind)" not in loop:
        raise SystemExit("visual runtime is not attached to the managed backend")
    if "drawing_program_render_backend_present(renderer)" not in loop:
        raise SystemExit("visual runtime bypasses managed presentation")
    if "*out_kind = DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT" not in parser:
        raise SystemExit("Vulkan is not the default presentation backend")
    offenders = []
    for path in (repo / "src").rglob("*.c"):
        if path.name == "drawing_program_render_backend_lifecycle.c":
            continue
        text = path.read_text()
        if "SDL_RenderPresent(" in text or "SDL_CreateRenderer(" in text:
            offenders.append(str(path.relative_to(repo)))
    if offenders:
        raise SystemExit("direct presentation bypass remains: " + ", ".join(offenders))
    required = ["vk_renderer_init", "vk_renderer_recreate_swapchain",
                "vk_renderer_request_capture", "vk_runtime_get_capability_report",
                "SDL_Vulkan_GetDrawableSize", "VK_FILTER_NEAREST",
                "device->instance != device->runtime.instance"]
    missing = [token for token in required if token not in backend]
    if missing:
        raise SystemExit("backend proof surface incomplete: " + ", ".join(missing))


def inspect_bmp(path: Path) -> tuple[int, int, int]:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise SystemExit(f"invalid capture: {path}")
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bpp = struct.unpack_from("<H", data, 28)[0]
    stride = ((width * bpp + 31) // 32) * 4
    colors = set()
    for row in range(0, abs(height), max(1, abs(height) // 64)):
        for column in range(0, width, max(1, width // 64)):
            start = offset + row * stride + column * (bpp // 8)
            colors.add(data[start:start + (bpp // 8)])
    if width <= 0 or height == 0 or bpp not in (24, 32) or len(colors) < 5:
        raise SystemExit(f"capture lacks Drawing Program frame evidence: {path}")
    return width, abs(height), len(colors)


def validation_env(shader_root: Path, moltenvk: Path | None = None,
                   icd_manifest: Path | None = None) -> dict[str, str]:
    env = os.environ.copy()
    candidates = [Path("/opt/homebrew/opt/vulkan-validationlayers"),
                  Path("/usr/local/opt/vulkan-validationlayers")]
    prefix = next((candidate for candidate in candidates
                   if (candidate / "lib/libVkLayer_khronos_validation.dylib").is_file()), None)
    if prefix is None:
        raise SystemExit("Khronos validation layer is unavailable")
    env["NSUnbufferedIO"] = "YES"
    library_paths = [str(prefix / "lib")]
    if moltenvk is not None:
        moltenvk = moltenvk.resolve()
        if not moltenvk.is_file() or icd_manifest is None:
            raise SystemExit(f"bundled MoltenVK is unavailable: {moltenvk}")
        icd_manifest.parent.mkdir(parents=True, exist_ok=True)
        icd_manifest.write_text(
            '{\n  "file_format_version": "1.0.0",\n  "ICD": {\n'
            f'    "library_path": "{moltenvk}",\n'
            '    "api_version": "1.4.0",\n'
            '    "is_portability_driver": true\n  }\n}\n')
        env["VK_ICD_FILENAMES"] = str(icd_manifest.resolve())
        env["VK_DRIVER_FILES"] = str(icd_manifest.resolve())
        library_paths.append(str(moltenvk.parent))
    env["DYLD_LIBRARY_PATH"] = ":".join(library_paths)
    env["VK_LAYER_PATH"] = str(prefix / "share/vulkan/explicit_layer.d")
    env["DRAWING_PROGRAM_REQUIRE_VULKAN"] = "1"
    env["DRAWING_PROGRAM_REQUIRE_VK_VALIDATION"] = "1"
    env["DRAWING_PROGRAM_VULKAN_SHADER_ROOT"] = str(shader_root.resolve())
    return env


def run_app(app: Path, shader_root: Path, initial: Path, resized: Path,
            log: Path, minimum_scale: float, moltenvk: Path | None) -> None:
    initial.unlink(missing_ok=True)
    resized.unlink(missing_ok=True)
    initial.parent.mkdir(parents=True, exist_ok=True)
    log.parent.mkdir(parents=True, exist_ok=True)
    env = validation_env(shader_root, moltenvk, log.parent / "MoltenVK_icd.json")
    env["DRAWING_PROGRAM_VULKAN_ROLLOUT_INITIAL_CAPTURE"] = str(initial.resolve())
    env["DRAWING_PROGRAM_VULKAN_ROLLOUT_RESIZED_CAPTURE"] = str(resized.resolve())
    env["DRAWING_PROGRAM_VULKAN_ROLLOUT_MIN_SCALE"] = str(minimum_scale)
    result = subprocess.run([str(app.resolve()), "--vulkan-rollout-self-test"],
                            env=env, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text(result.stdout or "")
    print(result.stdout or "", end="")
    if result.returncode:
        raise SystemExit(f"Drawing Program Vulkan self-test exited {result.returncode}")
    if "[vk_runtime validation]" in (result.stdout or ""):
        raise SystemExit("validation diagnostics were emitted")
    stages = re.findall(
        r"DRAWING_PROGRAM_VULKAN_RUNTIME schema=1 stage=(startup|resized|restart) "
        r"status=pass runtime=0\.6\.0 .* validation_requested=1 validation_enabled=1 "
        r"warnings=0 errors=0 handles=shared",
        result.stdout or "")
    if stages != ["startup", "resized", "restart"]:
        raise SystemExit(f"incomplete runtime receipts: {stages}")
    if "filter=nearest runtime=shared resize=recreated capture=native restart=pass" not in (result.stdout or ""):
        raise SystemExit("missing rollout completion receipt")
    initial_info = inspect_bmp(initial)
    resized_info = inspect_bmp(resized)
    if initial_info[:2] == resized_info[:2]:
        raise SystemExit("capture dimensions did not change after resize")
    print(f"Drawing Program Vulkan captures: initial={initial_info[0]}x{initial_info[1]} "
          f"resized={resized_info[0]}x{resized_info[1]} colors>=5")


def run_actual_application(app: Path, shader_root: Path, capture: Path,
                           log: Path, moltenvk: Path | None) -> None:
    capture.unlink(missing_ok=True)
    capture.parent.mkdir(parents=True, exist_ok=True)
    runtime = capture.parent / "runtime"
    runtime.mkdir(parents=True, exist_ok=True)
    log.parent.mkdir(parents=True, exist_ok=True)
    env = validation_env(shader_root, moltenvk,
                         log.parent / "MoltenVK_application_icd.json")
    env["DRAWING_PROGRAM_RUNTIME_DIR"] = str(runtime.resolve())
    result = subprocess.run(
        [str(app.resolve()), "--render-backend", "vulkan-kit",
         "--visual-artifact", str(capture.resolve()), "--no-persist"],
        env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text(result.stdout or "")
    print(result.stdout or "", end="")
    if result.returncode:
        raise SystemExit(f"Drawing Program application proof exited {result.returncode}")
    if "[vk_runtime validation]" in (result.stdout or ""):
        raise SystemExit("application proof emitted validation diagnostics")
    required = [
        "DRAWING_PROGRAM_RENDERER_BACKEND schema=1 backend=vulkan-kit status=ready",
        "DRAWING_PROGRAM_VULKAN_RUNTIME schema=1 stage=application-startup status=pass",
        "DRAWING_PROGRAM_VULKAN_RUNTIME schema=1 stage=application-frame status=pass",
        "DRAWING_PROGRAM_RENDERER_SHUTDOWN schema=1 backend=vulkan-kit frames=1 status=pass",
    ]
    missing = [receipt for receipt in required if receipt not in (result.stdout or "")]
    if missing:
        raise SystemExit("application proof missing receipts: " + ", ".join(missing))
    info = inspect_bmp(capture)
    print(f"Drawing Program application Vulkan capture: {info[0]}x{info[1]} colors={info[2]}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--shared-root", type=Path, required=True)
    parser.add_argument("--canonical-shared", type=Path, default=Path("../shared"))
    parser.add_argument("--app", type=Path)
    parser.add_argument("--shader-root", type=Path)
    parser.add_argument("--initial-capture", type=Path)
    parser.add_argument("--resized-capture", type=Path)
    parser.add_argument("--log", type=Path)
    parser.add_argument("--minimum-scale", type=float, default=1.0)
    parser.add_argument("--actual-app-capture", type=Path)
    parser.add_argument("--actual-app-log", type=Path)
    parser.add_argument("--moltenvk", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    adopted = args.shared_root.resolve()
    canonical = (repo / args.canonical_shared).resolve()
    manifest = verify_shared(adopted, canonical)
    verify_adoption(repo)
    runtime = (adopted / "vk_runtime/VERSION").read_text().strip()
    renderer = (adopted / "vk_renderer/VERSION").read_text().strip()
    if (runtime, renderer) != ("0.6.0", "1.3.2"):
        raise SystemExit(f"unexpected Vulkan versions: runtime={runtime} renderer={renderer}")
    if args.app:
        if not args.shader_root or not args.initial_capture or not args.resized_capture or not args.log:
            raise SystemExit("app execution requires shader root, captures, and log")
        run_app(args.app, args.shader_root, args.initial_capture,
                args.resized_capture, args.log, args.minimum_scale, args.moltenvk)
        if args.actual_app_capture:
            if not args.actual_app_log:
                raise SystemExit("actual app capture requires an actual app log")
            run_actual_application(args.app, args.shader_root,
                                   args.actual_app_capture, args.actual_app_log,
                                   args.moltenvk)
    print(f"Drawing Program Vulkan rollout contract: canonical_commit={EXPECTED_SHARED_COMMIT} "
          f"manifest_sha256={manifest} vk_runtime={runtime} vk_renderer={renderer}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
