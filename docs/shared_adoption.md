# Shared adoption and local consumer verification

The L4 review binds the retained sketCh shared inputs at Main Edit `0ca7eed`
plus the local integration delta. The supported local routes below use explicit
module roots and exact file hashes in `config/shared_adoption.json`. A module
version is descriptive; the file hashes and module-specific source commit identify
the reviewed content. Existing core/kit implementations are reused unchanged.

## Selected modules

| Module | Version | Shared source commit |
| --- | --- | --- |
| core/core_authored_texture | 0.4.0 | `1941bcd41775cb718562dcfaafa5d1193c7dfc8e` |
| core/core_base | 1.0.1 | `18e47fe8da6ec9c4e5a53a51ddfb52aaa8b8738a` |
| core/core_font | 1.0.2 | `894c19cb9ad97323d1edb60aa9da49d1f80c58ce` |
| core/core_io | 1.1.1 | `abf90d219b4af7baf8d842924717b18934e97965` |
| core/core_layout | 0.2.1 | `153b6ba912b64c41fd78a7246cf8b5727d4de81e` |
| core/core_object | 0.1.1 | `894c19cb9ad97323d1edb60aa9da49d1f80c58ce` |
| core/core_pack | 1.1.1 | `5e51646970f73a795d97e22d0fa9b75aa056aab1` |
| core/core_pane | 0.3.1 | `6b10b20e9b39ce72528bfe24ecf6983ae4ff9595` |
| core/core_pane_module | 0.2.0 | `cc340d78a3cea80b1086fc5e434ccbaf1118c34c` |
| core/core_scene | 1.2.0 | `4b83b523755a364a8144623b9ff5435de86e92d7` |
| core/core_theme | 2.0.1 | `894c19cb9ad97323d1edb60aa9da49d1f80c58ce` |
| core/core_units | 0.2.0 | `abf90d219b4af7baf8d842924717b18934e97965` |
| core/core_viewport2d | 0.2.2 | `96819d4a70cc04ba5b40d5b282cf54e972997a66` |
| kit/kit_pane | 0.5.0 | `ddc9fee6e17482dcd64cf777d7a105b7ed9b157d` |
| kit/kit_render | 0.14.6 | `f80f91d6f1e1ec731434934d83fac85b338172a3` |
| kit/kit_ui | 0.18.0 | `854b51ff57c756b4565021fe759c27a459efbfd3` |
| kit/kit_workspace_authoring | 0.6.1 | `ad3b83b64df01770308ef52b4443252d4717266e` |
| vk_renderer | 1.6.0 | `854b51ff57c756b4565021fe759c27a459efbfd3` |
| vk_runtime | 0.6.0 | `0cdf0aca285dc85f32de54a5a57c46f71388b1c1` |

Nineteen modules / 306 files are declared. Eighteen static libraries enter the
Clang application, headless and host-test link commands; their selected members
are observed in linker maps. `core_io` is retained in the declared input set but
has no compiled or linked consumer in these routes. Five shared SDL source files
are compiled directly from the selected kit roots as application objects.

## Build behavior

Default `SHARED_MODE=vendored` selects `third_party/codework_shared`.
`SHARED_MODE=workspace-linked SHARED_WORKSPACE_DIR=<root>` selects another complete,
explicit module tree, which must satisfy the same exact manifest. Missing vendored
authored-texture inputs no longer fall back silently to a live workspace copy.
Unknown modes, missing inputs, changed pins and unrecorded source fail before
compilation. An intentional adoption requires review of a new manifest and its
consumer proof; this is not an automatic upgrade to current shared HEAD.

Consumer, host-test, shared-archive and link identities include their relevant
commands/flags and build rules. Rule checksums expand after all makefiles load.
Shared archives rebuild conservatively when selected source/header/rule inputs
change; actual host compiler dependency graphs also cover outside-module headers.
Dependency targets retain the exact Make target spelling. Unchanged builds reuse
outputs. Recursive builds no longer invoke module `clean`; only the selected
regenerable archive is replaced to avoid retaining obsolete archive members.
Unrelated module output is preserved. Build in an isolated copy when selected roots
have another active writer: module recipes still write their own build directories.

The normal fisiCs route uses `tools/fisics_compile.py` with the compiler's native
`--emit-deps-json` graph. It validates dependencies and stages publication so a
compiler or metadata failure preserves the previous object and dependency pair.
The adapter's contents and selected compiler enter build identity. Use
`make BUILD_TOOLCHAIN=fisics build` to consume an existing compiler; the separate
`fisics-build` alias also rebuilds the compiler and is outside this verification.
Shared archives and regression objects remain host-compiled on that route.

## Verification and scope

From a fresh disposable source copy with the workspace target helper and root
provided, run `make build test run-headless visual-artifact`. Run the adoption and
dependency adapter unit tests under `tests/test_shared_adoption.py`,
`tests/test_shared_compile.py`, and `tests/test_fisics_dependencies.py`.
The real incremental tests `test_shared_build_identity.py` and
`test_fisics_build_identity.py` require `--source`, `--workspace`, and `--evidence`
and refuse a source directory with Git metadata or without `.l4-contained-copy`.
They mutate only that marked disposable copy and restore their probe inputs.

The November 9 L4 evidence covers local macOS arm64 Clang/Vulkan and normal
fisiCs/SDL builds; Clang regression and both routes' headless/visual and live SDK
workflows; native Vulkan resize/restart/capture and native SDL capture; selected
shared contracts; no-op, input/rule/flag invalidation and refusal/retry; and the
managed importer preservation/collision fixtures. The one application repair uses
the raster-sample read API for a raster-sample pointer in history validation.

The workspace-linked proof uses a separate tree of these exact reviewed bytes,
not the current live shared checkout. Arbitrary per-module Make overrides, Linux,
Intel, the instrumented memory-check overlay, full SDL-specific regression parity,
and physical packaging/install/release are not qualified by this review. Preparation
routing/frozen-provenance/package contract tests are source-level checks only.
No module version changed. Main Edit qualification does not imply canonical
adoption or installed behavior.
