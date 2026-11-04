"""Tests for scripts/sync-configs.py, which generates ilvo/data.

    python3 -m unittest discover -s tests/docker
"""
import importlib.util
import json
import shutil
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("sync_configs", ROOT / "scripts/sync-configs.py")
sync = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sync)


class MergeTest(unittest.TestCase):
    def test_objects_merge_per_key(self):
        self.assertEqual(sync.merge({"a": 1, "b": {"c": 2, "d": 3}}, {"b": {"c": 4}}),
                         {"a": 1, "b": {"c": 4, "d": 3}})

    def test_null_removes_a_key(self):
        self.assertEqual(sync.merge({"a": 1, "b": 2}, {"b": None}), {"a": 1})

    def test_lists_are_replaced(self):
        self.assertEqual(sync.merge({"a": [1, 2]}, {"a": [3]}), {"a": [3]})

    def test_named_list_is_patched_per_name(self):
        base = [{"Name": "x", "v": 1}, {"Name": "y", "v": 1}]
        self.assertEqual(sync.merge(base, {"y": {"v": 2}}),
                         [{"Name": "x", "v": 1}, {"Name": "y", "v": 2}])

    def test_unknown_name_fails(self):
        with self.assertRaises(SystemExit):
            sync.merge([{"Name": "x"}], {"typo": {"v": 2}})

    def test_merge_does_not_change_the_input(self):
        base = {"a": {"b": 1}}
        sync.merge(base, {"a": {"b": 2}})
        self.assertEqual(base, {"a": {"b": 1}})


class GenerateTest(unittest.TestCase):
    """Runs the script on a copy of the repository files."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp)
        self.defaults = self.tmp / "defaults"
        self.overrides = self.tmp / "docker-overrides.json"
        self.target = self.tmp / "data"
        shutil.copytree(sync.DEFAULTS, self.defaults)
        shutil.copy(sync.OVERRIDES, self.overrides)
        shutil.copytree(sync.TARGET, self.target, ignore=shutil.ignore_patterns("logs", "node-red"))
        for name, value in [("ROOT", self.tmp), ("DEFAULTS", self.defaults),
                            ("OVERRIDES", self.overrides), ("TARGET", self.target)]:
            original = getattr(sync, name)
            setattr(sync, name, value)
            self.addCleanup(setattr, sync, name, original)

    def run_script(self, *args):
        argv = sync.sys.argv
        sync.sys.argv = ["sync-configs.py", *args]
        self.addCleanup(setattr, sync.sys, "argv", argv)
        out = StringIO()
        code = 0
        with redirect_stdout(out):
            try:
                sync.main()
            except SystemExit as e:
                code = e.code
        return code, out.getvalue()

    def test_repository_is_in_sync(self):
        code, out = self.run_script("--check")
        self.assertEqual(code, 0, out)

    def test_check_reports_a_changed_default(self):
        settings = self.defaults / "settings.json"
        data = json.loads(settings.read_text())
        data["name"] = "changed"
        settings.write_text(json.dumps(data))
        code, out = self.run_script("--check")
        self.assertEqual(code, 1)
        self.assertIn("settings.json", out)

    def test_check_reports_a_hand_edit(self):
        (self.target / "implement/extra.json").write_text("{}")
        code, out = self.run_script("--check")
        self.assertEqual(code, 1)
        self.assertIn("implement/extra.json", out)

    def test_generate_repairs_and_keeps_unmanaged_files(self):
        (self.target / "config.json").write_text("{}")
        (self.target / "logs").mkdir()
        (self.target / "logs/keep.log").write_text("log")
        self.assertEqual(self.run_script()[0], 0)
        self.assertEqual(self.run_script("--check")[0], 0)
        self.assertTrue((self.target / "logs/keep.log").exists())

    def test_overrides_are_applied(self):
        self.run_script()
        config = json.loads((self.target / "config.json").read_text())
        self.assertEqual(config["protocols"]["redis"]["ip"], "redis")

    def test_override_for_an_unmanaged_file_fails(self):
        overrides = json.loads(self.overrides.read_text())
        overrides["unknown.json"] = {}
        self.overrides.write_text(json.dumps(overrides))
        code, _ = self.run_script("--check")
        self.assertNotEqual(code, 0)


if __name__ == "__main__":
    unittest.main()
