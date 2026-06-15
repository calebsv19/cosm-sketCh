# sketCh Memory-Check Audit

`make -C drawing_program memory-check-audit` is a default-off fisiCs
memory-check audit for the headless smoke path. It does not change normal
Clang builds, packaging, release behavior, or app launch behavior.

## Command

```sh
make -C drawing_program memory-check-audit
```

The target rebuilds the app with:

```text
--overlay=physics-units,memory-check
```

Then it runs the fisiCs-built headless binary with:

```text
--headless --smoke-frames 2 --print-lifecycle --no-persist
```

## Reports

- stdout: `drawing_program/build/memory_check/drawing_program.stdout`
- stderr/report: `drawing_program/build/memory_check/drawing_program.stderr`

`MEMORY_CHECK_REPORT_POLICY` defaults to `always` so clean runs leave a report.

## Current Baseline

Last audited: 2026-06-07.

```text
[fisics:memory-check] summary: active=0 leaked_bytes=0 allocs=20 frees=20 double_free=0 unknown_free=0 tracker_failures=0
```

Interpretation:

- no tracked allocations remained live at process exit
- all tracked allocation/free pairs balanced on the headless smoke path
- no double-free or unknown-pointer free events were reported
- the memory-check runtime tracker did not report internal failures
