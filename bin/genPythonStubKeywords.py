#!/usr/bin/env python3
"""
Writes the keyword properties of Builtin in python/opm_embedded/__init__.pyi.

The Python module has one Builtin property per keyword in
opm/input/eclipse/share/keywords/keyword_list.cmake, so this part of the stub
follows from the keyword list without building the module.  stubgen writes
the rest of the stub, see python/README.md.

Usage: ./bin/genPythonStubKeywords.py
"""

import pathlib
import re

root = pathlib.Path(__file__).resolve().parent.parent
keyword_dir = root / "opm/input/eclipse/share/keywords"
stub_file = root / "python/opm_embedded/__init__.pyi"

keyword_list = (keyword_dir / "keyword_list.cmake").read_text(encoding="utf-8")
keyword_files = re.search(r"set\(\s*keywords(.*?)\)", keyword_list, re.S).group(1).split()

# Not every keyword file is strict JSON, but the first "name" in each is the keyword's.
name_regex = re.compile(r'"name"\s*:\s*"([^"]+)"')
keywords = sorted({name_regex.search((keyword_dir / f).read_text(encoding="utf-8")).group(1)
                   for f in keyword_files})

stub = stub_file.read_text(encoding="utf-8")
lines = stub.split("\n")
begin = lines.index("class Builtin:") + 1
end = begin
while end < len(lines) and lines[end].startswith("    "):
    end += 1

# stubgen writes the properties after the methods.
first_property = lines.index("    @property", begin, end)
lines[first_property:end] = [line for keyword in keywords
                             for line in ("    @property",
                                          f"    def {keyword}(self) -> Any: ...")]

new_stub = "\n".join(lines)
if new_stub != stub:
    stub_file.write_text(new_stub, encoding="utf-8")
