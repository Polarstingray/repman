#!/usr/bin/env python3
"""
CLI integration tests for repman.
Run:  python3 tests/test_cli.py
      python3 -m pytest tests/test_cli.py
"""

import json
import os
import shutil
import subprocess
import tempfile
import unittest

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLI = os.path.join(PROJECT_ROOT, "cli", "repcli.py")

# Use the venv Python (which has python-dotenv installed) if available
_VENV_PYTHON = os.path.expanduser("~/.local/share/repman/cli/venv/bin/python3")
PYTHON = _VENV_PYTHON if os.path.exists(_VENV_PYTHON) else "python3"


class TestRepmanCLI(unittest.TestCase):

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="repman_test_")
        # Setting XDG_DATA_HOME redirects repman_get_data_dir() to the temp dir
        self.env = {**os.environ, "XDG_DATA_HOME": self.tmp}

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def run_cmd(self, *args, env=None):
        return subprocess.run(
            [PYTHON, CLI, *args],
            env=env if env is not None else self.env,
            capture_output=True,
            text=True,
        )

    @property
    def data_dir(self):
        return os.path.join(self.tmp, "repman")

    @property
    def installed_path(self):
        return os.path.join(self.data_dir, "index", "installed.json")

    def write_installed(self, packages):
        os.makedirs(os.path.join(self.data_dir, "index"), exist_ok=True)
        with open(self.installed_path, "w") as f:
            json.dump(packages, f)

    # ── ensure-dirs ────────────────────────────────────────────────────────────

    def test_ensure_dirs_exit_code(self):
        result = self.run_cmd("ensure-dirs")
        self.assertEqual(result.returncode, 0)

    def test_ensure_dirs_creates_subdirs(self):
        self.run_cmd("ensure-dirs")
        for subdir in ["index", "sig/index", "tmp", "cache", "packages"]:
            path = os.path.join(self.data_dir, subdir)
            self.assertTrue(os.path.isdir(path), f"missing subdir: {subdir}")

    def test_ensure_dirs_idempotent(self):
        self.assertEqual(self.run_cmd("ensure-dirs").returncode, 0)
        self.assertEqual(self.run_cmd("ensure-dirs").returncode, 0)

    # ── list ──────────────────────────────────────────────────────────────────

    def test_list_no_installed_file_exits_zero(self):
        result = self.run_cmd("list")
        self.assertEqual(result.returncode, 0)

    def test_list_no_installed_file_message(self):
        result = self.run_cmd("list")
        self.assertIn("No packages installed", result.stdout)

    def test_list_empty_installed_json(self):
        self.write_installed({})
        result = self.run_cmd("list")
        self.assertEqual(result.returncode, 0)
        self.assertIn("No packages installed", result.stdout)

    def test_list_shows_names_and_versions(self):
        self.write_installed({"ripgrep": "14.1.0", "jq": "1.7.1"})
        result = self.run_cmd("list")
        self.assertEqual(result.returncode, 0)
        self.assertIn("ripgrep", result.stdout)
        self.assertIn("14.1.0", result.stdout)
        self.assertIn("jq", result.stdout)
        self.assertIn("1.7.1", result.stdout)

    def test_list_one_line_per_package(self):
        self.write_installed({"ripgrep": "14.1.0", "jq": "1.7.1"})
        result = self.run_cmd("list")
        lines = [l for l in result.stdout.splitlines() if l.strip()]
        self.assertEqual(len(lines), 2)

    def test_list_single_package(self):
        self.write_installed({"bat": "0.24.0"})
        result = self.run_cmd("list")
        self.assertEqual(result.returncode, 0)
        lines = [l for l in result.stdout.splitlines() if l.strip()]
        self.assertEqual(len(lines), 1)
        self.assertIn("bat", lines[0])
        self.assertIn("0.24.0", lines[0])

    # ── install ────────────────────────────────────────────────────────────────

    def test_install_missing_name_exits_nonzero(self):
        result = self.run_cmd("install")
        self.assertNotEqual(result.returncode, 0)

    def test_install_unknown_package_no_index(self):
        # No index present — cannot resolve version, must fail
        result = self.run_cmd("install", "this-package-does-not-exist")
        self.assertNotEqual(result.returncode, 0)

    def test_install_unknown_package_with_version_no_index(self):
        result = self.run_cmd("install", "nonexistent", "-v", "1.0.0")
        self.assertNotEqual(result.returncode, 0)

    # ── uninstall ─────────────────────────────────────────────────────────────

    def test_uninstall_missing_name_exits_nonzero(self):
        result = self.run_cmd("uninstall")
        self.assertNotEqual(result.returncode, 0)

    def test_uninstall_not_installed_exits_nonzero(self):
        # No installed.json — package lookup fails
        result = self.run_cmd("uninstall", "nonexistent-pkg")
        self.assertNotEqual(result.returncode, 0)

    def test_uninstall_not_in_installed_json(self):
        self.write_installed({"other-pkg": "1.0.0"})
        result = self.run_cmd("uninstall", "nonexistent-pkg")
        self.assertNotEqual(result.returncode, 0)

    # ── upgrade ───────────────────────────────────────────────────────────────

    def test_upgrade_missing_installed_json_exits_nonzero(self):
        result = self.run_cmd("upgrade")
        self.assertNotEqual(result.returncode, 0)

    def test_upgrade_empty_installed_exits_zero(self):
        self.write_installed({})
        result = self.run_cmd("upgrade")
        self.assertEqual(result.returncode, 0)

    def test_upgrade_empty_installed_message(self):
        self.write_installed({})
        result = self.run_cmd("upgrade")
        self.assertIn("Everything up-to-date", result.stdout)

    # ── update ────────────────────────────────────────────────────────────────

    def test_update_bad_url_exits_nonzero(self):
        # Connection refused is an immediate failure — no network dependency
        bad_env = {**self.env, "INDEX_URL": "http://localhost:19999/nonexistent.json"}
        result = self.run_cmd("update", env=bad_env)
        self.assertNotEqual(result.returncode, 0)

    # ── fetch-key ─────────────────────────────────────────────────────────────

    def test_fetch_key_bad_url_exits_nonzero(self):
        bad_env = {**self.env, "PUBKEY_URL": "http://localhost:19999/nonexistent.pub"}
        result = self.run_cmd("fetch-key", env=bad_env)
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
