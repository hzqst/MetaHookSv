import argparse
import fnmatch
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


MAX_QUERY_BYTES = 16 * 1024
MAX_REQUEST_BYTES = 16 * 1024
MAX_QUERIES = 32
QUERY_TIMEOUT_SECONDS = 30
EXCLUDED_DIRECTORIES = {"thirdparty", "build", "output", "bin", "obj", "packages"}
EXCLUDED_FILES = ("*.designer.cs", "*.g.cs", "*.generated.*", "package-lock.json",
                  "packages.lock.json", "*.min.js", "*.min.css")
DIFF_EXCLUSIONS = [f":(exclude,glob,icase)**/{name}/**" for name in sorted(EXCLUDED_DIRECTORIES)]
DIFF_EXCLUSIONS += [f":(exclude,glob,icase)**/{name}" for name in EXCLUDED_FILES]
TOOL = {
    "name": "git_history",
    "description": "Read local Git history only. log lists commits; show reads a commit patch or a tracked text file at revision; diff compares base to revision; ls-tree lists tracked paths. Revisions must be ancestors of the release commit. No shell or arbitrary Git options. Output is bounded; paginate log with skip, or narrow by path.",
    "inputSchema": {
        "type": "object",
        "properties": {
            "command": {"type": "string", "enum": ["log", "show", "diff", "ls-tree"]},
            "revision": {"type": "string", "description": "Commit/tag, default release HEAD"},
            "base": {"type": "string", "description": "Required for diff; optional for log to list base..revision; ancestor commit/tag"},
            "path": {"type": "string", "description": "Optional literal repository-relative path; show reads this file at revision"},
            "limit": {"type": "integer", "minimum": 1, "maximum": 100},
            "skip": {"type": "integer", "minimum": 0, "maximum": 100000},
        },
        "required": ["command"],
        "additionalProperties": False,
    },
    "annotations": {"readOnlyHint": True, "destructiveHint": False, "openWorldHint": False},
}


class QueryError(Exception):
    pass


class GitHistory:
    def __init__(self, repository, budget):
        self.repository = Path(repository).resolve(strict=True)
        self.budget = budget
        self.queries = 0
        self.environment = {name: os.environ[name] for name in ("PATH", "SYSTEMROOT", "WINDIR") if name in os.environ}
        self.environment.update({"GIT_CONFIG_NOSYSTEM": "1", "GIT_CONFIG_GLOBAL": os.devnull,
                                 "GIT_CONFIG_SYSTEM": os.devnull, "GIT_OPTIONAL_LOCKS": "0",
                                 "GIT_TERMINAL_PROMPT": "0", "GIT_NO_REPLACE_OBJECTS": "1",
                                 "GIT_NO_LAZY_FETCH": "1", "LC_ALL": "C.UTF-8"})
        self.head = self.run("rev-parse", "--verify", "HEAD^{commit}", cap=128).strip()

    def run(self, *arguments, cap=MAX_QUERY_BYTES):
        command = ["git", "--no-pager", "-C", str(self.repository),
                   "-c", "core.fsmonitor=false", "-c", f"core.hooksPath={os.devnull}",
                   "-c", f"core.attributesFile={os.devnull}", "-c", "protocol.allow=never",
                   "-c", "credential.helper=", *arguments]
        with tempfile.TemporaryFile() as output:
            try:
                result = subprocess.run(command, env=self.environment, stdout=output,
                                        stderr=subprocess.DEVNULL, timeout=QUERY_TIMEOUT_SECONDS, check=False)
            except (OSError, subprocess.TimeoutExpired):
                raise QueryError("Git query failed or timed out") from None
            if result.returncode:
                raise QueryError("Git query failed: check revision and path")
            output.seek(0)
            body = output.read(cap + 1)
        if b"\0" in body:
            return "[Binary content omitted]"
        marker = b"\n[TRUNCATED: narrow the query or paginate]\n"
        body = body.decode("utf-8", errors="replace").encode("utf-8")
        if len(body) > cap:
            return body[:cap - len(marker)].decode("utf-8", errors="ignore") + marker.decode()
        return body.decode("utf-8")

    def revision(self, value):
        if not isinstance(value, str) or not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_./~^{}-]{0,199}", value):
            raise QueryError("Invalid revision")
        resolved = self.run("rev-parse", "--verify", "--end-of-options", value + "^{commit}", cap=128).strip()
        self.run("merge-base", "--is-ancestor", resolved, self.head)
        return resolved

    def query(self, arguments):
        self.queries += 1
        if self.queries > MAX_QUERIES or self.budget < 128:
            raise QueryError("Git evidence budget exhausted; summarize the evidence already available")
        if not isinstance(arguments, dict) or set(arguments) - set(TOOL["inputSchema"]["properties"]):
            raise QueryError("Unknown Git query arguments")
        command = arguments.get("command")
        if command not in ("log", "show", "diff", "ls-tree"):
            raise QueryError("Only log, show, diff and ls-tree are allowed")
        limit, skip = arguments.get("limit", 30), arguments.get("skip", 0)
        if type(limit) is not int or not 1 <= limit <= 100 or type(skip) is not int or not 0 <= skip <= 100000:
            raise QueryError("Invalid pagination")
        path = arguments.get("path", "")
        if not isinstance(path, str) or (path and (path.startswith(("/", "-")) or "\\" in path
                or ":" in path or any(ord(character) < 32 for character in path)
                or any(part in ("", ".", "..", ".git") for part in path.lower().split("/")))):
            raise QueryError("Expected a literal repository-relative path")
        if path and (set(path.lower().split("/")) & EXCLUDED_DIRECTORIES
                     or any(fnmatch.fnmatchcase(path.lower().split("/")[-1], pattern) for pattern in EXCLUDED_FILES)):
            raise QueryError("Generated and third-party source bodies are excluded")
        revision = self.revision(arguments.get("revision", self.head))
        paths = [f":(literal){path}" if path else ".", *DIFF_EXCLUSIONS]
        if command == "log":
            if "base" in arguments:
                revision = f'{self.revision(arguments["base"])}..{revision}'
            query = ["log", "--no-show-signature", f"--max-count={limit}", f"--skip={skip}", "--format=%H %s%n%b", revision, "--", *paths]
        elif command == "show":
            query = (["show", "--no-show-signature", "--no-ext-diff", "--no-textconv", f"{revision}:{path}"] if path else
                     ["show", "--no-show-signature", "--no-ext-diff", "--no-textconv", "--ignore-submodules=all",
                      "--format=fuller", revision, "--", *paths])
        elif command == "diff":
            base = self.revision(arguments.get("base"))
            query = ["diff", "--no-ext-diff", "--no-textconv", "--ignore-submodules=all", "--no-renames", base, revision, "--", *paths]
        else:
            query = ["--literal-pathspecs", "ls-tree", "-r", "--name-only", revision, "--"]
            if path:
                query.append(path)
        result = self.run(*query, cap=min(MAX_QUERY_BYTES, self.budget))
        encoded = result.encode("utf-8")[:self.budget]
        self.budget -= len(encoded)
        return encoded.decode("utf-8", errors="ignore")


def serve(repository, budget):
    history = GitHistory(repository, budget)
    for line in sys.stdin.buffer:
        if len(line) > MAX_REQUEST_BYTES:
            return
        try:
            request = json.loads(line)
            if "id" not in request:
                continue
            method = request.get("method")
            if method == "initialize":
                result = {"protocolVersion": "2024-11-05", "capabilities": {"tools": {}},
                          "serverInfo": {"name": "release-git", "version": "1.0.0"}}
            elif method == "ping":
                result = {}
            elif method == "tools/list":
                result = {"tools": [TOOL]}
            elif method == "tools/call":
                params = request.get("params", {})
                try:
                    if params.get("name") != TOOL["name"]:
                        raise QueryError("Unknown tool")
                    text = history.query(params.get("arguments", {}))
                    result = {"content": [{"type": "text", "text": text}], "isError": False}
                except QueryError as error:
                    result = {"content": [{"type": "text", "text": str(error)}], "isError": True}
            else:
                print(json.dumps({"jsonrpc": "2.0", "id": request["id"],
                                  "error": {"code": -32601, "message": "Method not found"}}), flush=True)
                continue
            print(json.dumps({"jsonrpc": "2.0", "id": request["id"], "result": result}), flush=True)
        except (ValueError, TypeError, KeyError):
            return


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Read-only Git history MCP server")
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--budget", type=int, required=True)
    options = parser.parse_args()
    serve(options.repository, options.budget)
