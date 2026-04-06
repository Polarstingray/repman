#!/usr/bin/env python3
import json, os, sys, subprocess
from pathlib import Path

from textual.app        import App, ComposeResult
from textual.widgets    import DataTable, Input, RichLog, Footer, Header
from textual            import work
from textual.binding    import Binding


def _load_rows(index_path: str, installed_path: str,
               os_name: str, arch: str) -> list[dict]:
    os_arch = f"{os_name}_{arch}"
    try:
        index = json.loads(Path(index_path).read_text())
    except (FileNotFoundError, json.JSONDecodeError):
        index = {}
    try:
        installed = json.loads(Path(installed_path).read_text())
    except (FileNotFoundError, json.JSONDecodeError):
        installed = {}

    rows = []
    for name, pkg in index.items():
        latest = pkg.get("latest", "")
        ver_obj = pkg.get("versions", {}).get(latest, {})
        if os_arch not in ver_obj.get("targets", {}):
            continue
        inst_ver = installed.get(name, "")
        if not inst_ver:
            status = "not installed"
        elif inst_ver == latest:
            status = "installed"
        else:
            status = "outdated"
        rows.append({
            "name":      name,
            "installed": inst_ver or "—",
            "available": latest,
            "status":    status,
        })
    rows.sort(key=lambda r: r["name"])
    return rows


class RepmanTUI(App):
    TITLE = "repman"
    CSS = """
    Input     { dock: top; height: 3; }
    DataTable { height: 1fr; }
    RichLog   { height: 8; border-top: solid $primary; }
    """
    BINDINGS = [
        Binding("u",     "update_index",  "Update index"),
        Binding("i",     "install_pkg",   "Install"),
        Binding("U",     "uninstall_pkg", "Uninstall"),
        Binding("g",     "upgrade_all",   "Upgrade all"),
        Binding("r",     "refresh",       "Refresh"),
        Binding("slash", "focus_filter",  "Filter"),
        Binding("q",     "quit",          "Quit"),
    ]

    def __init__(self, os_name: str, arch: str,
                 index_path: str, installed_path: str):
        super().__init__()
        self.os_name        = os_name
        self.arch           = arch
        self.index_path     = index_path
        self.installed_path = installed_path
        self._rows: list[dict] = []

    def compose(self) -> ComposeResult:
        yield Header()
        yield Input(placeholder="Filter packages…", id="filter")
        yield DataTable(id="table", zebra_stripes=True)
        yield RichLog(id="log", highlight=True, markup=True)
        yield Footer()

    def on_mount(self) -> None:
        t = self.query_one(DataTable)
        t.add_columns("Package", "Installed", "Available", "Status")
        self.action_refresh()

    # ── filter ────────────────────────────────────────────────────────────────

    def on_input_changed(self, event: Input.Changed) -> None:
        self._apply_filter(event.value)

    def action_focus_filter(self) -> None:
        self.query_one(Input).focus()

    def _apply_filter(self, query: str) -> None:
        t = self.query_one(DataTable)
        t.clear()
        q = query.lower()
        for r in self._rows:
            if q in r["name"].lower():
                t.add_row(r["name"], r["installed"],
                          r["available"], r["status"], key=r["name"])

    # ── data refresh ──────────────────────────────────────────────────────────

    def action_refresh(self) -> None:
        self._rows = _load_rows(
            self.index_path, self.installed_path,
            self.os_name, self.arch,
        )
        self._apply_filter(self.query_one(Input).value)
        self._log(f"[green]Loaded {len(self._rows)} packages[/green]")

    # ── key actions ───────────────────────────────────────────────────────────

    def action_update_index(self)  -> None: self._run(["update"])
    def action_upgrade_all(self)   -> None: self._run(["upgrade"])

    def action_install_pkg(self) -> None:
        n = self._selected_name()
        if n:
            self._run(["install", n])

    def action_uninstall_pkg(self) -> None:
        n = self._selected_name()
        if n:
            self._run(["uninstall", n])

    def _selected_name(self) -> str | None:
        t = self.query_one(DataTable)
        if t.cursor_row < 0 or not self._rows:
            return None
        return t.get_row_at(t.cursor_row)[0]

    # ── background worker ─────────────────────────────────────────────────────

    @work(thread=True)
    def _run(self, cmd: list[str]) -> None:
        repcli = os.path.join(os.path.dirname(__file__), "repcli.py")
        proc = subprocess.Popen(
            [sys.executable, repcli, *cmd],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        for line in proc.stdout:
            self.call_from_thread(self._log, line.rstrip())
        proc.wait()
        color = "green" if proc.returncode == 0 else "red"
        label = "Done" if proc.returncode == 0 else "Failed"
        self.call_from_thread(
            self._log,
            f"[{color}]{label}: {' '.join(cmd)}[/{color}]",
        )
        self.call_from_thread(self.action_refresh)

    def _log(self, msg: str) -> None:
        self.query_one(RichLog).write(msg)
