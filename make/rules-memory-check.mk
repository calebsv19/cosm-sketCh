# =========================
#  fisiCs memory-check audit
# =========================

MEMORY_CHECK_FISICS_OVERLAY := physics-units,memory-check
MEMORY_CHECK_REPORT_DIR := build/memory_check
MEMORY_CHECK_STDOUT := $(MEMORY_CHECK_REPORT_DIR)/drawing_program.stdout
MEMORY_CHECK_STDERR := $(MEMORY_CHECK_REPORT_DIR)/drawing_program.stderr
MEMORY_CHECK_HEADLESS_TARGET := $(TARGET_BUILD_DIR)/toolchains/fisics/bin/drawing-program-headless
MEMORY_CHECK_REPORT_POLICY ?= always
FISICS_MEMCHECK_RUNTIME ?= /Users/calebsv/Desktop/CodeWork/fisiCs/build/unsanitized/libfisics_memcheck_runtime.a
FISICS_MEMCHECK_LINK_LIBS ?=

memory-check-build:
	@$(MAKE) BUILD_TOOLCHAIN=fisics PROGRAM_CC="$(FISICS_CC) --overlay=$(MEMORY_CHECK_FISICS_OVERLAY)" PROGRAM_CC_DEP="$(FISICS_CC)" FISICS_MEMCHECK_LINK_LIBS="$(FISICS_MEMCHECK_RUNTIME)" -B build

memory-check-run: memory-check-build
	@mkdir -p "$(MEMORY_CHECK_REPORT_DIR)"
	FISICS_MEMCHECK_REPORT="$(MEMORY_CHECK_REPORT_POLICY)" "$(MEMORY_CHECK_HEADLESS_TARGET)" --headless --smoke-frames 2 --print-lifecycle --no-persist > "$(MEMORY_CHECK_STDOUT)" 2> "$(MEMORY_CHECK_STDERR)"
	@echo "memory-check stdout: $(MEMORY_CHECK_STDOUT)"
	@echo "memory-check stderr: $(MEMORY_CHECK_STDERR)"

memory-check-audit: memory-check-run
	@echo "memory-check summary:"
	@grep -E "\\[fisics:memory-check\\] (summary|leak|double free|unknown pointer free)" "$(MEMORY_CHECK_STDERR)" || true
