import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


SPEC = importlib.util.spec_from_file_location("release", Path(__file__).parents[1] / "release.py")
release = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(release)
NOTES = "## English\n- [Renderer] Fix cleanup.\n\n## \u4e2d\u6587\n- [Renderer] \u4fee\u590d\u6e05\u7406\u3002\n"


class FakeGitHub:
    def __init__(self, draft=None):
        self.release = draft
        self.assets = []
        self.calls = []
        self.sha = "abc123"
        self.fail_upload = False

    def request(self, method, path, data=None, missing_ok=False):
        self.calls.append((method, path, data))
        if path.startswith("git/ref/tags/"):
            return {"object": {"type": "commit", "sha": self.sha}}
        if path.startswith("releases/tags/"):
            return self.release
        if path == "releases" and method == "POST":
            self.release = dict(data, id=42)
            return self.release
        if path == "releases/42" and method == "PATCH":
            self.release.update(data)
            return self.release
        if path.startswith("releases/assets/") and method == "DELETE":
            asset_id = int(path.rsplit("/", 1)[1])
            self.assets = [asset for asset in self.assets if asset["id"] != asset_id]
            return None
        raise AssertionError((method, path, data))

    def paginate(self, path):
        if path == "releases":
            return [dict(self.release, tag_name="v1")] if self.release else []
        if path == "releases/42/assets":
            return list(self.assets)
        raise AssertionError(path)

    def upload(self, release_id, archive):
        if self.fail_upload:
            raise release.ReleaseError("Upload failed")
        self.assets.append({"id": len(self.calls) + len(self.assets), "name": archive.name,
                            "size": archive.stat().st_size, "state": "uploaded"})


class PublishingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for relative in release.EXPECTED_ASSETS:
            archive = self.root / relative
            archive.parent.mkdir(parents=True, exist_ok=True)
            archive.write_bytes(b"archive")
        self.api = FakeGitHub()

    def publish(self):
        release.publish_release(self.api, "v1", "abc123", self.root, NOTES)

    def test_complete_release_is_published_last(self):
        self.publish()
        self.assertFalse(self.api.release["draft"])
        self.assertEqual(5, len(self.api.assets))
        self.assertEqual(("PATCH", "releases/42", {"draft": False}), self.api.calls[-1])

    def test_missing_empty_extra_and_duplicate_assets_fail_before_api(self):
        archive = self.root / release.EXPECTED_ASSETS[0]
        for bad_case in ("empty", "missing", "extra", "duplicate"):
            with self.subTest(bad_case=bad_case):
                archive.write_bytes(b"archive")
                extra = self.root / (archive.name if bad_case == "duplicate" else "unexpected.7z")
                if bad_case == "empty":
                    archive.write_bytes(b"")
                elif bad_case == "missing":
                    archive.unlink()
                else:
                    extra.write_bytes(b"extra")
                with self.assertRaises(release.ReleaseError):
                    self.publish()
                self.assertEqual([], self.api.calls)
                extra.unlink(missing_ok=True)

    def test_existing_public_release_is_not_modified(self):
        self.api.release = {"id": 42, "draft": False}
        with self.assertRaises(release.ReleaseError):
            self.publish()
        self.assertTrue(all(method == "GET" for method, _, _ in self.api.calls))

    def test_upload_failure_keeps_draft_and_retry_completes(self):
        self.api.fail_upload = True
        with self.assertRaises(release.ReleaseError):
            self.publish()
        self.assertTrue(self.api.release["draft"])
        self.api.fail_upload = False
        self.api.assets = [{"id": 1, "name": Path(release.EXPECTED_ASSETS[0]).name,
                            "size": 1, "state": "starter"}]
        self.publish()
        self.assertFalse(self.api.release["draft"])
        self.assertEqual(1, sum(method == "POST" for method, _, _ in self.api.calls))

    def test_unexpected_draft_asset_is_not_deleted(self):
        self.api.release = {"id": 42, "draft": True}
        self.api.assets = [{"id": 1, "name": "manual.txt", "size": 1, "state": "uploaded"}]
        with self.assertRaises(release.ReleaseError):
            self.publish()
        self.assertFalse(any(method == "DELETE" for method, _, _ in self.api.calls))

    def test_moved_tag_blocks_creation(self):
        self.api.sha = "different"
        with self.assertRaises(release.ReleaseError):
            self.publish()
        self.assertIsNone(self.api.release)

    def test_tag_moved_during_upload_keeps_draft(self):
        upload = self.api.upload
        def move_tag(release_id, archive):
            upload(release_id, archive)
            self.api.sha = "different"
        self.api.upload = move_tag
        with self.assertRaises(release.ReleaseError):
            self.publish()
        self.assertTrue(self.api.release["draft"])

    def test_incorrect_uploaded_size_keeps_draft(self):
        upload = self.api.upload
        def wrong_size(release_id, archive):
            upload(release_id, archive)
            self.api.assets[-1]["size"] = 0
        self.api.upload = wrong_size
        with self.assertRaises(release.ReleaseError):
            self.publish()
        self.assertTrue(self.api.release["draft"])

    def test_annotated_tag_is_peeled(self):
        with patch.object(self.api, "request", side_effect=[
            {"object": {"type": "tag", "sha": "tag-object"}},
            {"object": {"type": "commit", "sha": "abc123"}},
        ]):
            release.verify_tag(self.api, "v1", "abc123")

    def test_multiple_releases_for_tag_are_rejected(self):
        with patch.object(self.api, "paginate", return_value=[
            {"id": 1, "tag_name": "v1", "draft": True},
            {"id": 2, "tag_name": "v1", "draft": True},
        ]):
            with self.assertRaises(release.ReleaseError):
                self.publish()

    def test_manual_publication_during_upload_blocks_final_patch(self):
        upload = self.api.upload
        def publish_manually(release_id, archive):
            upload(release_id, archive)
            self.api.release["draft"] = False
        self.api.upload = publish_manually
        with self.assertRaises(release.ReleaseError):
            self.publish()
        self.assertFalse(any(data == {"draft": False} for _, _, data in self.api.calls))


class ContextTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.git("init", "-q")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "user.name", "Test")
        self.commit("initial")
        self.git("tag", "v1")
        self.commit("fix cleanup")
        self.git("tag", "v2")
        self.head = self.git("rev-parse", "HEAD").strip()

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.root), *args], text=True)

    def commit(self, message):
        with (self.root / "code.cpp").open("a") as source:
            source.write(message + "\n")
        self.git("add", ".")
        self.git("commit", "-qm", message)

    def test_baseline_ignores_current_drafts_prereleases_and_nonancestors(self):
        self.git("checkout", "-q", "--orphan", "unrelated")
        self.commit("unrelated")
        self.git("tag", "v-other")
        self.git("checkout", "-q", "v2")
        releases = [
            {"tag_name": "missing-tag", "published_at": "2026-06-06"},
            {"tag_name": "v2", "published_at": "2026-06-05"},
            {"tag_name": "v-other", "published_at": "2026-06-04"},
            {"tag_name": "v2", "published_at": "2026-06-03", "draft": True},
            {"tag_name": "v2", "published_at": "2026-06-02", "prerelease": True},
            {"tag_name": "v1", "published_at": "2026-06-01"},
        ]
        self.assertEqual("v1", release.select_baseline(self.root, releases, "v2", self.head))

    def test_first_release_and_changed_source(self):
        context = release.build_context(self.root, [], "v2", self.head)
        self.assertIn("First release", context)
        self.assertIn("fix cleanup", context)
        self.assertLessEqual(len(context.encode("utf-8")), release.MAX_CONTEXT_BYTES)

    def test_context_is_utf8_bounded_and_excludes_thirdparty_diff(self):
        thirdparty = self.root / "thirdparty"
        thirdparty.mkdir()
        (thirdparty / "vendor.cpp").write_text("unique-vendor-body", encoding="utf-8")
        (self.root / "code.cpp").write_text("\u4e2d\u6587" * 120000, encoding="utf-8")
        self.git("add", ".")
        self.git("commit", "-qm", "large source change")
        head = self.git("rev-parse", "HEAD").strip()
        context = release.build_context(self.root, [{"tag_name": "v1", "body": "style"}], "v3", head)
        self.assertLessEqual(len(context.encode("utf-8")), release.MAX_CONTEXT_BYTES)
        self.assertIn("TRUNCATED", context)
        self.assertNotIn("unique-vendor-body", context)


class NotesTests(unittest.TestCase):
    def test_invalid_provider_and_endpoint(self):
        for provider, endpoint in (("other", "https://example.invalid"), ("codex", "http://example.invalid"),
                                   ("claude", "https://user:pass@example.invalid")):
            with self.subTest(provider=provider, endpoint=endpoint):
                with self.assertRaises(release.ReleaseError):
                    release.validate_ai_settings(provider, "model", endpoint, "secret")

    def test_missing_key_or_model(self):
        for model, key in (("", "key"), ("model", "")):
            with self.assertRaises(release.ReleaseError):
                release.validate_ai_settings("codex", model, "https://api.invalid", key)

    def test_real_subprocess_timeout_and_failure(self):
        with tempfile.TemporaryDirectory() as root:
            with patch.object(release, "CLI_TIMEOUT_SECONDS", 0.1):
                with self.assertRaises(subprocess.TimeoutExpired):
                    release.run_cli_process([sys.executable, "-c", "import time; time.sleep(30)"], "", root, os.environ.copy())
            with self.assertRaises(subprocess.CalledProcessError):
                release.run_cli_process([sys.executable, "-c", "raise SystemExit(3)"], "", root, os.environ.copy())

    def test_retry_on_failure_timeout_and_empty_output(self):
        for failure in (subprocess.TimeoutExpired("cli", 600),
                        subprocess.CalledProcessError(1, "cli"), release.ReleaseError("empty")):
            with self.subTest(failure=type(failure).__name__):
                with patch.object(release, "run_cli_once", side_effect=[failure, NOTES]) as run:
                    self.assertEqual(NOTES, release.generate_notes("context", "claude", "model", "https://api.invalid", "key"))
                    self.assertEqual(2, run.call_count)

    def test_exhausted_retries_fail_closed(self):
        with patch.object(release, "run_cli_once", side_effect=release.ReleaseError("empty")) as run:
            with self.assertRaises(release.ReleaseError):
                release.generate_notes("context", "codex", "model", "https://api.invalid", "key")
            self.assertEqual(2, run.call_count)

    def test_empty_or_nonbilingual_notes_fail(self):
        for text in ("", "   ", "error: authentication failed", "## English\nOnly English",
                     "## English\n\n## \u4e2d\u6587\n", "## English\nText\n\n## \u4e2d\u6587\n"):
            with self.assertRaises(release.ReleaseError):
                release.validate_notes(text)

    def test_codex_only_accepts_allowlisted_git_tool_events(self):
        release.validate_codex_events(json.dumps({"type": "item.completed", "item": {
            "type": "mcp_tool_call", "server": "release_git", "tool": "git_history"}}))
        for item_type in ("command_execution", "mcp_tool_call", "file_change", "web_search"):
            events = json.dumps({"type": "item.completed", "item": {"type": item_type}})
            with self.assertRaises(release.ReleaseError):
                release.validate_codex_events(events)

    def test_claude_error_result_is_rejected(self):
        with self.assertRaises(release.ReleaseError):
            release.claude_result(json.dumps({"is_error": True, "result": NOTES}))
        self.assertEqual(NOTES, release.claude_result(json.dumps({"is_error": False, "result": NOTES})))

    def test_cli_adapters_isolate_credentials_and_parse_results(self):
        def fake_process(command, context, work, environment):
            self.assertNotIn("GH_TOKEN", environment)
            self.assertNotIn("GITHUB_TOKEN", environment)
            self.assertNotIn("ACTIONS_RUNTIME_TOKEN", environment)
            self.assertNotIn("private-key", command)
            self.assertEqual([], list(work.iterdir()))
            if command[0] == "codex":
                self.assertEqual("private-key", environment["RELEASE_NOTES_API_KEY"])
                output = Path(command[command.index("--output-last-message") + 1])
                output.write_text(NOTES, encoding="utf-8")
                return json.dumps({"type": "item.completed", "item": {"type": "agent_message", "text": NOTES}})
            self.assertEqual("private-key", environment["ANTHROPIC_API_KEY"])
            self.assertEqual("", command[command.index("--tools") + 1])
            return json.dumps({"is_error": False, "result": NOTES})
        with patch.dict(os.environ, {"GH_TOKEN": "github-secret", "ACTIONS_RUNTIME_TOKEN": "artifact-secret"}):
            with patch.object(release, "run_cli_process", side_effect=fake_process):
                for provider in ("codex", "claude"):
                    self.assertEqual(NOTES, release.run_cli_once("context", provider, "model", "https://api.invalid", "private-key"))

    def test_credential_in_model_output_is_rejected(self):
        with patch.object(release, "run_cli_once", return_value=NOTES + "private-key"):
            with self.assertRaises(release.ReleaseError):
                release.generate_notes("context", "claude", "model", "https://api.invalid", "private-key")

if __name__ == "__main__":
    unittest.main()
