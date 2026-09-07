from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import threading
import unittest

from test_release import NOTES, release


@unittest.skipUnless(os.environ.get("RELEASE_CLI_SMOKE") == "1", "Opt-in test: requires pinned CLIs on PATH")
class CliSmokeTests(unittest.TestCase):
    def setUp(self):
        self.requests = []
        self.round = 0
        self.native_write = None
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repository = Path(self.temp.name)
        for arguments in (("init", "-q"), ("config", "user.name", "Test"),
                          ("config", "user.email", "test@example.invalid")):
            subprocess.run(["git", "-C", str(self.repository), *arguments], check=True)
        (self.repository / "source.cpp").write_text("cli-history-source-marker\n", encoding="utf-8")
        subprocess.run(["git", "-C", str(self.repository), "add", "."], check=True)
        subprocess.run(["git", "-C", str(self.repository), "commit", "-qm", "cli-history-commit-marker"], check=True)
        test = self

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def do_POST(self):
                payload = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
                test.requests.append((self.path, payload))
                if "count_tokens" in self.path:
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.end_headers()
                    self.wfile.write(b'{"input_tokens":20}')
                    return
                if "responses" in self.path:
                    events = test.responses_events(payload)
                else:
                    events = test.messages_events(payload)
                test.round += 1
                self.send_response(200)
                self.send_header("Content-Type", "text/event-stream")
                self.end_headers()
                for sequence, (kind, data) in enumerate(events):
                    event = dict(data, type=kind, sequence_number=sequence)
                    self.wfile.write((f"event: {kind}\ndata: {json.dumps(event)}\n\n").encode())
                self.wfile.flush()

        self.server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.endpoint = f"http://127.0.0.1:{self.server.server_port}"
        self.addCleanup(self.stop_server)

    def stop_server(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()

    def tool_call(self, payload, provider):
        if self.native_write:
            if self.round:
                return None
            command = "printf modified > " + shlex.quote(str(self.repository / "source.cpp"))
            if self.native_write == "Write":
                return "Write", {"file_path": str(self.repository / "source.cpp"), "content": "modified"}
            if self.native_write == "Edit":
                return "Edit", {"file_path": str(self.repository / "source.cpp"),
                                "old_string": "cli-history-source-marker", "new_string": "modified"}
            return self.native_write, {"cmd" if self.native_write == "exec_command" else "command": command}
        if self.round >= 3:
            return None
        name = next((tool.get("name") for tool in payload.get("tools", []) if "git_history" in tool.get("name", "")), "missing_git_tool")
        arguments = ({"command": "log", "limit": 1}, {"command": "show", "path": "source.cpp"}, {"command": "reset"})
        return name, arguments[self.round]

    def responses_events(self, payload):
        item = {"id": "msg_test", "type": "message", "role": "assistant", "status": "completed",
                "content": [{"type": "output_text", "text": NOTES, "annotations": []}]}
        response = {"id": "resp_test", "object": "response", "created_at": 1, "model": "release-test-model",
                    "status": "completed", "output": [item],
                    "usage": {"input_tokens": 20, "output_tokens": 20, "total_tokens": 40}}
        tool = self.tool_call(payload, "codex")
        if tool:
            if self.native_write == "apply_patch":
                item = {"id": "patch_test", "type": "custom_tool_call", "name": "apply_patch", "call_id": "call_patch",
                        "input": f"*** Begin Patch\n*** Update File: {self.repository / 'source.cpp'}\n@@\n-cli-history-source-marker\n+modified\n*** End Patch"}
                response["output"] = [item]
                return [("response.created", {"response": dict(response, status="in_progress", output=[])}),
                        ("response.output_item.added", {"output_index": 0, "item": item}),
                        ("response.output_item.done", {"output_index": 0, "item": item}),
                        ("response.completed", {"response": response})]
            item = {"id": f"fc_{self.round}", "type": "function_call", "name": tool[0],
                    "call_id": f"call_{self.round}", "arguments": json.dumps(tool[1]), "status": "completed"}
            response["output"] = [item]
            return [("response.created", {"response": dict(response, status="in_progress", output=[])}),
                    ("response.output_item.added", {"output_index": 0, "item": dict(item, arguments="", status="in_progress")}),
                    ("response.function_call_arguments.delta", {"item_id": item["id"], "output_index": 0, "delta": item["arguments"]}),
                    ("response.function_call_arguments.done", {"item_id": item["id"], "output_index": 0, "arguments": item["arguments"]}),
                    ("response.output_item.done", {"output_index": 0, "item": item}),
                    ("response.completed", {"response": response})]
        return [
            ("response.created", {"response": dict(response, status="in_progress", output=[])}),
            ("response.output_item.added", {"output_index": 0, "item": dict(item, status="in_progress", content=[])}),
            ("response.content_part.added", {"item_id": "msg_test", "output_index": 0, "content_index": 0,
                                            "part": {"type": "output_text", "text": "", "annotations": []}}),
            ("response.output_text.delta", {"item_id": "msg_test", "output_index": 0, "content_index": 0, "delta": NOTES}),
            ("response.output_text.done", {"item_id": "msg_test", "output_index": 0, "content_index": 0, "text": NOTES}),
            ("response.content_part.done", {"item_id": "msg_test", "output_index": 0, "content_index": 0,
                                           "part": item["content"][0]}),
            ("response.output_item.done", {"output_index": 0, "item": item}),
            ("response.completed", {"response": response}),
        ]

    def messages_events(self, payload):
        message = {"id": "msg_test", "type": "message", "role": "assistant", "model": "release-test-model",
                   "content": [], "stop_reason": None, "stop_sequence": None,
                   "usage": {"input_tokens": 20, "output_tokens": 0}}
        tool = self.tool_call(payload, "claude")
        if tool:
            return [
                ("message_start", {"message": message}),
                ("content_block_start", {"index": 0, "content_block": {"type": "tool_use", "id": f"tool_{self.round}", "name": tool[0], "input": {}}}),
                ("content_block_delta", {"index": 0, "delta": {"type": "input_json_delta", "partial_json": json.dumps(tool[1])}}),
                ("content_block_stop", {"index": 0}),
                ("message_delta", {"delta": {"stop_reason": "tool_use", "stop_sequence": None}, "usage": {"output_tokens": 20}}),
                ("message_stop", {}),
            ]
        return [
            ("message_start", {"message": message}),
            ("content_block_start", {"index": 0, "content_block": {"type": "text", "text": ""}}),
            ("content_block_delta", {"index": 0, "delta": {"type": "text_delta", "text": NOTES}}),
            ("content_block_stop", {"index": 0}),
            ("message_delta", {"delta": {"stop_reason": "end_turn", "stop_sequence": None}, "usage": {"output_tokens": 20}}),
            ("message_stop", {}),
        ]

    def test_pinned_clis_query_git_and_reject_git_writes(self):
        before = {path.relative_to(self.repository): path.read_bytes()
                  for path in self.repository.rglob("*") if path.is_file()}
        for provider in ("codex", "claude"):
            with self.subTest(provider=provider):
                self.round = 0
                self.requests.clear()
                self.assertEqual(NOTES, release.run_cli_once("Investigate Git history and generate bilingual notes.", provider,
                                                            "release-test-model", self.endpoint, "fake-test-key", self.repository))
                exchanged = json.dumps(self.requests)
                self.assertIn("cli-history-commit-marker", exchanged)
                self.assertIn("cli-history-source-marker", exchanged)
                self.assertIn("Only log, show, diff and ls-tree are allowed", exchanged)
                self.assertEqual("cli-history-source-marker\n", (self.repository / "source.cpp").read_text())
                self.assertEqual(before, {path.relative_to(self.repository): path.read_bytes()
                                          for path in self.repository.rglob("*") if path.is_file()})

    def test_native_execution_and_editing_tools_cannot_write_repository(self):
        for provider, tool in (("codex", "shell_command"), ("codex", "exec_command"), ("codex", "apply_patch"),
                               ("claude", "Bash"), ("claude", "Write"), ("claude", "Edit")):
            with self.subTest(provider=provider, tool=tool):
                self.native_write = tool
                self.round = 0
                try:
                    release.run_cli_once("Generate notes.", provider, "release-test-model", self.endpoint,
                                         "fake-test-key", self.repository)
                except (release.ReleaseError, subprocess.SubprocessError):
                    pass
                self.assertGreater(self.round, 0)
                self.assertEqual("cli-history-source-marker\n", (self.repository / "source.cpp").read_text())


if __name__ == "__main__":
    unittest.main()
