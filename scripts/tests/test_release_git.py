import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


SPEC = importlib.util.spec_from_file_location("release_git", Path(__file__).parents[1] / "release_git.py")
release_git = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(release_git)


class GitHistoryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.git("init", "-q")
        self.git("config", "user.name", "Test")
        self.git("config", "user.email", "test@example.invalid")
        (self.root / "code.cpp").write_text("initial source\n", encoding="utf-8")
        self.git("add", ".")
        self.git("commit", "-qm", "initial history evidence")
        self.git("tag", "v1")
        (self.root / "code.cpp").write_text("updated source\n", encoding="utf-8")
        self.git("add", ".")
        self.git("commit", "-qm", "fix source evidence")
        self.history = release_git.GitHistory(self.root, 100 * 1024)

    def git(self, *arguments):
        return subprocess.check_output(["git", "-C", str(self.root), *arguments], text=True).strip()

    def snapshot(self):
        return {path.relative_to(self.root).as_posix(): path.read_bytes()
                for path in self.root.rglob("*") if path.is_file()}

    def test_read_history_source_diff_and_tree_without_any_repository_write(self):
        before = self.snapshot()
        self.assertIn("fix source evidence", self.history.query({"command": "log", "limit": 1}))
        self.assertNotIn("fix source evidence", self.history.query({"command": "log", "limit": 1, "skip": 1}))
        self.assertIn("initial history evidence", self.history.query({"command": "log", "revision": "v1"}))
        ranged = self.history.query({"command": "log", "base": "v1"})
        self.assertIn("fix source evidence", ranged)
        self.assertNotIn("initial history evidence", ranged)
        self.assertEqual("initial source\n", self.history.query({"command": "show", "revision": "v1", "path": "code.cpp"}))
        self.assertIn("+updated source", self.history.query({"command": "show"}))
        self.assertIn("+updated source", self.history.query({"command": "diff", "base": "v1"}))
        self.assertIn("code.cpp", self.history.query({"command": "ls-tree"}))
        self.assertEqual(before, self.snapshot())

    def test_write_commands_option_injection_and_path_escape_are_rejected(self):
        before = self.snapshot()
        cases = [{"command": command} for command in ("reset", "checkout", "clean", "commit", "config", "fetch", "push")]
        cases += [{"command": "log", "args": ["--output=owned"]},
                  {"command": "log", "revision": "--output=owned"},
                  {"command": "show", "revision": "HEAD; touch owned"},
                  {"command": "show", "revision": "HEAD:code.cpp"},
                  {"command": "show", "path": "../secret"},
                  {"command": "show", "path": ".git/config"},
                  {"command": "show", "path": "/etc/passwd"},
                  {"command": "show", "path": ":(top)*"},
                  {"command": "log", "limit": True}]
        for arguments in cases:
            with self.subTest(arguments=arguments), self.assertRaises(release_git.QueryError):
                self.history.query(arguments)
        self.assertEqual(before, self.snapshot())

    def test_thirdparty_generated_and_binary_bodies_are_not_returned(self):
        for relative in ("thirdparty/vendor.cpp", "nested/thirdparty/vendor.cpp", "Build/generated.cpp", "nested/code.g.cs"):
            path = self.root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("excluded-body-marker\n", encoding="utf-8")
        (self.root / "binary.dat").write_bytes(b"\0binary-secret")
        self.git("add", ".")
        self.git("commit", "-qm", "add excluded material")
        self.history = release_git.GitHistory(self.root, 100 * 1024)
        self.assertNotIn("excluded-body-marker", self.history.query({"command": "diff", "base": "v1"}))
        self.assertNotIn("excluded-body-marker", self.history.query({"command": "show"}))
        self.assertEqual("[Binary content omitted]", self.history.query({"command": "show", "path": "binary.dat"}))
        for path in ("thirdparty/vendor.cpp", "nested/thirdparty/vendor.cpp", "Build/generated.cpp", "nested/code.g.cs"):
            with self.assertRaises(release_git.QueryError):
                self.history.query({"command": "show", "path": path})

    def test_nonancestor_revision_is_rejected(self):
        self.git("checkout", "-q", "--orphan", "unrelated")
        self.git("commit", "-qm", "unrelated")
        self.git("tag", "v-other")
        with self.assertRaises(release_git.QueryError):
            self.history.query({"command": "log", "revision": "v-other"})

    def test_output_and_call_budgets_are_enforced(self):
        self.history.budget = 128
        self.assertLessEqual(len(self.history.query({"command": "show"}).encode("utf-8")), 128)
        with self.assertRaises(release_git.QueryError):
            self.history.query({"command": "log"})
        self.history.budget = 10000
        self.history.queries = release_git.MAX_QUERIES
        with self.assertRaises(release_git.QueryError):
            self.history.query({"command": "log"})

    def test_git_has_no_credentials_and_cannot_invoke_external_diff(self):
        before = self.snapshot()
        with patch.dict(os.environ, {"GH_TOKEN": "github-secret", "RELEASE_NOTES_API_KEY": "api-secret",
                                     "GIT_EXTERNAL_DIFF": "not-a-program", "GIT_CONFIG_COUNT": "1"}):
            history = release_git.GitHistory(self.root, 10000)
            self.assertNotIn("GH_TOKEN", history.environment)
            self.assertNotIn("RELEASE_NOTES_API_KEY", history.environment)
            self.assertNotIn("GIT_EXTERNAL_DIFF", history.environment)
            self.assertNotIn("GIT_CONFIG_COUNT", history.environment)
            self.assertIn("+updated source", history.query({"command": "diff", "base": "v1"}))
        self.assertEqual(before, self.snapshot())

    def test_non_utf8_source_is_bounded_after_decoding(self):
        (self.root / "legacy.cpp").write_bytes(b"\xff" * 20000)
        self.git("add", ".")
        self.git("commit", "-qm", "legacy encoding")
        history = release_git.GitHistory(self.root, 100 * 1024)
        result = history.query({"command": "show", "path": "legacy.cpp"})
        self.assertLessEqual(len(result.encode("utf-8")), release_git.MAX_QUERY_BYTES)
        self.assertIn("TRUNCATED", result)

    def test_git_query_timeout_fails_closed(self):
        with patch.object(release_git.subprocess, "run", side_effect=subprocess.TimeoutExpired("git", 30)):
            with self.assertRaises(release_git.QueryError):
                self.history.query({"command": "log"})

    def test_real_stdio_mcp_round_trip_and_write_rejection(self):
        before = self.snapshot()
        requests = [
            {"id": 1, "method": "initialize", "params": {"protocolVersion": "2024-11-05"}},
            {"method": "notifications/initialized"},
            {"id": 2, "method": "tools/list"},
            {"id": 3, "method": "tools/call", "params": {"name": "git_history", "arguments": {"command": "log"}}},
            {"id": 4, "method": "tools/call", "params": {"name": "git_history", "arguments": {"command": "reset"}}},
        ]
        result = subprocess.run([sys.executable, "-B", str(Path(release_git.__file__)), "--repository", str(self.root),
                                 "--budget", "10000"], input="\n".join(json.dumps(dict(request, jsonrpc="2.0")) for request in requests),
                                text=True, capture_output=True, timeout=30, check=True)
        responses = [json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(4, len(responses))
        self.assertIn("fix source evidence", responses[2]["result"]["content"][0]["text"])
        self.assertTrue(responses[3]["result"]["isError"])
        self.assertEqual(before, self.snapshot())
