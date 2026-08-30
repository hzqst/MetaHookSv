#!/usr/bin/env python3
"""Download, validate, and atomically publish MetaHook gamedata."""

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
import time
import uuid
from contextlib import contextmanager
from dataclasses import dataclass
from http.client import HTTPException
from pathlib import Path
from typing import Dict, Iterator, List, Optional, Set, Tuple
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin, urlsplit
from urllib.request import Request, urlopen


DEFAULT_INDEX_URL = (
    "https://hlnd2t.github.io/GoldSrc_VibeSignatures/gamesymbols/index.json"
)
INDEX_URL_ENVIRONMENT_VARIABLE = "GOLDSRC_VIBESIGNATURES_INDEX_URL"
INDEX_FILE_NAME = "index.json"
SUPPORTED_INDEX_SCHEMA_VERSION = 4
SUPPORTED_SNAPSHOT_SCHEMA_VERSION = 3
SUPPORTED_SNAPSHOT_CONTRACT_VERSION = 6
MAXIMUM_INDEX_BYTES = 1024 * 1024
MAXIMUM_SNAPSHOT_BYTES = 16 * 1024 * 1024
MAXIMUM_VERSIONS = 4096
MAXIMUM_RECORDS = 100000
HTTP_TIMEOUT_SECONDS = 120
HTTP_ATTEMPTS = 3
HTTP_RETRY_DELAYS_SECONDS = (1, 2)
LOCK_TIMEOUT_SECONDS = 600
HTTP_USER_AGENT = "MetaHook-GameDataUpdater/1.0"
GAME_VERSION_PATTERN = re.compile(r"^[A-Za-z0-9_]+-[0-9]+[A-Za-z]*$")
LOWERCASE_SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")
SHA256_PATTERN = re.compile(r"^[0-9A-Fa-f]{64}$")


class UpdateError(Exception):
    pass


class RetryableDownloadError(Exception):
    pass


class DuplicateJsonMemberError(ValueError):
    pass


@dataclass(frozen=True)
class SnapshotEntry:
    game_version: str
    file_name: str
    sha256: str
    size: int
    snapshot_schema_version: int
    file_count: int
    last_publish_time: str


@dataclass(frozen=True)
class GameDataIndex:
    raw: bytes
    entries: List[SnapshotEntry]


def log(message: str) -> None:
    print("[MetaHook gamedata] {}".format(message), flush=True)


def require_python_version() -> None:
    if sys.version_info < (3, 8):
        raise UpdateError(
            "Python 3.8 or newer is required; current version is {}.{}.{}".format(
                sys.version_info.major,
                sys.version_info.minor,
                sys.version_info.micro,
            )
        )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download, validate, and publish MetaHook gamedata."
    )
    parser.add_argument(
        "--target-dir",
        required=True,
        help="Destination metahook/gamedata directory.",
    )
    parser.add_argument(
        "--temp-root",
        help="Same-volume directory used for verified staging data.",
    )
    parser.add_argument(
        "--index-url",
        help=(
            "HTTPS index URL. Defaults to {} or the built-in catalog URL.".format(
                INDEX_URL_ENVIRONMENT_VARIABLE
            )
        ),
    )
    parser.add_argument(
        "--validate-only",
        action="store_true",
        help="Validate the existing target directory without using the network.",
    )
    return parser.parse_args()


def normalize_index_url(argument_value: Optional[str]) -> str:
    index_url = (argument_value or "").strip()
    if not index_url:
        index_url = os.environ.get(INDEX_URL_ENVIRONMENT_VARIABLE, "").strip()
    if not index_url:
        index_url = DEFAULT_INDEX_URL

    parsed = urlsplit(index_url)
    if parsed.scheme.lower() != "https" or not parsed.netloc:
        raise UpdateError("index URL must be an absolute HTTPS URL: {!r}".format(index_url))
    if parsed.username or parsed.password:
        raise UpdateError("index URL must not contain user information")
    if parsed.query or parsed.fragment:
        raise UpdateError("index URL must not contain a query string or fragment")
    if not parsed.path or parsed.path.endswith("/"):
        raise UpdateError("index URL must identify index.json")
    return index_url


def reject_duplicate_json_members(
    pairs: List[Tuple[str, object]],
) -> Dict[str, object]:
    result: Dict[str, object] = {}
    for name, value in pairs:
        if name in result:
            raise DuplicateJsonMemberError(
                "duplicate JSON object member: {}".format(name)
            )
        result[name] = value
    return result


def reject_json_constant(value: str) -> object:
    raise ValueError("invalid JSON constant: {}".format(value))


def parse_json_document(contents: bytes, description: str) -> object:
    try:
        text = contents.decode("utf-8")
    except UnicodeDecodeError as error:
        raise UpdateError("{} is not valid UTF-8: {}".format(description, error)) from error

    try:
        return json.loads(
            text,
            object_pairs_hook=reject_duplicate_json_members,
            parse_constant=reject_json_constant,
        )
    except (json.JSONDecodeError, DuplicateJsonMemberError, ValueError) as error:
        raise UpdateError("{} is not valid JSON: {}".format(description, error)) from error


def is_uint(value: object, maximum: int) -> bool:
    return (
        isinstance(value, int)
        and not isinstance(value, bool)
        and 0 <= value <= maximum
    )


def require_string(
    source: Dict[str, object],
    name: str,
    description: str,
    maximum_length: int,
) -> str:
    value = source.get(name)
    if not isinstance(value, str) or not value or len(value) > maximum_length:
        raise UpdateError(
            "{} contains a missing or invalid string field: {}".format(
                description,
                name,
            )
        )
    return value


def require_uint(
    source: Dict[str, object],
    name: str,
    description: str,
    maximum: int,
) -> int:
    value = source.get(name)
    if not is_uint(value, maximum):
        raise UpdateError(
            "{} contains a missing or invalid unsigned field: {}".format(
                description,
                name,
            )
        )
    return value


def is_safe_snapshot_file_name(file_name: str) -> bool:
    return (
        6 <= len(file_name) <= 255
        and file_name not in (".", "..")
        and "\0" not in file_name
        and "/" not in file_name
        and "\\" not in file_name
        and ":" not in file_name
        and file_name.endswith(".json")
    )


def parse_index(contents: bytes, description: str) -> GameDataIndex:
    if not contents or len(contents) > MAXIMUM_INDEX_BYTES:
        raise UpdateError("{} size is invalid".format(description))

    document = parse_json_document(contents, description)
    if not isinstance(document, dict):
        raise UpdateError("{} root is not an object".format(description))

    schema_version = require_uint(
        document,
        "schemaVersion",
        description,
        0xFFFFFFFF,
    )
    if schema_version != SUPPORTED_INDEX_SCHEMA_VERSION:
        raise UpdateError(
            "{} schemaVersion {} is unsupported".format(description, schema_version)
        )

    versions = document.get("versions")
    if not isinstance(versions, list) or len(versions) > MAXIMUM_VERSIONS:
        raise UpdateError("{} contains an invalid versions array".format(description))

    entries: List[SnapshotEntry] = []
    seen_game_versions: Set[str] = set()
    seen_file_names: Set[str] = set()
    for ordinal, value in enumerate(versions):
        entry_description = "{} version entry {}".format(description, ordinal)
        if not isinstance(value, dict):
            raise UpdateError("{} is not an object".format(entry_description))

        game_version = require_string(value, "gameVersion", entry_description, 64)
        if not GAME_VERSION_PATTERN.fullmatch(game_version):
            raise UpdateError(
                "{} contains an invalid gameVersion: {!r}".format(
                    entry_description,
                    game_version,
                )
            )
        if game_version in seen_game_versions:
            raise UpdateError(
                "{} contains duplicate gameVersion: {}".format(
                    description,
                    game_version,
                )
            )
        seen_game_versions.add(game_version)

        file_name = require_string(value, "url", entry_description, 255)
        sha256 = require_string(value, "sha256", entry_description, 64)
        size = require_uint(value, "size", entry_description, MAXIMUM_SNAPSHOT_BYTES)
        snapshot_schema_version = require_uint(
            value,
            "snapshotSchemaVersion",
            entry_description,
            0xFFFFFFFF,
        )
        file_count = require_uint(
            value,
            "fileCount",
            entry_description,
            MAXIMUM_RECORDS,
        )
        last_publish_time = require_string(
            value,
            "lastPublishTime",
            entry_description,
            128,
        )

        if not is_safe_snapshot_file_name(file_name):
            raise UpdateError(
                "{} contains an unsafe snapshot URL: {!r}".format(
                    entry_description,
                    file_name,
                )
            )
        if file_name in seen_file_names:
            raise UpdateError(
                "{} contains duplicate snapshot URL: {}".format(
                    description,
                    file_name,
                )
            )
        seen_file_names.add(file_name)
        if not LOWERCASE_SHA256_PATTERN.fullmatch(sha256):
            raise UpdateError(
                "{} contains an invalid lowercase SHA-256".format(entry_description)
            )
        if size == 0:
            raise UpdateError("{} contains an invalid size".format(entry_description))
        if snapshot_schema_version != SUPPORTED_SNAPSHOT_CONTRACT_VERSION:
            raise UpdateError(
                "{} uses unsupported snapshot contract {}".format(
                    entry_description,
                    snapshot_schema_version,
                )
            )

        expected_file_name = "{}.{}.json".format(game_version, sha256)
        if file_name != expected_file_name:
            raise UpdateError(
                "{} URL is not content-addressed; expected {!r}".format(
                    entry_description,
                    expected_file_name,
                )
            )

        entries.append(
            SnapshotEntry(
                game_version=game_version,
                file_name=file_name,
                sha256=sha256,
                size=size,
                snapshot_schema_version=snapshot_schema_version,
                file_count=file_count,
                last_publish_time=last_publish_time,
            )
        )

    return GameDataIndex(raw=contents, entries=entries)


def validate_snapshot_contents(contents: bytes, entry: SnapshotEntry) -> None:
    description = "snapshot {}".format(entry.game_version)
    if len(contents) != entry.size:
        raise UpdateError(
            "{} size mismatch: expected {}, got {}".format(
                description,
                entry.size,
                len(contents),
            )
        )

    actual_sha256 = hashlib.sha256(contents).hexdigest()
    if actual_sha256 != entry.sha256:
        raise UpdateError(
            "{} SHA-256 mismatch: expected {}, got {}".format(
                description,
                entry.sha256,
                actual_sha256,
            )
        )

    document = parse_json_document(contents, description)
    if not isinstance(document, dict):
        raise UpdateError("{} root is not an object".format(description))

    schema_version = require_uint(
        document,
        "schemaVersion",
        description,
        0xFFFFFFFF,
    )
    if schema_version != SUPPORTED_SNAPSHOT_SCHEMA_VERSION:
        raise UpdateError(
            "{} schemaVersion {} is unsupported".format(description, schema_version)
        )

    source = document.get("source")
    if not isinstance(source, dict):
        raise UpdateError("{} source is not an object".format(description))

    source_game_version = require_string(
        source,
        "gameVersion",
        "{} source".format(description),
        64,
    )
    source_snapshot_schema_version = require_uint(
        source,
        "snapshotSchemaVersion",
        "{} source".format(description),
        0xFFFFFFFF,
    )
    require_uint(
        source,
        "configDigestVersion",
        "{} source".format(description),
        0xFFFFFFFF,
    )
    require_uint(
        source,
        "analysisOutputContractVersion",
        "{} source".format(description),
        0xFFFFFFFF,
    )
    config_sha256 = require_string(
        source,
        "configSha256",
        "{} source".format(description),
        80,
    )
    source_file_count = require_uint(
        source,
        "fileCount",
        "{} source".format(description),
        MAXIMUM_RECORDS,
    )
    require_string(
        source,
        "lastPublishTime",
        "{} source".format(description),
        128,
    )

    if source_game_version != entry.game_version:
        raise UpdateError("{} source.gameVersion does not match index".format(description))
    if source_snapshot_schema_version != entry.snapshot_schema_version:
        raise UpdateError(
            "{} source.snapshotSchemaVersion does not match index".format(description)
        )
    if source_file_count != entry.file_count:
        raise UpdateError("{} source.fileCount does not match index".format(description))
    if (
        not config_sha256.startswith("sha256:")
        or not SHA256_PATTERN.fullmatch(config_sha256[7:])
    ):
        raise UpdateError("{} source.configSha256 is invalid".format(description))

    records = document.get("records")
    if not isinstance(records, list) or len(records) != entry.file_count:
        raise UpdateError("{} records count does not match index".format(description))


def resolve_snapshot_url(index_url: str, file_name: str) -> str:
    snapshot_url = urljoin(index_url, file_name)
    index_parts = urlsplit(index_url)
    snapshot_parts = urlsplit(snapshot_url)
    if snapshot_parts.scheme.lower() != "https":
        raise UpdateError("snapshot URL must use HTTPS: {}".format(snapshot_url))
    if (
        snapshot_parts.hostname != index_parts.hostname
        or snapshot_parts.port != index_parts.port
    ):
        raise UpdateError("snapshot URL must not change host or port: {}".format(snapshot_url))
    return snapshot_url


def read_url_bytes(url: str, maximum_bytes: int) -> bytes:
    request = Request(
        url,
        headers={
            "Accept": "application/json",
            "User-Agent": HTTP_USER_AGENT,
        },
    )
    with urlopen(request, timeout=HTTP_TIMEOUT_SECONDS) as response:
        declared_length: Optional[int] = None
        content_length = response.headers.get("Content-Length")
        if content_length:
            try:
                parsed_length = int(content_length)
            except ValueError:
                parsed_length = -1
            if parsed_length >= 0:
                declared_length = parsed_length
            if declared_length is not None and declared_length > maximum_bytes:
                raise UpdateError(
                    "{} declares {} bytes, exceeding the {} byte limit".format(
                        url,
                        declared_length,
                        maximum_bytes,
                    )
                )

        contents = response.read(maximum_bytes + 1)
        if len(contents) > maximum_bytes:
            raise UpdateError("{} exceeds the {} byte limit".format(url, maximum_bytes))
        if declared_length is not None and len(contents) != declared_length:
            raise RetryableDownloadError(
                "{} returned {} bytes, but declared {}".format(
                    url,
                    len(contents),
                    declared_length,
                )
            )
        return contents


def format_download_error(error: BaseException) -> str:
    if isinstance(error, HTTPError):
        return "HTTP {} {}".format(error.code, error.reason)
    if isinstance(error, URLError):
        return str(error.reason)
    return str(error)


def fetch_index(url: str) -> bytes:
    last_error: Optional[BaseException] = None
    for attempt in range(1, HTTP_ATTEMPTS + 1):
        try:
            return read_url_bytes(url, MAXIMUM_INDEX_BYTES)
        except HTTPError as error:
            if 400 <= error.code < 500:
                raise UpdateError(
                    "index request failed without retry: {}".format(
                        format_download_error(error)
                    )
                ) from error
            last_error = error
        except UpdateError:
            raise
        except (
            RetryableDownloadError,
            URLError,
            HTTPException,
            OSError,
            TimeoutError,
        ) as error:
            last_error = error

        if attempt < HTTP_ATTEMPTS:
            delay = HTTP_RETRY_DELAYS_SECONDS[attempt - 1]
            log(
                "index download attempt {} failed: {}; retrying in {} second(s)".format(
                    attempt,
                    format_download_error(last_error),
                    delay,
                )
            )
            time.sleep(delay)

    raise UpdateError(
        "index download failed after {} attempts: {}".format(
            HTTP_ATTEMPTS,
            format_download_error(last_error),
        )
    )


def download_snapshot(
    url: str,
    destination: Path,
    entry: SnapshotEntry,
) -> None:
    last_error: Optional[BaseException] = None
    for attempt in range(1, HTTP_ATTEMPTS + 1):
        part_path = destination.with_name(destination.name + ".part")
        try:
            if part_path.exists():
                part_path.unlink()

            request = Request(
                url,
                headers={
                    "Accept": "application/json",
                    "User-Agent": HTTP_USER_AGENT,
                },
            )
            digest = hashlib.sha256()
            total_size = 0
            with urlopen(request, timeout=HTTP_TIMEOUT_SECONDS) as response:
                content_length = response.headers.get("Content-Length")
                if content_length:
                    try:
                        declared_length = int(content_length)
                    except ValueError:
                        declared_length = -1
                    if declared_length >= 0 and declared_length != entry.size:
                        raise RetryableDownloadError(
                            "Content-Length mismatch: expected {}, got {}".format(
                                entry.size,
                                declared_length,
                            )
                        )

                with part_path.open("wb") as output:
                    while True:
                        chunk = response.read(64 * 1024)
                        if not chunk:
                            break
                        total_size += len(chunk)
                        if total_size > entry.size:
                            raise RetryableDownloadError(
                                "download exceeded expected size {}".format(entry.size)
                            )
                        digest.update(chunk)
                        output.write(chunk)

            if total_size != entry.size:
                raise RetryableDownloadError(
                    "size mismatch: expected {}, got {}".format(
                        entry.size,
                        total_size,
                    )
                )
            actual_sha256 = digest.hexdigest()
            if actual_sha256 != entry.sha256:
                raise RetryableDownloadError(
                    "SHA-256 mismatch: expected {}, got {}".format(
                        entry.sha256,
                        actual_sha256,
                    )
                )

            part_path.replace(destination)
            validate_snapshot_contents(destination.read_bytes(), entry)
            return
        except HTTPError as error:
            if 400 <= error.code < 500:
                raise UpdateError(
                    "snapshot {} request failed without retry: {}".format(
                        entry.game_version,
                        format_download_error(error),
                    )
                ) from error
            last_error = error
        except RetryableDownloadError as error:
            last_error = error
        except UpdateError:
            raise
        except (URLError, HTTPException, OSError, TimeoutError) as error:
            last_error = error
        finally:
            if part_path.exists():
                part_path.unlink()

        if attempt < HTTP_ATTEMPTS:
            delay = HTTP_RETRY_DELAYS_SECONDS[attempt - 1]
            log(
                "snapshot {} download attempt {} failed: {}; retrying in {} second(s)".format(
                    entry.game_version,
                    attempt,
                    format_download_error(last_error),
                    delay,
                )
            )
            time.sleep(delay)

    raise UpdateError(
        "snapshot {} download failed after {} attempts: {}".format(
            entry.game_version,
            HTTP_ATTEMPTS,
            format_download_error(last_error),
        )
    )


def validate_target_path(target_dir: Path) -> Path:
    absolute_target = Path(os.path.abspath(str(target_dir)))
    if (
        absolute_target.name.lower() != "gamedata"
        or absolute_target.parent.name.lower() != "metahook"
    ):
        raise UpdateError(
            "refusing to update unexpected target directory: {}".format(absolute_target)
        )
    if absolute_target.is_symlink():
        raise UpdateError(
            "refusing to replace a symbolic-link target directory: {}".format(
                absolute_target
            )
        )
    if absolute_target.exists() and not absolute_target.is_dir():
        raise UpdateError(
            "target path exists but is not a directory: {}".format(absolute_target)
        )
    return absolute_target


def validate_temp_root(temp_root: Path, target_dir: Path) -> Path:
    absolute_temp_root = Path(os.path.abspath(str(temp_root)))
    if absolute_temp_root == target_dir or target_dir in absolute_temp_root.parents:
        raise UpdateError(
            "temporary root must not be inside the target directory: {}".format(
                absolute_temp_root
            )
        )
    if absolute_temp_root.anchor.lower() != target_dir.anchor.lower():
        raise UpdateError("temporary root and target directory must be on the same volume")
    absolute_temp_root.mkdir(parents=True, exist_ok=True)
    if absolute_temp_root.is_symlink() or not absolute_temp_root.is_dir():
        raise UpdateError(
            "temporary root is not a regular directory: {}".format(absolute_temp_root)
        )
    return absolute_temp_root


@contextmanager
def acquire_update_lock(target_dir: Path) -> Iterator[None]:
    try:
        import msvcrt
    except ImportError as error:
        raise UpdateError("the gamedata updater requires Windows") from error

    target_key = os.path.normcase(str(target_dir)).encode("utf-8")
    lock_root = Path(tempfile.gettempdir()) / "MetaHookGameDataUpdater"
    lock_root.mkdir(parents=True, exist_ok=True)
    lock_name = "metahook-gamedata-{}.lock".format(
        hashlib.sha256(target_key).hexdigest()
    )
    lock_path = lock_root / lock_name
    lock_file = lock_path.open("a+b")
    acquired = False
    deadline = time.monotonic() + LOCK_TIMEOUT_SECONDS
    waiting_logged = False

    try:
        if lock_path.stat().st_size == 0:
            lock_file.write(b"\0")
            lock_file.flush()

        while not acquired:
            try:
                lock_file.seek(0)
                msvcrt.locking(lock_file.fileno(), msvcrt.LK_NBLCK, 1)
                acquired = True
            except OSError as error:
                if time.monotonic() >= deadline:
                    raise UpdateError(
                        "timed out waiting for the gamedata update lock after {} seconds".format(
                            LOCK_TIMEOUT_SECONDS
                        )
                    ) from error
                if not waiting_logged:
                    log("waiting for another gamedata updater")
                    waiting_logged = True
                time.sleep(0.25)

        yield
    finally:
        if acquired:
            lock_file.seek(0)
            msvcrt.locking(lock_file.fileno(), msvcrt.LK_UNLCK, 1)
        lock_file.close()


def read_local_index(target_dir: Path) -> Optional[bytes]:
    index_path = target_dir / INDEX_FILE_NAME
    if index_path.is_symlink() or not index_path.is_file():
        return None
    try:
        if index_path.stat().st_size > MAXIMUM_INDEX_BYTES:
            return None
        return index_path.read_bytes()
    except OSError:
        return None


def validate_local_snapshot(
    target_dir: Path,
    entry: SnapshotEntry,
) -> Tuple[bool, str]:
    snapshot_path = target_dir / entry.file_name
    if snapshot_path.is_symlink() or not snapshot_path.is_file():
        return False, "file is missing or is not a regular file"

    try:
        if snapshot_path.stat().st_size != entry.size:
            return False, "file size does not match index"
        contents = snapshot_path.read_bytes()
        validate_snapshot_contents(contents, entry)
    except (OSError, UpdateError) as error:
        return False, str(error)
    return True, ""


def collect_target_names(target_dir: Path) -> Set[str]:
    if not target_dir.exists():
        return set()
    try:
        return {entry.name for entry in target_dir.iterdir()}
    except OSError as error:
        raise UpdateError(
            "failed to inspect target directory {}: {}".format(target_dir, error)
        ) from error


def copy_reusable_snapshot(
    source: Path,
    destination: Path,
    entry: SnapshotEntry,
) -> bool:
    try:
        shutil.copy2(str(source), str(destination))
        validate_snapshot_contents(destination.read_bytes(), entry)
        return True
    except (OSError, UpdateError) as error:
        log(
            "snapshot {} could not be reused after staging: {}".format(
                entry.game_version,
                error,
            )
        )
        if destination.exists():
            destination.unlink()
        return False


def publish_stage(stage_dir: Path, target_dir: Path) -> None:
    target_dir.parent.mkdir(parents=True, exist_ok=True)
    backup_dir = target_dir.parent / ".gamedata.backup.{}.{}".format(
        os.getpid(),
        uuid.uuid4().hex,
    )
    had_target = target_dir.exists()

    try:
        if had_target:
            target_dir.replace(backup_dir)
        try:
            stage_dir.replace(target_dir)
        except Exception:
            if had_target and backup_dir.exists() and not target_dir.exists():
                backup_dir.replace(target_dir)
            raise

        if had_target and backup_dir.exists():
            try:
                shutil.rmtree(str(backup_dir))
            except OSError as error:
                log(
                    "WARNING: installed the new generation but could not remove "
                    "backup {}: {}".format(backup_dir, error)
                )
    except OSError as error:
        if had_target and backup_dir.exists() and not target_dir.exists():
            backup_dir.replace(target_dir)
        raise UpdateError("failed to publish gamedata atomically: {}".format(error)) from error


def validate_existing_directory(target_dir: Path) -> None:
    local_index_raw = read_local_index(target_dir)
    if local_index_raw is None:
        raise UpdateError("index.json is missing or invalid in {}".format(target_dir))
    local_index = parse_index(local_index_raw, "local index")

    expected_names = {INDEX_FILE_NAME}
    expected_names.update(entry.file_name for entry in local_index.entries)
    actual_names = collect_target_names(target_dir)
    if actual_names != expected_names:
        missing_names = sorted(expected_names - actual_names)
        extra_names = sorted(actual_names - expected_names)
        details = []
        if missing_names:
            details.append("missing: {}".format(", ".join(missing_names)))
        if extra_names:
            details.append("undeclared: {}".format(", ".join(extra_names)))
        raise UpdateError(
            "target directory contents differ from index ({})".format(
                "; ".join(details)
            )
        )

    for entry in local_index.entries:
        valid, reason = validate_local_snapshot(target_dir, entry)
        if not valid:
            raise UpdateError(
                "snapshot {} failed validation: {}".format(entry.game_version, reason)
            )

    log(
        "offline validation passed for {} ({} snapshot(s))".format(
            target_dir,
            len(local_index.entries),
        )
    )


def update_game_data(index_url: str, target_dir: Path, temp_root: Path) -> None:
    log("checking {}".format(index_url))
    remote_index_raw = fetch_index(index_url)
    remote_index = parse_index(remote_index_raw, "remote index")

    expected_names = {INDEX_FILE_NAME}
    expected_names.update(entry.file_name for entry in remote_index.entries)
    actual_names = collect_target_names(target_dir)
    extra_names = sorted(actual_names - expected_names)

    local_index_raw = read_local_index(target_dir)
    index_is_identical = local_index_raw == remote_index.raw

    reusable_entries: Dict[str, SnapshotEntry] = {}
    invalid_entries: Dict[str, str] = {}
    for entry in remote_index.entries:
        reusable, reason = validate_local_snapshot(target_dir, entry)
        if reusable:
            reusable_entries[entry.file_name] = entry
        else:
            invalid_entries[entry.file_name] = reason

    if (
        index_is_identical
        and not invalid_entries
        and not extra_names
        and actual_names == expected_names
    ):
        log(
            "already up to date: {} snapshot(s), byte-identical index".format(
                len(remote_index.entries)
            )
        )
        return

    if not index_is_identical:
        log("remote index differs from the local index")
    for file_name, reason in invalid_entries.items():
        log("snapshot requires download: {} ({})".format(file_name, reason))
    if extra_names:
        log(
            "target contains {} unreferenced item(s); directory will be normalized".format(
                len(extra_names)
            )
        )

    stage_dir = Path(tempfile.mkdtemp(prefix="metahook-gamedata-", dir=str(temp_root)))
    reused_count = 0
    downloaded_count = 0
    try:
        for entry in remote_index.entries:
            staged_path = stage_dir / entry.file_name
            reusable_entry = reusable_entries.get(entry.file_name)
            if reusable_entry is not None and copy_reusable_snapshot(
                target_dir / reusable_entry.file_name,
                staged_path,
                entry,
            ):
                reused_count += 1
                continue

            snapshot_url = resolve_snapshot_url(index_url, entry.file_name)
            log(
                "downloading snapshot {} from {}".format(
                    entry.game_version,
                    snapshot_url,
                )
            )
            download_snapshot(snapshot_url, staged_path, entry)
            downloaded_count += 1

        (stage_dir / INDEX_FILE_NAME).write_bytes(remote_index.raw)
        validate_existing_directory(stage_dir)
        publish_stage(stage_dir, target_dir)
    finally:
        if stage_dir.exists():
            shutil.rmtree(str(stage_dir))

    log(
        "published {} snapshot(s) to {} (reused {}, downloaded {})".format(
            len(remote_index.entries),
            target_dir,
            reused_count,
            downloaded_count,
        )
    )


def main() -> int:
    try:
        require_python_version()
        arguments = parse_arguments()
        target_dir = validate_target_path(Path(arguments.target_dir))

        if arguments.validate_only:
            validate_existing_directory(target_dir)
            return 0

        if not arguments.temp_root:
            raise UpdateError("--temp-root is required unless --validate-only is used")
        temp_root = validate_temp_root(Path(arguments.temp_root), target_dir)
        index_url = normalize_index_url(arguments.index_url)
        with acquire_update_lock(target_dir):
            update_game_data(index_url, target_dir, temp_root)
        return 0
    except UpdateError as error:
        print(
            "[MetaHook gamedata] ERROR: {}".format(error),
            file=sys.stderr,
            flush=True,
        )
        return 1
    except Exception as error:
        print(
            "[MetaHook gamedata] ERROR: unexpected failure: {}".format(error),
            file=sys.stderr,
            flush=True,
        )
        return 1


if __name__ == "__main__":
    sys.exit(main())
