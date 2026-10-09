#!/bin/sh
set -eu
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
python_bin="${SKETCH_MCP_PYTHON:-$repo_root/build/mcp-venv/bin/python}"
if [ ! -x "$python_bin" ]; then
    echo 'Missing MCP SDK environment; follow tools/mcp/README.md or set SKETCH_MCP_PYTHON.' >&2
    exit 1
fi
exec "$python_bin" "$repo_root/tools/mcp/server.py"
