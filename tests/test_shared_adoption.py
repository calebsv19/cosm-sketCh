#!/usr/bin/env python3
"""Exercise selected-root pin refusal without touching retained vendor files."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

SPEC=importlib.util.spec_from_file_location('adoption',Path(__file__).resolve().parents[1]/'tools/verify_shared_adoption.py')
CHECK=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(CHECK)


class Adoption(unittest.TestCase):
    def test_selected_input_refusal_and_retained_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'src').mkdir();source=root/'src/input.c';source.write_text('int value;\n')
            doc={'modules':{'core/fixture':{}},'files':{'core/fixture/src/input.c':hashlib.sha256(source.read_bytes()).hexdigest()}}
            roots={'core/fixture':root}
            self.assertEqual(CHECK.verify(doc,roots),[])
            (root/'build').mkdir();(root/'build/retained.bin').write_bytes(b'preserved')
            self.assertEqual(CHECK.verify(doc,roots),[])
            source.write_text('int changed;\n');self.assertTrue(CHECK.verify(doc,roots))
            source.unlink();self.assertTrue(CHECK.verify(doc,roots))
            source.write_text('int value;\n');(root/'src/new.c').write_text('int added;\n')
            self.assertTrue(CHECK.verify(doc,roots));(root/'src/new.c').unlink()
            self.assertEqual(CHECK.verify(doc,roots),[])
            self.assertTrue(CHECK.verify(doc,{}))
            self.assertEqual((root/'build/retained.bin').read_bytes(),b'preserved')


if __name__=='__main__':unittest.main()
