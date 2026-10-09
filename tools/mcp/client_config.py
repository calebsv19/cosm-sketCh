#!/usr/bin/env python3
"""Print configuration; never edit client settings automatically."""
import json, os
from pathlib import Path
root=Path(__file__).resolve().parents[2]
env={"SKETCH_CONTROL_SOCKET":os.environ.get("SKETCH_CONTROL_SOCKET",f"/tmp/sketch-control-{os.getuid()}/control.sock")}
if os.environ.get("SKETCH_MCP_PYTHON"): env["SKETCH_MCP_PYTHON"]=os.environ["SKETCH_MCP_PYTHON"]
print(json.dumps({"mcpServers":{"sketch":{"command":str(root/"tools/mcp/run.sh"),"env":env}}},indent=2))
