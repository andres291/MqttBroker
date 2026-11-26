"""Launch IDF's unmodified ldgen.py with arguments loaded from a local file."""
import json
import runpy
import sys
from pathlib import Path

arguments = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
sys.argv = arguments[1:]
sys.path.insert(0, str(Path(sys.argv[0]).parent))
runpy.run_path(sys.argv[0], run_name="__main__")
