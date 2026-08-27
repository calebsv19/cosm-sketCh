# Drawing Program Main Edit Worktree

The persistent Main Edit lane is the default single-writer integration lane for
ongoing `drawing_program` / `sketCh` functional development.

## Lane Identity

- canonical branch: `main`
- Main Edit branch: `codex/drawing-program-main-edit`
- worktree convention: `<workspace>/_worktrees/drawing_program_main_edit`
- development app: `sketCh Main Edit.app`
- bundle ID: `com.cosm.sketch.main-edit`
- runtime/log namespace: `DrawingProgram-Main-Edit`
- identity schema: `codework_local_development_build_identity_v1`

The program `VERSION` is inherited from canonical and is not changed merely to
start or checkpoint development work.

## Start Gate

Before editing, read back both lanes:

```sh
git -C <repo> status --short
git -C <repo> worktree list --porcelain
git -C <repo> rev-list --left-right --count main...codex/drawing-program-main-edit
git -C <workspace>/_worktrees/drawing_program_main_edit status --short
```

Stop on unexpected ownership, staged/untracked paths, canonical product drift,
or a second writer. Preserve all unrelated and ignored assets, generated
artifacts, Dungeon fixtures, and specialist worktrees.

## Checkpoint Gate

Use the narrowest affected suite first, then the broad source/package ladder:

```sh
make -C <workspace>/_worktrees/drawing_program_main_edit
make -C <workspace>/_worktrees/drawing_program_main_edit main-edit-package-contract-checks
make -C <workspace>/_worktrees/drawing_program_main_edit test
make -C <workspace>/_worktrees/drawing_program_main_edit run-headless
make -C <workspace>/_worktrees/drawing_program_main_edit visual-artifact
make -C <workspace>/_worktrees/drawing_program_main_edit package-desktop-main-edit-self-test
git -C <workspace>/_worktrees/drawing_program_main_edit diff --check
```

The Main Edit package is generated under the target-specific development root.
It embeds exact source and binary identity and fails closed if source changes
during packaging. The Vulkan provenance verifier resolves canonical shared
history through the repository common directory so the same check works from
both canonical and linked-worktree paths.

## Integration Gate

Before canonical adoption, freshly classify canonical-only commits and rerun
affected gates after any reconciliation. Use a fast-forward when canonical is
an ancestor of the verified Main Edit tip. Use an explicit reviewed merge only
when legitimate canonical drift requires it. Independently read back the final
canonical commit and cleanliness.

Source adoption does not authorize a version change, release artifact,
Registry mutation, publication, deployment, push, or Desktop-app replacement.

## Retention Gate

Retain the named lane after adoption by default. Never reset, clean,
force-remove, or repurpose it to recover its name. Recycling requires a clean
lane, no untracked owner data, retained commit reachability, explicit ignored
artifact handling, and no process owner. The guarded refresh target refuses to
replace a running Main Edit app and cannot target the canonical Desktop app.
