from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
import threading
import unittest

from test_release import NOTES, release


@unittest.skipUnless(os.environ.get("RELEASE_CLI_SMOKE") == "1", "Opt-in test: requires pinned CLIs on PATH")
class CliSmokeTests(unittest.TestCase):
    def setUp(self):
        self.requests = []
        self.inject_tool = False
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
                    events = test.responses_events()
                else:
                    events = test.messages_events()
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

    def responses_events(self):
        item = {"id": "msg_test", "type": "message", "role": "assistant", "status": "completed",
                "content": [{"type": "output_text", "text": NOTES, "annotations": []}]}
        response = {"id": "resp_test", "object": "response", "created_at": 1, "model": "release-test-model",
                    "status": "completed", "output": [item],
                    "usage": {"input_tokens": 20, "output_tokens": 20, "total_tokens": 40}}
        if self.inject_tool:
            return [("response.output_item.added", {"item": {"type": "function_call", "name": "view_image",
                     "call_id": "call_test", "arguments": '{"path":"/etc/passwd"}'}}),
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

    def messages_events(self):
        message = {"id": "msg_test", "type": "message", "role": "assistant", "model": "release-test-model",
                   "content": [], "stop_reason": None, "stop_sequence": None,
                   "usage": {"input_tokens": 20, "output_tokens": 0}}
        return [
            ("message_start", {"message": message}),
            ("content_block_start", {"index": 0, "content_block": {"type": "text", "text": ""}}),
            ("content_block_delta", {"index": 0, "delta": {"type": "text_delta", "text": NOTES}}),
            ("content_block_stop", {"index": 0}),
            ("message_delta", {"delta": {"stop_reason": "end_turn", "stop_sequence": None}, "usage": {"output_tokens": 20}}),
            ("message_stop", {}),
        ]

    def test_pinned_clis_generate_notes_without_upstream_tools(self):
        for provider in ("codex", "claude"):
            with self.subTest(provider=provider):
                self.assertEqual(NOTES, release.run_cli_once("Generate bilingual notes. Never use tools.", provider,
                                                            "release-test-model", self.endpoint, "fake-test-key"))
        self.assertTrue(any("responses" in path for path, _ in self.requests))
        self.assertTrue(any("messages" in path for path, _ in self.requests))
        for path, payload in self.requests:
            self.assertEqual([], payload.get("tools", []))
            if "responses" in path:
                self.assertEqual("none", payload["tool_choice"])

    def test_codex_upstream_tool_response_fails_closed(self):
        self.inject_tool = True
        with self.assertRaises((release.ReleaseError, release.subprocess.SubprocessError)):
            release.run_cli_once("Generate notes.", "codex", "release-test-model", self.endpoint, "fake-test-key")


if __name__ == "__main__":
    unittest.main()
