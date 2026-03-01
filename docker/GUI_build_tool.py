import os
import sys
import threading
import subprocess
import queue
import time
import re
import shutil
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

# Import your existing script as a module (DO NOT MODIFY IT)
# Assumes this GUI script lives in the same folder as build_tool.py
try:
    import build_tool as bt
except Exception as e:
    raise SystemExit(f"Failed to import build_tool.py: {e}")

# Hardcoded upstream for update checks (repo-specific GUI behavior).
UPSTREAM_REPO_URL = "https://github.com/openshwprojects/OpenBK7231T_App.git"
UPSTREAM_BRANCH = "main"



class OBKBuildToolGUI(tk.Tk):
    SOURCE_HINT_TEXT = "Hint: repo root containing docker/, src/, platforms/ (leave as-is if script is inside repo)."

    def _on_close(self):
        self._cleanup_pycache()
        self.destroy()

    def _cleanup_pycache(self):
        """Remove __pycache__ directories created by this GUI script."""
        try:
            base_dir = os.path.dirname(os.path.abspath(__file__))
            for root, dirs, files in os.walk(base_dir):
                if "__pycache__" in dirs:
                    try:
                        shutil.rmtree(os.path.join(root, "__pycache__"))
                    except Exception:
                        pass
        except Exception:
            pass

    def __init__(self):
        super().__init__()
        self.title("OBK Build Tool")
        self.protocol("WM_DELETE_WINDOW", self._on_close)

        self.minsize(950, 650)

        self._log_queue: "queue.Queue[str]" = queue.Queue()
        self._build_thread: threading.Thread | None = None
        self._busy = False

        # Caching for _validate_source_dir to avoid redundant Git calls
        self._last_validated_dir: str | None = None
        self._last_validation_res: tuple[bool, str] | None = None

        # Suppress terminal windows for background subprocesses on Windows
        self._creation_flags = 0x08000000 if sys.platform == "win32" else 0

        # Track build success for OTA gating
        self._last_build_success = False
        # Store driver variables {driver_name: BooleanVar}
        self._driver_vars: dict[str, tk.BooleanVar] = {}

        # Variables
        self.var_src_dir = tk.StringVar(value="")
        self.var_output_dir = tk.StringVar(value="")
        self.var_platform = tk.StringVar(value=bt.get_platforms()[0] if bt.get_platforms() else "")
        self.var_flash_size = tk.StringVar(value="")
        self.var_version = tk.StringVar(value="")
        self.var_clean = tk.BooleanVar(value=False)
        self.var_no_cache = tk.BooleanVar(value=False)
        self.var_txw_packager = tk.StringVar(value="auto")
        self.var_ota_host = tk.StringVar(value="")
        self.var_ota_auto = tk.BooleanVar(value=False)

        # Build UI
        self._build_ui()

        # Auto-refresh drivers when platform changes (covers both dropdown and programmatic changes)
        try:
            self.var_platform.trace_add("write", lambda *_: self._on_platform_change())
        except Exception:
            pass
        try:
            self.var_src_dir.trace_add("write", lambda *_: self._on_source_change())
        except Exception:
            pass
        try:
            self.var_output_dir.trace_add("write", lambda *_: self._on_output_change())
        except Exception:
            pass
        try:
            self.var_ota_host.trace_add("write", lambda *_: self._on_ota_settings_change())
        except Exception:
            pass
        try:
            self.var_ota_auto.trace_add("write", lambda *_: self._on_ota_settings_change())
        except Exception:
            pass

        # Initialize defaults based on platform
        self._on_platform_change()
        self._on_output_change()
        self._on_ota_settings_change()
        self._update_start_enabled()

        # Periodic log pumping
        self.after(50, self._pump_logs)

    def _check_main_clicked(self):
        ok, err = self._validate_source_dir()
        if not ok:
            messagebox.showerror("Invalid Source Directory", err)
            return
        threading.Thread(target=self._check_updates_worker, daemon=True).start()

    def _run_git(self, repo_dir: str, args: list[str], timeout_sec: int = 20) -> subprocess.CompletedProcess:
        return subprocess.run(
            ["git", "-C", repo_dir] + args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_sec,
            env=self._subprocess_env(),
            creationflags=self._creation_flags,
        )

    def _check_updates_worker(self):
        repo_dir = self.var_src_dir.get().strip() or bt.REPO_ROOT
        if not repo_dir or not os.path.isdir(repo_dir):
            return
        if not shutil.which("git"):
            return

        try:
            inside = self._run_git(repo_dir, ["rev-parse", "--is-inside-work-tree"])
            if inside.returncode != 0 or inside.stdout.strip().lower() != "true":
                self.after(
                    0,
                    lambda: messagebox.showwarning(
                        "Check Main",
                        "Selected source directory is not a Git working tree.",
                    ),
                )
                return

            fetch = self._run_git(
                repo_dir,
                ["fetch", "--quiet", UPSTREAM_REPO_URL, UPSTREAM_BRANCH],
                timeout_sec=60,
            )
            if fetch.returncode != 0:
                err = (fetch.stderr or "").strip() or "Unable to fetch upstream."
                self.after(0, lambda: messagebox.showwarning("Check Main", f"Fetch failed:\n{err}"))
                return

            upstream_head = self._run_git(repo_dir, ["rev-parse", "FETCH_HEAD"])
            if upstream_head.returncode != 0:
                return

            local_head = self._run_git(repo_dir, ["rev-parse", "HEAD"])
            if local_head.returncode != 0:
                return

            local_sha = local_head.stdout.strip()
            remote_sha = upstream_head.stdout.strip()
            if not local_sha or not remote_sha:
                return

            status = self._run_git(repo_dir, ["status", "--porcelain"])
            dirty = bool(status.stdout.strip()) if status.returncode == 0 else False

            counts = self._run_git(repo_dir, ["rev-list", "--left-right", "--count", "HEAD...FETCH_HEAD"])
            ahead = "?"
            behind = "?"
            if counts.returncode == 0:
                parts = counts.stdout.strip().split()
                if len(parts) == 2:
                    ahead, behind = parts[0], parts[1]

            current_branch = "?"
            branch_probe = self._run_git(repo_dir, ["rev-parse", "--abbrev-ref", "HEAD"])
            if branch_probe.returncode == 0:
                current_branch = branch_probe.stdout.strip() or "?"

            local_desc = local_sha[:10]
            remote_desc = remote_sha[:10]
            local_show = self._run_git(repo_dir, ["show", "-s", "--format=%h %ci %s", "HEAD"])
            if local_show.returncode == 0 and local_show.stdout.strip():
                local_desc = local_show.stdout.strip()
            remote_show = self._run_git(repo_dir, ["show", "-s", "--format=%h %ci %s", "FETCH_HEAD"])
            if remote_show.returncode == 0 and remote_show.stdout.strip():
                remote_desc = remote_show.stdout.strip()

            self.after(
                0,
                lambda: self._show_main_status(
                    repo_dir=repo_dir,
                    current_branch=current_branch,
                    local_desc=local_desc,
                    remote_desc=remote_desc,
                    ahead=ahead,
                    behind=behind,
                    dirty=dirty,
                ),
            )
        except Exception:
            self.after(
                0,
                lambda: messagebox.showwarning(
                    "Check Main",
                    "Main status check failed unexpectedly. See console log for details.",
                ),
            )

    def _show_main_status(
        self,
        repo_dir: str,
        current_branch: str,
        local_desc: str,
        remote_desc: str,
        ahead: str,
        behind: str,
        dirty: bool,
    ):
        dirty_text = "Yes" if dirty else "No"
        if ahead == "0" and behind == "0":
            sync_state = "Up to date with upstream main."
        else:
            sync_state = "Differs from upstream main."

        msg = [
            sync_state,
            "",
            f"Source: {UPSTREAM_REPO_URL}",
            f"Repo dir: {repo_dir}",
            f"Current branch: {current_branch}",
            "",
            f"Local HEAD:    {local_desc}",
            f"Upstream main: {remote_desc}",
            f"Ahead: {ahead}, Behind: {behind}",
            f"Working tree dirty: {dirty_text}",
        ]
        messagebox.showinfo("Main Status", "\n".join(msg))


    def _clear_log(self):
        # Clear visible log and any queued messages
        try:
            while True:
                _ = self._log_queue.get_nowait()
        except queue.Empty:
            pass
        self.txt_log.delete("1.0", "end")
        self._log("Ready.\n")

    def _copy_log_selection(self):
        try:
            text = self.txt_log.get("sel.first", "sel.last")
        except tk.TclError:
            text = ""
        if not text:
            return
        try:
            self.clipboard_clear()
            self.clipboard_append(text)
        except Exception:
            pass

    def _copy_log_all(self):
        text = self.txt_log.get("1.0", "end-1c")
        if not text:
            return
        try:
            self.clipboard_clear()
            self.clipboard_append(text)
        except Exception:
            pass

    def _show_log_menu(self, event):
        try:
            # Enable/disable Copy based on selection
            has_sel = True
            try:
                _ = self.txt_log.get("sel.first", "sel.last")
            except tk.TclError:
                has_sel = False

            self._log_menu.entryconfig("Copy", state=("normal" if has_sel else "disabled"))
            self._log_menu.tk_popup(event.x_root, event.y_root)
        finally:
            try:
                self._log_menu.grab_release()
            except Exception:
                pass


    # ---------------- UI ----------------
    def _build_ui(self):
        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)
        self.rowconfigure(3, weight=1)

        # --- Build Settings frame ---
        frm_settings = ttk.LabelFrame(self, text="Build Settings")
        frm_settings.grid(row=0, column=0, sticky="ew", padx=10, pady=10)
        frm_settings.columnconfigure(1, weight=1)

        # Source directory
        ttk.Label(frm_settings, text="Source Directory:").grid(row=0, column=0, sticky="w", padx=8, pady=(6, 2))
        ent_src = ttk.Entry(frm_settings, textvariable=self.var_src_dir)
        ent_src.grid(row=0, column=1, sticky="ew", padx=8, pady=(6, 2))
        btn_src = ttk.Button(frm_settings, text="...", width=4, command=self._browse_src)
        btn_src.grid(row=0, column=2, padx=8, pady=(6, 2))
        btn_check_main = ttk.Button(frm_settings, text="Check Main", command=self._check_main_clicked)
        btn_check_main.grid(row=0, column=3, padx=(0, 8), pady=(6, 2), sticky="w")
        self.btn_check_main = btn_check_main

        hint_src = ttk.Label(
            frm_settings,
            text=self.SOURCE_HINT_TEXT,
            foreground="#555",
            wraplength=760,
            justify="left",
        )
        hint_src.grid(row=1, column=1, columnspan=3, sticky="w", padx=8, pady=(0, 6))
        self.lbl_source_hint = hint_src
        # Output folder
        ttk.Label(frm_settings, text="Output Folder:").grid(row=2, column=0, sticky="w", padx=8, pady=(6, 2))
        ent_out = ttk.Entry(frm_settings, textvariable=self.var_output_dir)
        ent_out.grid(row=2, column=1, sticky="ew", padx=8, pady=(6, 2))
        btn_out = ttk.Button(frm_settings, text="...", width=4, command=self._browse_out)
        btn_out.grid(row=2, column=2, padx=8, pady=(6, 2))

        hint_out = ttk.Label(frm_settings, text="Hint: host folder where build artifacts will be written (/app/output in container).",
                             foreground="#555")
        hint_out.grid(row=3, column=1, columnspan=2, sticky="w", padx=8, pady=(0, 6))
        # Platform dropdown
        ttk.Label(frm_settings, text="Platform:").grid(row=4, column=0, sticky="w", padx=8, pady=(6, 2))
        self.cmb_platform = ttk.Combobox(frm_settings, textvariable=self.var_platform, state="readonly",
                                         values=bt.get_platforms())
        self.cmb_platform.grid(row=4, column=1, sticky="w", padx=8, pady=(6, 2))
        # Removed redundant bind: self.cmb_platform.bind("<<ComboboxSelected>>", ...) 
        # as self.var_platform.trace_add already handles this.
        # Flash size (editable combo)
        ttk.Label(frm_settings, text="Flash Size:").grid(row=4, column=2, sticky="w", padx=(8, 2), pady=(6, 2))
        self.cmb_flash = ttk.Combobox(frm_settings, textvariable=self.var_flash_size, width=12)
        self.cmb_flash["values"] = ["1MB", "2MB", "4MB", "8MB", "16MB"]
        self.cmb_flash.grid(row=4, column=2, sticky="e", padx=(90, 8), pady=(6, 2))
        hint_flash = ttk.Label(frm_settings, text="Hint: double check flash size for each platform before building.",
                               foreground="#555")
        hint_flash.grid(row=5, column=1, columnspan=2, sticky="w", padx=8, pady=(0, 6))

        # Version/name
        ttk.Label(frm_settings, text="Version / Name:").grid(row=6, column=0, sticky="w", padx=8, pady=(6, 2))
        ent_ver = ttk.Entry(frm_settings, textvariable=self.var_version)
        self.var_version.trace_add("write", lambda *_: self._update_start_enabled())
        ent_ver.grid(row=6, column=1, sticky="ew", padx=8, pady=(6, 2))
        self.ent_ver = ent_ver

        hint_ver = ttk.Label(frm_settings, text="Hint: avoid whitespace. Allowed: A–Z a–z 0–9 . _ - (whitespace will be converted to '_').",
                             foreground="#555")
        hint_ver.grid(row=7, column=1, columnspan=2, sticky="w", padx=8, pady=(0, 6))
        # Options row
        frm_opts = ttk.Frame(frm_settings)
        frm_opts.grid(row=8, column=0, columnspan=3, sticky="w", padx=8, pady=(2, 8))
        chk_clean = ttk.Checkbutton(frm_opts, text="Clean Build (make clean)", variable=self.var_clean)
        chk_clean.grid(row=0, column=0, sticky="w", padx=(0, 18))
        chk_nocache = ttk.Checkbutton(frm_opts, text="Rebuild Docker Image (no-cache)", variable=self.var_no_cache)
        chk_nocache.grid(row=0, column=1, sticky="w")
        self.chk_clean = chk_clean
        self.chk_nocache = chk_nocache
        ttk.Label(frm_opts, text="TXW packager:").grid(row=0, column=2, sticky="w", padx=(18, 6))
        self.cmb_txw = ttk.Combobox(
            frm_opts,
            textvariable=self.var_txw_packager,
            state="readonly",
            values=["auto", "wine", "fallback"],
            width=10,
        )
        self.cmb_txw.grid(row=0, column=3, sticky="w")
        # --- Drivers frame ---
        frm_drivers = ttk.LabelFrame(self, text="Drivers (platform-aware)")
        frm_drivers.grid(row=1, column=0, sticky="nsew", padx=10, pady=(0, 10))
        frm_drivers.columnconfigure(0, weight=1)
        frm_drivers.rowconfigure(0, weight=1)

        inner = ttk.Frame(frm_drivers)
        inner.grid(row=0, column=0, sticky="nsew", padx=8, pady=8)
        inner.columnconfigure(0, weight=1)
        inner.rowconfigure(0, weight=1)

        # Scrollable canvas for checkboxes
        self.can_drivers = tk.Canvas(inner, highlightthickness=0)
        self.can_drivers.grid(row=0, column=0, sticky="nsew")
        
        self.scr_drivers = ttk.Scrollbar(inner, orient="vertical", command=self.can_drivers.yview)
        self.scr_drivers.grid(row=0, column=1, sticky="ns")
        self.can_drivers.config(yscrollcommand=self.scr_drivers.set)

        self.frm_chk_drivers = ttk.Frame(self.can_drivers)
        self.can_drivers.create_window((0, 0), window=self.frm_chk_drivers, anchor="nw")

        def _on_frm_chk_configure(event):
            self.can_drivers.configure(scrollregion=self.can_drivers.bbox("all"))
        
        self.frm_chk_drivers.bind("<Configure>", _on_frm_chk_configure)

        btns = ttk.Frame(frm_drivers)
        btns.grid(row=0, column=1, sticky="ns", padx=8, pady=8)
        btn_refresh = ttk.Button(btns, text="Refresh", command=self._refresh_drivers)
        btn_refresh.grid(row=0, column=0, sticky="ew", pady=(0, 6))
        
        btn_sel_all = ttk.Button(btns, text="Select All", command=self._select_all_drivers)
        btn_sel_all.grid(row=1, column=0, sticky="ew", pady=(0, 6))
        
        btn_sel_none = ttk.Button(btns, text="Deselect All", command=self._select_none_drivers)
        btn_sel_none.grid(row=2, column=0, sticky="ew")

        self.btn_refresh = btn_refresh
        # --- Actions row ---
        frm_actions = ttk.Frame(self)
        frm_actions.grid(row=2, column=0, sticky="ew", padx=10)
        # Columns: Start | Clear | Open | OTA label | OTA host | OTA auto | OTA upload | spacer
        frm_actions.columnconfigure(7, weight=1)

        self.btn_start = ttk.Button(frm_actions, text="Start Build", command=self._start_build_clicked)
        self.btn_start.grid(row=0, column=0, sticky="w", padx=(0, 10), pady=8)

        btn_clear = ttk.Button(frm_actions, text="Clear Log", command=self._clear_log)
        btn_clear.grid(row=0, column=1, sticky="w", padx=(0, 10), pady=8)

        btn_open = ttk.Button(frm_actions, text="Open Folder", command=self._open_output_folder)
        btn_open.grid(row=0, column=2, sticky="w", padx=(0, 10), pady=8)
        self.btn_open = btn_open

        ttk.Label(frm_actions, text="OTA Host/IP:").grid(row=0, column=3, sticky="w", padx=(8, 6), pady=8)
        self.ent_ota_host = ttk.Entry(frm_actions, textvariable=self.var_ota_host, width=18)
        self.ent_ota_host.grid(row=0, column=4, sticky="w", padx=(0, 10), pady=8)

        self.chk_ota_auto = ttk.Checkbutton(frm_actions, text="Auto-upload OTA", variable=self.var_ota_auto)
        self.chk_ota_auto.grid(row=0, column=5, sticky="w", padx=(0, 10), pady=8)

        self.btn_ota_upload = ttk.Button(frm_actions, text="Upload OTA", command=self._upload_ota_clicked)
        self.btn_ota_upload.grid(row=0, column=6, sticky="w", padx=(0, 10), pady=8)
# --- Build output ---
        frm_log = ttk.LabelFrame(self, text="Build Output")
        frm_log.grid(row=3, column=0, sticky="nsew", padx=10, pady=10)
        frm_log.columnconfigure(0, weight=1)
        frm_log.rowconfigure(0, weight=1)

        self.txt_log = tk.Text(frm_log, wrap="word", height=16)
        self.txt_log.grid(row=0, column=0, sticky="nsew")
        scr2 = ttk.Scrollbar(frm_log, orient="vertical", command=self.txt_log.yview)
        scr2.grid(row=0, column=1, sticky="ns")
        self.txt_log.config(yscrollcommand=scr2.set)

        # Right-click context menu for log window (Copy / Copy All)
        self._log_menu = tk.Menu(self, tearoff=0)
        self._log_menu.add_command(label="Copy", command=self._copy_log_selection)
        self._log_menu.add_command(label="Copy All", command=self._copy_log_all)

        # Right click bindings (Windows/Linux/macOS variants)
        self.txt_log.bind("<Button-3>", self._show_log_menu)          # Windows/Linux
        self.txt_log.bind("<Control-Button-1>", self._show_log_menu)  # macOS
        self.txt_log.bind("<Button-2>", self._show_log_menu)          # some X11 configs


        self._log("Ready.\n")

    # ---------------- Helpers ----------------
    def _log(self, msg: str):
        self._log_queue.put(msg)

    def _subprocess_env(self) -> dict[str, str]:
        env = os.environ.copy()
        env.setdefault("PYTHONIOENCODING", "utf-8")
        env.setdefault("PYTHONUTF8", "1")
        return env

    def _pump_logs(self):
        try:
            while True:
                msg = self._log_queue.get_nowait()
                self.txt_log.insert("end", msg)
                self.txt_log.see("end")
        except queue.Empty:
            pass
        self.after(50, self._pump_logs)

    def _browse_src(self):
        d = filedialog.askdirectory(title="Select repository root (where build_tool.py expects docker/, src/, etc.)")
        if d:
            self.var_src_dir.set(d)
            self._on_source_change()
            ok, err = self._validate_source_dir()
            if not ok:
                messagebox.showwarning("Source Folder Warning", err)

    def _browse_out(self):
        d = filedialog.askdirectory(title="Select output folder (host)")
        if d:
            self.var_output_dir.set(d)
            self._on_output_change()

    def _open_output_folder(self):
        raw_out = self.var_output_dir.get().strip()
        if not raw_out:
            messagebox.showerror("Missing Output Folder", "Please select an output folder first.")
            return
        out_dir = os.path.abspath(raw_out)
        os.makedirs(out_dir, exist_ok=True)
        if sys.platform.startswith("win"):
            os.startfile(out_dir)  # noqa: E1101
        elif sys.platform == "darwin":
            subprocess.run(["open", out_dir], check=False)
        else:
            subprocess.run(["xdg-open", out_dir], check=False)

    def _on_platform_change(self):
        # Update bt paths based on src
        src_dir = self.var_src_dir.get().strip()
        bt.update_paths(src_dir if src_dir else None)

        self._last_build_success = False

        # Update default flash size for platform if empty
        plat = self.var_platform.get().strip()
        default_flash = bt.get_default_flash_size(plat) if plat else "2MB"
        current_flash = self.var_flash_size.get().strip()
        if not current_flash:
            self.var_flash_size.set(default_flash)

        # TXW packager is only relevant for TXW81X.
        if plat == "TXW81X":
            self.cmb_txw.config(state="readonly")
        else:
            self.var_txw_packager.set("auto")
            self.cmb_txw.config(state="disabled")

        # Refresh drivers list
        self._refresh_drivers()
        self._update_ota_controls()

    def _on_source_change(self):
        src_dir = self.var_src_dir.get().strip()
        bt.update_paths(src_dir if src_dir else None)
        self._last_build_success = False
        self._update_source_hint()
        self._apply_output_gate()
        self._refresh_drivers()
        self._update_ota_controls()
        self._update_start_enabled()

    def _on_output_change(self):
        self._apply_output_gate()
        self._update_ota_controls()
        self._update_start_enabled()

    def _on_ota_settings_change(self):
        self._update_ota_controls()

    def _validate_source_dir(self) -> tuple[bool, str]:
        src_dir = (self.var_src_dir.get().strip() or "").strip()

        # Return cached result if directory hasn't changed to avoid slow Git calls in EXE
        if src_dir == self._last_validated_dir and self._last_validation_res is not None:
            return self._last_validation_res

        self._last_validated_dir = src_dir

        if not src_dir:
            self._last_validation_res = (False, "Please select a source directory.")
            return self._last_validation_res

        if not os.path.isdir(src_dir):
            self._last_validation_res = (False, f"Source directory does not exist:\n{src_dir}")
            return self._last_validation_res

        required_entries = ["docker", "src", "platforms", "Makefile"]
        missing = []
        for entry in required_entries:
            p = os.path.join(src_dir, entry)
            if entry == "Makefile":
                if not os.path.isfile(p):
                    missing.append(entry)
            else:
                if not os.path.isdir(p):
                    missing.append(entry)
        if missing:
            self._last_validation_res = (False, (
                "Selected folder does not look like OpenBK repo root.\n"
                f"Missing: {', '.join(missing)}"
            ))
            return self._last_validation_res

        if shutil.which("git"):
            probe = self._run_git(src_dir, ["rev-parse", "--is-inside-work-tree"])
            if probe.returncode != 0 or probe.stdout.strip().lower() != "true":
                self._last_validation_res = (False, "Selected folder is not a Git working tree.")
            else:
                self._last_validation_res = (True, "")
        else:
            if not os.path.isdir(os.path.join(src_dir, ".git")):
                self._last_validation_res = (False, "Selected folder is not a Git working tree (.git missing).")
            else:
                self._last_validation_res = (True, "")

        return self._last_validation_res

    def _update_source_hint(self):
        if not hasattr(self, "lbl_source_hint"):
            return
        ok, err = self._validate_source_dir()
        if ok:
            self.lbl_source_hint.config(text=self.SOURCE_HINT_TEXT, foreground="#555")
            return

        # Keep warning short/clean for on-form display.
        msg = (err or "Selected source directory is invalid.").replace("\n", " ")
        self.lbl_source_hint.config(text=f"Warning: {msg}", foreground="#b00020")

    def _apply_output_gate(self):
        source_ok, _ = self._validate_source_dir()
        output_set = bool(self.var_output_dir.get().strip())
        enabled = output_set and source_ok and (not self._busy)

        self.cmb_platform.config(state=("readonly" if enabled else "disabled"))
        self.cmb_flash.config(state=("normal" if enabled else "disabled"))
        self.ent_ver.config(state=("normal" if enabled else "disabled"))
        self.chk_clean.config(state=("normal" if enabled else "disabled"))
        self.chk_nocache.config(state=("normal" if enabled else "disabled"))
        self.btn_refresh.config(state=("normal" if enabled else "disabled"))
        self.btn_open.config(state=("normal" if output_set else "disabled"))
        self.btn_check_main.config(state=("normal" if source_ok and (not self._busy) else "disabled"))
        if not enabled:
            self.cmb_txw.config(state="disabled")
        else:
            # Restore TXW control state based on active platform.
            self._on_platform_change()
        self._update_ota_controls()

    def _select_all_drivers(self):
        for var in self._driver_vars.values():
            var.set(True)
        self._update_start_enabled()

    def _select_none_drivers(self):
        for var in self._driver_vars.values():
            var.set(False)
        self._update_start_enabled()

    def _refresh_drivers(self):
        # Save previous selection
        prev_selection = self._get_selected_drivers()

        # Clear existing checkboxes
        for widget in self.frm_chk_drivers.winfo_children():
            widget.destroy()
        self._driver_vars.clear()

        source_ok, _ = self._validate_source_dir()
        if not source_ok:
            self._update_start_enabled()
            return

        if not self.var_output_dir.get().strip():
            self._update_start_enabled()
            return

        plat = self.var_platform.get().strip()
        if not plat:
            return

        drivers_dict = bt.get_available_drivers(plat)
        # Sort drivers alphabetically
        drivers = sorted(drivers_dict.keys())
        
        row = 0
        col = 0
        max_cols = 5 # Display drivers in multiple columns

        # If we have no previous selection (first load or platform switch),
        # use the defaults from obk_config.h
        use_defaults = (len(prev_selection) == 0)

        for d in drivers:
            # Strip prefixes for display label
            display_name = d
            if display_name.startswith("ENABLE_DRIVER_"):
                display_name = display_name[len("ENABLE_DRIVER_"):]
            elif display_name.startswith("ENABLE_"):
                display_name = display_name[len("ENABLE_"):]
            
            if use_defaults:
                # Use value from config (1=True, 0=False)
                # is_checked = (drivers_dict[d] != 0)
                is_checked = False
            else:
                # Maintain previous state
                is_checked = (d in prev_selection)

            var = tk.BooleanVar(value=is_checked)
            self._driver_vars[d] = var
            chk = ttk.Checkbutton(
                self.frm_chk_drivers, 
                text=display_name, 
                variable=var,
                command=self._update_start_enabled
            )
            chk.grid(row=row, column=col, sticky="w", padx=5, pady=2)
            col += 1
            if col >= max_cols:
                col = 0
                row += 1

        if not drivers:
            self._log(f"[Info] No drivers found (missing {bt.CONFIG_FILE}?)\n")
        
        self.can_drivers.xview_moveto(0)
        self.can_drivers.yview_moveto(0)
        self._update_start_enabled()

    def _update_start_enabled(self):
        # Start Build is enabled only when:
        # - at least one driver is selected
        # - Version / Name is not empty
        try:
            has_driver = len(self._get_selected_drivers()) > 0
        except Exception:
            has_driver = False

        version_ok = bool(self.var_version.get().strip())

        output_ok = bool(self.var_output_dir.get().strip())
        source_ok, _ = self._validate_source_dir()
        allow = has_driver and version_ok and output_ok and source_ok and (not self._busy)
        self.btn_start.config(state=("normal" if allow else "disabled"))

    def _get_selected_drivers(self) -> list[str]:
        return [name for name, var in self._driver_vars.items() if var.get()]

    def _normalize_flash(self, s: str) -> str:
        s = (s or "").strip()
        if s.isdigit():
            return f"{s}MB"
        return s

    def _is_valid_ipv4(self, ip_str: str) -> bool:
        """Checks if a string is a valid IPv4 address (4 parts, 0-255)."""
        parts = ip_str.split('.')
        if len(parts) != 4:
            return False
        for part in parts:
            if not part.isdigit():
                return False
            val = int(part)
            if val < 0 or val > 255:
                return False
        return True

    def _update_ota_controls(self):
        if not hasattr(self, "btn_ota_upload"):
            return
        source_ok, _ = self._validate_source_dir()
        output_ok = bool(self.var_output_dir.get().strip())
        platform = self.var_platform.get().strip()
        ota_capable = bool(platform) and bt.is_platform_ota_capable(platform)
        
        # OTA controls are enabled if platform is OTA capable (gating by build success reverted per user request)
        base_enabled = source_ok and output_ok and ota_capable and (not self._busy)

        self.ent_ota_host.config(state=("normal" if base_enabled else "disabled"))
        self.chk_ota_auto.config(state=("normal" if base_enabled else "disabled"))

        manual_enabled = (
            base_enabled
            and (not self.var_ota_auto.get())
            and bool(self.var_ota_host.get().strip())
        )
        self.btn_ota_upload.config(state=("normal" if manual_enabled else "disabled"))

    def _perform_ota_upload(self, out_dir: str, platform: str, host: str) -> bool:
        try:
            target = bt.get_target_platform(platform)
            ext = bt.get_ota_extension_for_platform(platform)
            if not ext:
                self._log(f"[OTA] Platform '{platform}' ({target}) is not marked OTA-capable.\n")
                return False

            ota_file = bt.find_ota_image_file(out_dir, platform)
            self._log(f"[OTA] Selected artifact: {ota_file}\n")

            info = bt.query_device_platform(host)
            token = (info.get("token") or "").strip()
            raw_platform = (info.get("raw_platform") or "").strip()
            source = info.get("source") or "none"
            
            if source == "none" or not token:
                self._log("[OTA] ERROR: Device platform probe failed. Check host/IP and connectivity.\n")
                return False

            compatible = bt.is_device_platform_compatible(platform, token)
            self._log(
                f"[OTA] Device platform ({source}): '{raw_platform}' -> '{token}', "
                f"target '{target}', compatible={compatible}\n"
            )
            if not compatible:
                self._log("[OTA] ERROR: Upload blocked due to platform mismatch.\n")
                return False

            result = bt.upload_ota_file(host, ota_file)
            if result.get("ok"):
                self._log(f"[OTA] Upload OK (HTTP {result.get('status', 0)}).\n")
                body = (result.get("body") or "").strip()
                if body:
                    self._log(f"[OTA] Response: {body}\n")
                restart = bt.restart_device(host)
                if restart.get("ok"):
                    self._log("[OTA] Restart command sent (Restart 1).\n")
                    rbody = (restart.get("body") or "").strip()
                    if rbody:
                        self._log(f"[OTA] Restart response: {rbody}\n")
                else:
                    self._log(f"[OTA] Restart FAILED (HTTP {restart.get('status', 0)}).\n")
                    rbody = (restart.get("body") or "").strip()
                    if rbody:
                        self._log(f"[OTA] Restart error: {rbody}\n")
                return True
            self._log(f"[OTA] Upload FAILED (HTTP {result.get('status', 0)}).\n")
            body = (result.get("body") or "").strip()
            if body:
                self._log(f"[OTA] Error: {body}\n")
            return False
        except Exception as e:
            self._log(f"[OTA] Upload failed: {e}\n")
            return False

    def _upload_ota_clicked(self):
        if self._busy:
            messagebox.showwarning("Busy", "Please wait for current operation to finish.")
            return
        source_ok, source_err = self._validate_source_dir()
        if not source_ok:
            messagebox.showerror("Invalid Source Directory", source_err)
            return
        out_dir = self.var_output_dir.get().strip()
        if not out_dir:
            messagebox.showerror("Missing Output Folder", "Please select an output folder first.")
            return
        platform = self.var_platform.get().strip()
        if not platform:
            messagebox.showerror("Missing Platform", "Please select a platform.")
            return
        if not bt.is_platform_ota_capable(platform):
            messagebox.showerror("OTA Not Supported", f"Platform '{platform}' is not marked OTA-capable.")
            return
        host = self.var_ota_host.get().strip()
        if not host:
            messagebox.showerror("Missing OTA Host", "Please provide device host/IP.")
            return
        
        if not self._is_valid_ipv4(host):
            messagebox.showerror("Invalid IP", f"'{host}' is not a valid IPv4 address (e.g., 192.168.0.100).")
            return

        self._set_ui_busy(True)
        self._log(f"[OTA] Manual upload requested for {platform} -> {host}\n")

        def _worker():
            try:
                self._perform_ota_upload(out_dir, platform, host)
            finally:
                self._set_ui_busy(False)

        threading.Thread(target=_worker, daemon=True).start()

    def _sanitize_version(self, s: str) -> str:
        """
        Downstream OBK build scripts are not robust to spaces/special chars in APP_VERSION.
        Keep it conservative: letters, digits, dot, dash, underscore.
        - Whitespace -> underscores
        - Other characters -> underscores
        """
        s = (s or "").strip()
        if not s:
            return ""
        s = re.sub(r"\s+", "_", s)
        s = re.sub(r"[^A-Za-z0-9._-]", "_", s)
        s = re.sub(r"_+", "_", s).strip("_")
        return s

    # ---------------- Build logic ----------------
    def _set_ui_busy(self, busy: bool):
        self._busy = busy
        self._apply_output_gate()
        self._update_start_enabled()

    def _start_build_clicked(self):
        if self._build_thread and self._build_thread.is_alive():
            messagebox.showwarning("Build Running", "A build is already running.")
            return

        # Validate minimal inputs
        src_dir = self.var_src_dir.get().strip()
        out_dir = self.var_output_dir.get().strip()
        plat = self.var_platform.get().strip()
        flash = self._normalize_flash(self.var_flash_size.get())

        if not out_dir:
            messagebox.showerror("Missing Output Folder", "Please select an output folder before starting the build.")
            return
        source_ok, source_err = self._validate_source_dir()
        if not source_ok:
            messagebox.showerror("Invalid Source Directory", source_err)
            return

        raw_version = self.var_version.get().strip()
        safe_version = self._sanitize_version(raw_version)
        if raw_version and raw_version != safe_version:
            self._log(f"[Warn] Version/Name sanitized: '{raw_version}' -> '{safe_version}'\n")
        version = safe_version or None

        if not plat:
            messagebox.showerror("Missing Platform", "Please select a platform.")
            return

        # Apply repo root override
        bt.update_paths(src_dir if src_dir else None)

        # sanity: docker dir exists
        if not os.path.isdir(bt.DOCKER_DIR):
            messagebox.showerror(
                "Invalid Source Directory",
                f"docker/ folder not found at:\n{bt.DOCKER_DIR}\n\n"
                "Point Source Directory to the repo root."
            )
            return

        selected_drivers = self._get_selected_drivers()
        if len(selected_drivers) == 0:
            messagebox.showerror("No Drivers Selected", "Please select at least one driver before starting the build.")
            self._update_start_enabled()
            return
        clean = bool(self.var_clean.get())
        no_cache = bool(self.var_no_cache.get())
        txw_packager = (self.var_txw_packager.get().strip() or "auto").lower()
        if txw_packager not in {"auto", "wine", "fallback"}:
            txw_packager = "auto"
        ota_auto = bool(self.var_ota_auto.get())
        ota_host = self.var_ota_host.get().strip()

        # Kick thread
        self._set_ui_busy(True)
        self.txt_log.delete("1.0", "end")
        self._log("--- OpenBK Build Tool (GUI) ---\n")
        self._log(f"Source: {bt.REPO_ROOT}\n")
        self._log(f"Output: {os.path.abspath(out_dir)}\n")
        self._log(f"Platform: {plat}\n")
        self._log(f"Flash Size: {flash}\n")
        self._log(f"Drivers: {len(selected_drivers)} selected\n")
        if version:
            self._log(f"Version: {version}\n")
        if clean:
            self._log("Option: CLEAN BUILD enabled\n")
        if no_cache:
            self._log("Option: DOCKER IMAGE REBUILD (no-cache) enabled\n")
        self._log(f"TXW Packager: {txw_packager}\n")
        self._log(f"OTA Host/IP: {ota_host if ota_host else '(not set)'}\n")
        self._log(f"OTA Auto-upload: {'enabled' if ota_auto else 'disabled'}\n")
        self._log("\n")

        self._build_thread = threading.Thread(
            target=self._run_build_worker,
            args=(out_dir, plat, selected_drivers, flash, version, clean, no_cache, txw_packager, ota_auto, ota_host),
            daemon=True
        )
        self._build_thread.start()

    def _run_cmd_stream(self, cmd: list[str], cwd: str | None = None):
        """Run a command and stream stdout/stderr into the GUI log."""
        self._log(f"$ {' '.join(cmd)}\n")
        p = subprocess.Popen(
            cmd,
            cwd=cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=self._subprocess_env(),
            bufsize=1,
            creationflags=self._creation_flags,
        )
        assert p.stdout is not None
        for line in p.stdout:
            self._log(line)
        rc = p.wait()
        if rc != 0:
            raise RuntimeError(f"Command failed with exit code {rc}: {' '.join(cmd)}")

    def _docker_image_exists(self, image_name: str) -> bool:
        try:
            probe = subprocess.run(
                ["docker", "image", "inspect", image_name],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
                env=self._subprocess_env(),
                creationflags=self._creation_flags,
            )
            return probe.returncode == 0
        except Exception:
            return False

    def _run_build_worker(self, out_dir: str, platform: str, selected_drivers: list[str],
                          flash_size: str, version: str | None, clean: bool, no_cache: bool,
                          txw_packager: str, ota_auto: bool, ota_host: str):
        start_time = time.time()
        temp_config_path = os.path.join(bt.REPO_ROOT, "temp_obk_config.h")
        try:
            # Docker daemon check (reuse your logic)
            if not bt.check_docker_running(creationflags=self._creation_flags):
                self._log("\n❌ ERROR: Docker is not running!\nPlease start Docker Desktop and try again.\n")
                return

            # Generate temp config using your existing function
            cfg_text = bt.create_custom_config(selected_drivers)
            with open(temp_config_path, "w", encoding="utf-8") as f:
                f.write(cfg_text)

            # 1) Ensure required GCC archive is present (auto-download if missing)
            if hasattr(bt, "ensure_gcc_arm_archive"):
                self._log("Checking required GCC ARM archive...\n")
                bt.ensure_gcc_arm_archive()

            # 2) docker build image (reuse existing image unless forced)
            image_name = "openbk_builder"
            needs_build = no_cache or (not self._docker_image_exists(image_name))
            if needs_build:
                if no_cache:
                    self._log("Building Docker image with --no-cache (forced rebuild)...\n")
                else:
                    self._log("Docker image not found; building openbk_builder...\n")
                build_cmd = ["docker", "build", "-t", image_name, "."]
                if no_cache:
                    build_cmd.insert(2, "--no-cache")
                self._run_cmd_stream(build_cmd, cwd=bt.DOCKER_DIR)
            else:
                self._log("Using existing Docker image 'openbk_builder' (skip rebuild).\n")

            # 3) docker run build
            # Resolve target platform
            target_platform = bt.PLATFORMS.get(platform, {}).get("target", platform)

            abs_output_dir = os.path.abspath(out_dir)
            os.makedirs(abs_output_dir, exist_ok=True)

            cmd = [
                "docker", "run", "--rm",
                "-v", f"{bt.REPO_ROOT}:/app/source:ro",
                "-v", f"{abs_output_dir}:/app/output",
                "-v", f"{temp_config_path}:/app/custom_config.h:ro",
                "-v", f"{bt.DEFAULT_BUILD_VOLUME}:/app/build",
                "-v", f"{bt.DEFAULT_RTK_TOOLCHAIN_VOLUME}:/opt/rtk-toolchain",
                "-v", f"{bt.DEFAULT_ESPRESSIF_TOOLS_VOLUME}:/app/espressif-cache",
                "-v", f"{bt.DEFAULT_ESP8266_TOOLS_VOLUME}:/app/esp8266-cache",
                "-v", f"{bt.DEFAULT_MBEDTLS_CACHE_VOLUME}:/app/mbedtls-cache",
                "-v", f"{bt.DEFAULT_CSKY_W800_CACHE_VOLUME}:/app/csky-w800-cache",
                "-v", f"{bt.DEFAULT_CSKY_TXW_CACHE_VOLUME}:/app/csky-txw-cache",
                "-v", f"{bt.DEFAULT_PIP_CACHE_VOLUME}:/app/pip-cache",
            ]

            if version:
                cmd += ["-e", f"APP_VERSION={version}"]
            if clean:
                cmd += ["-e", "CLEAN_BUILD=1"]
            if flash_size:
                cmd += ["-e", f"FLASH_SIZE={flash_size}"]
            if txw_packager:
                cmd += ["-e", f"TXW_PACKAGER_MODE={txw_packager}"]

            # Pass timezone similar to your script
            try:
                is_dst = time.daylight and time.localtime().tm_isdst > 0
                local_tz = time.tzname[1] if is_dst else time.tzname[0]
                if local_tz:
                    cmd += ["-e", f"TZ={local_tz}"]
            except Exception:
                pass

            cmd += ["openbk_builder", target_platform]

            self._run_cmd_stream(cmd, cwd=None)

            # verify output file presence
            try:
                bt.find_ota_image_file(out_dir, platform)
                self._last_build_success = True
            except Exception as ve:
                self._log(f"\n[Warn] Build finished but OTA artifact not found: {ve}\n")
                self._last_build_success = False

            elapsed = time.time() - start_time
            self._log(f"\n[OK] Build completed in {elapsed:.2f} seconds.\n")

            if ota_auto and self._last_build_success:
                if not bt.is_platform_ota_capable(platform):
                    self._log(f"[OTA] Auto-upload skipped: platform '{platform}' is not marked OTA-capable.\n")
                elif not ota_host:
                    self._log("[OTA] Auto-upload skipped: OTA host/IP is empty.\n")
                elif not self._is_valid_ipv4(ota_host):
                    self._log(f"[OTA] Auto-upload aborted: invalid IP address '{ota_host}'.\n")
                else:
                    self._log(f"[OTA] Auto-upload enabled. Uploading to {ota_host}...\n")
                    self._perform_ota_upload(out_dir, platform, ota_host)

        except Exception as e:
            self._last_build_success = False
            self._log(f"\n[ERR] Build failed: {e}\n")
        finally:
            # Cleanup temp config
            try:
                if os.path.exists(temp_config_path):
                    pass
                    #os.remove(temp_config_path)
            except Exception:
                pass

            self._set_ui_busy(False)


if __name__ == "__main__":
    app = OBKBuildToolGUI()
    app.mainloop()
