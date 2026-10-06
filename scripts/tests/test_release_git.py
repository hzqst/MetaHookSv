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

# Invocation-scoped: the fixture repositories must not start background
# maintenance that would rewrite .git storage while a read-only assertion runs.
GIT_MAINTENANCE_DISABLED = ("-c", "gc.auto=0", "-c", "maintenance.auto=false")


def worktree_snapshot(repository):
    """Byte snapshot of work tree content only.

    .git is excluded: loose objects, packs, packed-refs and index metadata are
    Git's internal storage and can be rewritten by background maintenance while
    the work tree bytes, HEAD, reference targets and staging area stay identical.
    Comparing them would make the read-only assertions depend on timing.
    """
    repository = Path(repository)
    snapshot = {}
    for path in repository.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(repository)
        if ".git" in relative.parts:
            continue
        snapshot[relative.as_posix()] = path.read_bytes()
    return snapshot


def repository_state(repository):
    """Journal-free state of every repository in the tree: HEAD, reference
    targets and staging area. Replaces the discarded .git byte comparison, so
    accidental reference or index writes are still caught."""
    repository = Path(repository)

    def query(root, *arguments):
        return subprocess.check_output(["git", "-C", str(root), *arguments], text=True,
                                       stderr=subprocess.DEVNULL).strip()

    roots = sorted({path.parent for path in repository.rglob(".git")} | {repository})
    state = {}
    for root in roots:
        state[root.relative_to(repository).as_posix()] = (
            query(root, "rev-parse", "--verify", "HEAD"),
            query(root, "for-each-ref", "--format=%(refname) %(objectname)"),
            query(root, "status", "--porcelain"),
        )
    return state


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
        self.state = repository_state(self.root)

    def git(self, *arguments):
        return subprocess.check_output(["git", "-C", str(self.root), *GIT_MAINTENANCE_DISABLED, *arguments],
                                       text=True).strip()

    def snapshot(self):
        return worktree_snapshot(self.root)

    def assert_repository_unchanged(self, before):
        self.assertEqual(before, self.snapshot())
        self.assertEqual(self.state, repository_state(self.root))

    def test_repository_content_comparison_survives_storage_compaction(self):
        # Git may pack loose objects or rewrite packed-refs in the background.
        # Storage layout is not repository content, so the read-only invariant
        # must hold across an explicit gc.
        before = self.snapshot()
        self.git("gc", "--quiet")
        self.assert_repository_unchanged(before)

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
        self.assert_repository_unchanged(before)

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
        self.assert_repository_unchanged(before)

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
        self.assert_repository_unchanged(before)

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
        self.assert_repository_unchanged(before)


class SubmoduleGitHistoryTests(unittest.TestCase):
    # The server runs with protocol.allow=never, but that only constrains its own
    # queries. The fixture writes gitlink entries straight into the index instead of
    # running `git submodule add`, so no protocol allowance and no .gitmodules entry
    # are needed: GitHistory builds its map from ls-tree, not from .gitmodules.
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.init_repo(self.root)
        (self.root / "aggregator.cpp").write_text("aggregator source\n", encoding="utf-8")
        self.git(self.root, "add", ".")
        self.git(self.root, "commit", "-qm", "aggregator initial history evidence")
        self.git(self.root, "tag", "v0")
        self.inner = self.root / "MetaSub"
        self.inner.mkdir()
        self.init_repo(self.inner)
        (self.inner / "inner.cpp").write_text("inner initial source\n", encoding="utf-8")
        self.git(self.inner, "add", ".")
        self.git(self.inner, "commit", "-qm", "inner initial evidence")
        self.link("MetaSub")
        self.git(self.root, "commit", "-qm", "add MetaSub submodule")
        self.git(self.root, "tag", "v1")
        (self.inner / "inner.cpp").write_text("inner updated source\n", encoding="utf-8")
        self.git(self.inner, "add", ".")
        self.git(self.inner, "commit", "-qm", "inner fix evidence")
        self.link("MetaSub")
        self.git(self.root, "commit", "-qm", "bump MetaSub submodule")
        vendor = self.root / "thirdparty" / "Vendor"
        vendor.mkdir(parents=True)
        self.init_repo(vendor)
        (vendor / "vendor.cpp").write_text("vendor-secret-marker\n", encoding="utf-8")
        self.git(vendor, "add", ".")
        self.git(vendor, "commit", "-qm", "vendor content")
        self.link("thirdparty/Vendor")
        self.git(self.root, "commit", "-qm", "add excluded vendor submodule")
        self.history = release_git.GitHistory(self.root, 100 * 1024)
        self.state = repository_state(self.root)

    def git(self, repository, *arguments):
        return subprocess.check_output(["git", "-C", str(repository), *GIT_MAINTENANCE_DISABLED, *arguments],
                                       text=True).strip()

    def init_repo(self, repository):
        self.git(repository, "init", "-q")
        self.git(repository, "config", "user.name", "Test")
        self.git(repository, "config", "user.email", "test@example.invalid")

    def link(self, name):
        # Submodule paths always use forward slashes, even on Windows.
        sha = self.git(self.root / Path(*name.split("/")), "rev-parse", "HEAD")
        self.git(self.root, "update-index", "--add", "--cacheinfo", f"160000,{sha},{name}")

    def snapshot(self):
        return worktree_snapshot(self.root)

    def assert_repository_unchanged(self, before):
        self.assertEqual(before, self.snapshot())
        self.assertEqual(self.state, repository_state(self.root))

    def test_submodule_log_show_diff_and_tree_are_reachable_without_any_repository_write(self):
        before = self.snapshot()
        self.assertIn("inner fix evidence", self.history.query({"command": "log", "submodule": "MetaSub", "limit": 5}))
        self.assertIn("inner initial evidence", self.history.query({"command": "log", "submodule": "MetaSub", "revision": "v1"}))
        ranged = self.history.query({"command": "log", "submodule": "MetaSub", "base": "v1"})
        self.assertIn("inner fix evidence", ranged)
        self.assertNotIn("inner initial evidence", ranged)
        self.assertEqual("inner initial source\n",
                         self.history.query({"command": "show", "submodule": "MetaSub", "revision": "v1", "path": "inner.cpp"}))
        self.assertIn("+inner updated source", self.history.query({"command": "show", "submodule": "MetaSub"}))
        self.assertIn("+inner updated source", self.history.query({"command": "diff", "submodule": "MetaSub", "base": "v1"}))
        self.assertIn("inner.cpp", self.history.query({"command": "ls-tree", "submodule": "MetaSub"}))
        self.assert_repository_unchanged(before)

    def test_submodule_names_are_validated_and_excluded(self):
        before = self.snapshot()
        cases = [{"command": "log", "submodule": "Missing"},
                 {"command": "log", "submodule": "../MetaSub"},
                 {"command": "log", "submodule": "MetaSub/.."},
                 {"command": "log", "submodule": "MetaSub\\.."},
                 {"command": "log", "submodule": "/MetaSub"},
                 {"command": "log", "submodule": "MetaSub/.git"},
                 {"command": "log", "submodule": "thirdparty/Vendor"},
                 {"command": "log", "submodule": 123},
                 {"command": "log", "submodule": "MetaSub", "path": "../inner.cpp"},
                 {"command": "log", "submodule": "MetaSub", "revision": "HEAD:inner.cpp"},
                 {"command": "log", "submodule": "MetaSub", "revision": "93e9eb1648f7cfe9c5b4d0d5a78707be825543d3"},
                 {"command": "log", "submodule": "93e9eb1648f7cfe9c5b4d0d5a78707be825543d3"}]
        for arguments in cases:
            with self.subTest(arguments=arguments), self.assertRaises(release_git.QueryError):
                self.history.query(arguments)
        self.assert_repository_unchanged(before)

    def test_submodule_absent_at_revision_is_rejected(self):
        with self.assertRaises(release_git.QueryError):
            self.history.query({"command": "log", "submodule": "MetaSub", "revision": "v0"})

    def test_submodule_queries_respect_budget_and_do_not_mutate_repository(self):
        before = self.snapshot()
        self.history.budget = 128
        self.assertLessEqual(len(self.history.query({"command": "show", "submodule": "MetaSub"}).encode("utf-8")), 128)
        with self.assertRaises(release_git.QueryError):
            self.history.query({"command": "log", "submodule": "MetaSub"})
        self.assert_repository_unchanged(before)

    def test_real_stdio_submodule_round_trip(self):
        before = self.snapshot()
        requests = [
            {"id": 1, "method": "initialize", "params": {"protocolVersion": "2024-11-05"}},
            {"method": "notifications/initialized"},
            {"id": 2, "method": "tools/list"},
            {"id": 3, "method": "tools/call", "params": {"name": "git_history",
                                                         "arguments": {"command": "log", "submodule": "MetaSub"}}},
            {"id": 4, "method": "tools/call", "params": {"name": "git_history",
                                                         "arguments": {"command": "log", "submodule": "thirdparty/Vendor"}}},
        ]
        result = subprocess.run([sys.executable, "-B", str(Path(release_git.__file__)), "--repository", str(self.root),
                                 "--budget", "10000"], input="\n".join(json.dumps(dict(request, jsonrpc="2.0")) for request in requests),
                                text=True, capture_output=True, timeout=30, check=True)
        responses = [json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(4, len(responses))
        self.assertIn("inner fix evidence", responses[2]["result"]["content"][0]["text"])
        self.assertTrue(responses[3]["result"]["isError"])
        self.assert_repository_unchanged(before)
