#!/usr/bin/env python3
"""Verify dependency capture under a module's existing strict -MMD flags."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile
import unittest


class CompileCapture(unittest.TestCase):
    def test_actual_header_graph_and_compile_refusal(self):
        tool=Path(__file__).resolve().parents[1]/'tools/shared_compile.py'
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);header=root/'nested header.h';header.write_text('#define VALUE 1\n')
            source=root/'input.c';source.write_text('#include "nested header.h"\nint value=VALUE;\n')
            obj=root/'input.o';archive=root/'consumer.a'
            command=[sys.executable,str(tool),'--target',str(archive),'--','cc','-Werror','-MMD','-MP','-c',str(source),'-o',str(obj)]
            result=subprocess.run(command,capture_output=True,text=True);self.assertEqual(result.returncode,0,result.stderr)
            record=json.loads(Path(str(obj)+'.l4.json').read_text());self.assertIn(str(header),record['dependencies'])
            dep=Path(str(obj)+'.l4.d');original=dep.read_bytes();self.assertIn('nested\\ header.h',dep.read_text())
            source.write_text('invalid C syntax\n');result=subprocess.run(command,capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0);self.assertEqual(dep.read_bytes(),original)
            self.assertFalse(Path(str(dep)+'.raw').exists())


if __name__=='__main__':unittest.main()
