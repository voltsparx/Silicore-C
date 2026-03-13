# 📡Silicore-C v1.0

<strong>Release Theme: Crystal Lattice</strong><br>
Multi-engine OSINT orchestration for profile intelligence, domain-surface reconnaissance, and fused correlation reporting.


<p align="center">
  <img src="docs/image-icon/silicore-c-icon.png" alt="Silicore-C Logo" width="500px">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-v1.0-0A66C2?style=for-the-badge" alt="Version v1.0">
  <img src="https://img.shields.io/badge/theme-Crystal%20Lattice-1F7A8C?style=for-the-badge" alt="Theme Crystal Lattice">
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++20">
  <img src="https://img.shields.io/badge/tests-pending-8A6D3B?style=for-the-badge" alt="Tests Pending">
  <img src="https://img.shields.io/badge/platforms-70-4C956C?style=for-the-badge" alt="Platforms">
  <img src="https://img.shields.io/badge/license-Proprietary-8B0000?style=for-the-badge" alt="License Proprietary">
</p>

> **Power Summary:**
> Silicore-C is a native C++ OSINT framework designed for identity, surface, and fusion intelligence workflows. It uses multi-engine orchestration (async, threading, scheduler, and parallel execution) to gather, correlate, and score open-source signals with structured reporting. <br>
> Silicore-C is built to run intelligence workflows as one coordinated system:
> `profile`, `surface`, `fusion`, and `orchestrate`.
>  
> It combines policy-driven execution, plugin/filter control, and explainable scoring into one operator flow, with rich outputs across CLI, JSON, CSV, HTML, and logs. <br>

---

## 📊 Why Use Silicore-C?

* Unified OSINT workflows in one tool: `profile`, `surface`, `fusion`, and `orchestrate`
* Strong extension system with plugins + filters + scope compatibility checks
* Prompt mode and flag mode for both guided and automation-friendly usage
* Rich output lanes: CLI, JSON, CSV, HTML, and logs
* Built-in quicktest templates for fast smoke validation
* Native C++ runtime for lower overhead and faster cold starts

---

## 🗂 Quick Start

### Windows

```powershell
git clone https://github.com/voltsparx/Silicore-C.git
cd Silicore-C
cmake --preset windows-msvc
cmake --build --preset windows-msvc
.\build\windows-msvc\silicore-c.exe
```

### Linux

```bash
git clone https://github.com/voltsparx/Silicore-C.git
cd Silicore-C
cmake --preset linux-release
cmake --build --preset linux-release
./build/linux-release/silicore-c
```

In prompt mode, start here:

```text
help
show plugins
show filters
profile <username>
surface <domain>
fusion <username> <domain>
```

For full command reference, see [Usage Guide](docs/Usage.txt).

---

## ⚠️ Disclaimer

* Legal and authorized use only
* You are responsible for compliance with local laws and platform Terms of Service
* Do not use this framework for harassment, stalking, or unauthorized collection

---

## ✨ Highlights

* 🔎 Profile scan workflow (`profile`, `scan`, `persona`, `social`)
* 🌐 Domain surface workflow (`surface`, `domain`, `asset`)
* 🔗 Fusion workflow (`fusion`, `full`, `combo`)
* 🧩 Pluggable intelligence system (`src/extensions/signal_forge.cpp` + `src/plugins/`)
* 🧹 Pluggable filtering system (`src/extensions/signal_sieve.cpp` + `src/filters/`)
* 🧱 External module catalog system (`src/modules/catalog.cpp` + `src/modules/`)
* 🌌 Signal fusion connector layer (`src/collect/source_fusion.cpp` + `signal_*` plugin/filter pair)
* 🖥️ Prompt mode with keyword shortcuts, metasploit-style context prompt, and session defaults
* 📖 Explain system (`--explain`, `explain`) for command/plugin/filter onboarding
* 📊 HTML, JSON, CLI, CSV, and run-log outputs
* 🧅 Optional Tor/proxy routing with diagnostics and guided startup

---

## 📘 Documentation Index

* [Usage Guide](docs/Usage.txt)
* [Orchestration Architecture](docs/orchestration-architecture.md)
* [Capability Scan Report](docs/silica-capability-scan.md)
* [OCR/Image Infrastructure Plan](docs/ocr-image-scan-infrastructure.md)
* [Release Checklist v9.0 Lattice](docs/release-checklist-v9.0-lattice.md)
* [Release Notes v9.0 Lattice](docs/release-notes-v9.0-lattice.md)
* [Release Notes v9.2 Lattice](docs/release-notes-v9.2-lattice.md)
* [Release Commit Plan v9.0 Lattice](docs/release-commit-plan-v9.0-lattice.md)
* [Source Layout](docs/source-layout.md)
* [Code of Conduct](CODE_OF_CONDUCT.md)

---

## 🚀 v1.0 Engine Architecture Updates

* Standardized engine result contract: `name`, `status`, `data`, `error`, `execution_time`
* Engine health monitor metrics: active tasks, failure counters, and average latency
* Stabilizer engine adjusts concurrency based on runtime health
* Native async HTTP engine (libcurl multi) with adaptive batch concurrency
* Thread and CPU pools for blocking and CPU-heavy tasks
* Hybrid parallel engine for async + thread + CPU execution
* Fusion analytics engine with confidence scoring and graph output

---

## 🛠️ Engineering Upgrades Included

* Parser construction split into `src/interface/cli_parser.cpp`
* Shared prompt presets/keywords split into `src/interface/cli_config.cpp`
* Prompt command handlers centralized in `src/interface/prompt.cpp`
* Centralized about/explain renderer in `src/interface/about.cpp` + `src/interface/explain.cpp`
* Async engine (`src/engines/async_engine.cpp`) with concurrency caps and tuned curl options
* Thread engine (`src/engines/thread_engine.cpp`) with shared executor
* Parallel orchestration engine (`src/engines/parallel_engine.cpp`) for async + thread + CPU execution
* Fusion engine (`src/engines/fusion_engine.cpp`) with confidence scoring and anomaly flags
* Plugin/filter loaders in `src/extensions/` with compatibility checks
* Module catalog system (`src/modules/`)
* Reporting stack (`src/reporting/`) with HTML/JSON/CSV/TXT exporters
* Capability intel and parity scaffolding under `resources/`

---

## 📊 Verification Snapshot (pending)

* Build: not yet run in this environment
* Unit tests: not yet run (GTest + CTest)
* Static checks: not configured
* Platform manifests compiled: **70**
* Plugin sources: **21**
* Filter sources: **17**

---

## 🚀 Installation

```bash
git clone https://github.com/voltsparx/Silicore-C.git
cd Silicore-C
```

### Build dependencies (vcpkg)

```powershell
# Set VCPKG_ROOT or place vcpkg at external/vcpkg
setx VCPKG_ROOT C:\vcpkg
```

---

## ▶️ Run

```bash
silicore-c
```

Running without flags starts **prompt mode**.

---

## 📚 Documentation Tables

<table>
  <thead>
    <tr>
      <th>Mode</th>
      <th>Command</th>
      <th>Aliases</th>
      <th>Primary Purpose</th>
      <th>High-Value Flags</th>
    </tr>
  </thead>
  <tbody>
    <tr><td>Flag/Prompt</td><td><code>profile &lt;username...&gt;</code></td><td><code>scan</code>, <code>persona</code>, <code>social</code></td><td>Username/profile reconnaissance</td><td><code>--preset</code>, <code>--plugin</code>, <code>--filter</code>, <code>--html</code>, <code>--csv</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>surface &lt;domain&gt;</code></td><td><code>domain</code>, <code>asset</code></td><td>Domain surface exposure collection</td><td><code>--preset</code>, <code>--ct</code>, <code>--rdap</code>, <code>--plugin</code>, <code>--filter</code>, <code>--html</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>fusion &lt;username&gt; &lt;domain&gt;</code></td><td><code>full</code>, <code>combo</code></td><td>Combined profile + surface intelligence</td><td><code>--profile-preset</code>, <code>--surface-preset</code>, <code>--plugin</code>, <code>--filter</code>, <code>--html</code>, <code>--csv</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>orchestrate &lt;mode&gt; &lt;target&gt;</code></td><td><code>orch</code></td><td>Policy-driven orchestration pipeline</td><td><code>--profile</code>, <code>--source-profile</code>, <code>--min-confidence</code>, <code>--json</code>, <code>--html</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>quicktest</code></td><td><code>qtest</code>, <code>smoke</code></td><td>Offline synthetic victim test run with full artifact generation</td><td><code>--template</code>, <code>--seed</code>, <code>--list-templates</code>, <code>--json</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>plugins</code></td><td>-</td><td>List plugin inventory</td><td><code>--scope all|profile|surface|fusion</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>filters</code></td><td>-</td><td>List filter inventory</td><td><code>--scope all|profile|surface|fusion</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>modules</code></td><td>-</td><td>List/sync/query module catalog</td><td><code>--sync</code>, <code>--kind</code>, <code>--search</code>, <code>--tag</code>, <code>--stats-only</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>history</code></td><td><code>targets</code>, <code>scans</code></td><td>Show local scan history</td><td><code>--limit</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>anonymity</code></td><td>-</td><td>Inspect/configure routing state</td><td><code>--tor</code>, <code>--proxy</code>, <code>--check</code>, <code>--prompt</code></td></tr>
    <tr><td>Flag</td><td><code>live &lt;target&gt;</code></td><td>-</td><td>Launch local dashboard for a saved target</td><td><code>--port</code>, <code>--no-browser</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>wizard</code></td><td>-</td><td>Guided interactive workflow</td><td><code>--profile-preset</code>, <code>--surface-preset</code>, <code>--extension-control</code>, <code>--plugin</code>, <code>--filter</code>, <code>--tor</code>, <code>--proxy</code></td></tr>
    <tr><td>Flag/Prompt</td><td><code>keywords</code></td><td>-</td><td>Show keyword-to-command map</td><td>-</td></tr>
    <tr><td>Flag/Prompt</td><td><code>about</code>, <code>explain</code>, <code>help</code></td><td>-</td><td>Documentation and metadata</td><td><code>--about</code>, <code>--explain</code> (global)</td></tr>
  </tbody>
</table>

<br>

<table>
  <thead>
    <tr>
      <th>Prompt Control</th>
      <th>Example</th>
      <th>Behavior</th>
    </tr>
  </thead>
  <tbody>
    <tr><td>Module switch</td><td><code>use fusion</code> or <code>select module fusion</code></td><td>Changes active prompt context.</td></tr>
    <tr><td>Plugin set</td><td><code>set plugins threat_conductor,signal_fusion_core</code></td><td>Sets module-compatible plugins by id/alias/title.</td></tr>
    <tr><td>Filter set</td><td><code>set filters triage_priority_filter,link_hygiene_filter</code></td><td>Sets module-compatible filters by id/alias/title.</td></tr>
    <tr><td>Incremental plugin edits</td><td><code>add plugins x</code> / <code>remove plugins x</code></td><td>Adds/removes specific plugins while preserving compatibility checks.</td></tr>
    <tr><td>Incremental filter edits</td><td><code>add filters x</code> / <code>remove filters x</code></td><td>Adds/removes specific filters while preserving compatibility checks.</td></tr>
    <tr><td>Preset defaults</td><td><code>set profile_preset deep</code>, <code>set surface_preset quick</code></td><td>Updates prompt defaults for later commands.</td></tr>
    <tr><td>Extension control</td><td><code>set extension_control hybrid</code></td><td>Controls auto/manual/hybrid selection behavior.</td></tr>
    <tr><td>Quick smoke run</td><td><code>quicktest --seed 7</code></td><td>Runs synthetic end-to-end flow from prompt mode.</td></tr>
  </tbody>
</table>

<br>

<table>
  <thead>
    <tr>
      <th>Artifact</th>
      <th>Path Pattern</th>
      <th>Contains</th>
    </tr>
  </thead>
  <tbody>
    <tr><td>Primary JSON</td><td><code>output/data/&lt;target&gt;/results.json</code></td><td>Structured run payload (results, issues, plugins/filters, intelligence, summary).</td></tr>
    <tr><td>HTML Report</td><td><code>output/html/&lt;target&gt;.html</code></td><td>Visual dashboard report with tables/cards/correlation/guidance.</td></tr>
    <tr><td>CLI Report</td><td><code>output/cli/&lt;target&gt;.txt</code></td><td>Readable text report with scoring and extension summaries.</td></tr>
    <tr><td>CSV Main</td><td><code>output/cli/&lt;target&gt;.csv</code></td><td>Core flattened rows.</td></tr>
    <tr><td>CSV Companions</td><td><code>*.issues.csv</code>, <code>*.plugins.csv</code>, <code>*.filters.csv</code>, <code>*.intel-entities.csv</code>, <code>*.intel-contacts.csv</code></td><td>Detailed slices for downstream analysis.</td></tr>
    <tr><td>Run Logs</td><td><code>output/logs/&lt;target&gt;_&lt;timestamp&gt;.txt</code>, <code>output/logs/framework.log.txt</code></td><td>Per-run and framework lifecycle logs.</td></tr>
  </tbody>
</table>

<br>

<table>
  <thead>
    <tr>
      <th>Quicktest Template ID</th>
      <th>Victim Label</th>
      <th>Username</th>
      <th>Domain</th>
      <th>Default Selection</th>
    </tr>
  </thead>
  <tbody>
    <tr><td><code>atlas-mercier</code></td><td>Atlas Mercier</td><td><code>atlas_mercier</code></td><td><code>atlaslab.dev</code></td><td rowspan="5">Random when no <code>--template</code> is provided.</td></tr>
    <tr><td><code>noor-akhtar</code></td><td>Noor Akhtar</td><td><code>noor_akhtar</code></td><td><code>nordelta-ops.net</code></td></tr>
    <tr><td><code>juno-harbor</code></td><td>Juno Harbor</td><td><code>juno_harbor</code></td><td><code>harbor-grid.io</code></td></tr>
    <tr><td><code>raven-ion</code></td><td>Raven Ion</td><td><code>raven_ion</code></td><td><code>ionrelay.cloud</code></td></tr>
    <tr><td><code>maya-cipher</code></td><td>Maya Cipher</td><td><code>maya_cipher</code></td><td><code>ciphertrail.ai</code></td></tr>
  </tbody>
</table>

<br>

<table>
  <thead>
    <tr>
      <th>Smoke Suite (pending)</th>
      <th>Status</th>
      <th>Notes</th>
    </tr>
  </thead>
  <tbody>
    <tr><td><code>ctest --preset windows-msvc-tests</code></td><td>PENDING</td><td>Not yet executed in this environment.</td></tr>
    <tr><td><code>ctest --preset linux-release-tests</code></td><td>PENDING</td><td>Not yet executed in this environment.</td></tr>
    <tr><td>CLI matrix (about/explain/help/keywords/plugins/filters/modules/history)</td><td>PENDING</td><td>Run after toolchain setup.</td></tr>
    <tr><td>Command-path matrix (profile/surface/fusion/orchestrate via <code>--list-plugins/--list-filters</code>, plus <code>anonymity --check</code>)</td><td>PENDING</td><td>Run after toolchain setup.</td></tr>
    <tr><td>Quicktest matrix (<code>quicktest</code>, <code>qtest</code>, <code>smoke</code>, prompt quicktest)</td><td>PENDING</td><td>Run after toolchain setup.</td></tr>
    <tr><td><code>live</code> command</td><td>PENDING</td><td>Long-running server mode; verify manually.</td></tr>
  </tbody>
</table>

---

## 🧭 Core Commands

* `profile <username...> [flags]`
* `surface <domain> [flags]`
* `fusion <username> <domain> [flags]`
* `orchestrate <profile|surface|fusion> <target> [--secondary-target ...] [flags]`
* `plugins [--scope all|profile|surface|fusion]`
* `filters [--scope all|profile|surface|fusion]`
* `modules [--sync] [--kind all|plugin|filter] [--scope all|profile|surface|fusion]`
* `history [--limit N]` (aliases: `targets`, `scans`)
* `anonymity [--tor|--no-tor] [--proxy|--no-proxy] [--check] [--prompt]`
* `live <target> [--port PORT] [--no-browser]`
* `wizard [--profile-phase|--no-profile-phase] [--surface-phase|--no-surface-phase] [--fusion-phase|--no-fusion-phase] [--profile-preset ...] [--surface-preset ...] [--extension-control ...] [--plugin ...] [--filter ...] [--html|--no-html] [--csv|--no-csv] [--ct|--no-ct] [--rdap|--no-rdap] [--sync-modules]`
* `keywords`
* `about`
* `explain`
* `help`

---

## 🎛️ Key Flags

### Global

* `--about` → print framework description and exit
* `--explain` → print plain-language command/plugin/filter guide and exit
* `--about` and `--explain` must be used alone

### Runtime

* `--preset`, `--profile-preset`, `--surface-preset`
* `--timeout`, `--concurrency`, `--max-subdomains`
* `--max-workers`, `--source-profile`, `--max-platforms`, `--min-confidence`

### Output

* `--html`, `--csv`, `--json`, `--txt`, `--live`, `--live-port`, `--no-browser`

### Routing

* `--tor`, `--no-tor`, `--proxy`, `--no-proxy`, `--check`, `--prompt`

### Plugin / Filter

* `--plugin`, `--all-plugins`, `--list-plugins`
* `--filter`, `--all-filters`, `--list-filters`
* `--extension-control auto|manual|hybrid`

---

## 🧙 Wizard Workflow

`wizard` supports both fully guided operation and flag-seeded operation.

When you provide wizard flags, those values are used directly.  
When flags are omitted, wizard prompts for the missing decisions.

Wizard supports:

* Phase toggles: `--profile-phase|--no-profile-phase`, `--surface-phase|--no-surface-phase`, `--fusion-phase|--no-fusion-phase`
* Targets: `--usernames <a,b,c>`, `--domain <domain>`
* Runtime control: `--profile-preset`, `--surface-preset`, `--extension-control auto|manual|hybrid`
* Extension selection: `--plugin`, `--all-plugins`, `--list-plugins`, `--filter`, `--all-filters`, `--list-filters`
* Output toggles: `--html|--no-html`, `--csv|--no-csv`
* Surface toggles: `--ct|--no-ct`, `--rdap|--no-rdap`
* Catalog refresh: `--sync-modules`

Wizard includes extension compatibility preflight across selected scopes.  
If selectors conflict or are incompatible, wizard stops before scanning and prints recovery hints.

Example:

```bash
silicore-c wizard \
  --profile-phase --surface-phase --fusion-phase \
  --usernames alice,bob --domain example.com \
  --profile-preset deep --surface-preset balanced \
  --extension-control hybrid \
  --plugin threat_conductor --filter triage_priority_filter \
  --html --csv --ct --rdap
```

---

## 🔐 Crypto Plugin Operations

Crypto plugin set:

* `crypto_aes_attachment`
* `crypto_xor`
* `crypto_rot13`

Selection:

```bash
silicore-c profile alice --plugin crypto_aes_attachment --html
silicore-c fusion alice example.com --plugin crypto_xor --filter signal_lane_fusion --html
```

Runtime behavior:

* Crypto plugins are discoverable under `src/plugins/crypto/` and listed in a dedicated `Cryptography Plugin Set`.
* Reports include crypto configuration details and source coverage so selected behavior is transparent in CLI/HTML outputs.

---

## 🖼️ OCR/Image Infrastructure

OCR/image intelligence is documented as an architecture track and implementation guide (roadmap-level, not enabled as a built-in runtime plugin set yet).  
Reference docs:

* [OCR/Image Infrastructure Plan](docs/ocr-image-scan-infrastructure.md)

---

## 🖥️ Prompt Commands

* `scan <username>`
* `profile <username...>`
* `surface <domain>`
* `fusion <username> <domain>`
* `orchestrate <profile|surface|fusion> <target> [--secondary-target ...]`
* `plugins`, `filters`, `modules`, `history`
* `anonymity`, `config`
* `about` (keywords: `about`, `info`, `details`)
* `explain` (keywords: `explain`, `understand`, `describe`)
* `banner` (prompt-only; reprints banner)
* `use <profile|surface|fusion>`
* `select module <profile|surface|fusion>` (alias for `use`)
* `set plugins <none|all|selector1,selector2>` (module-compatible, id/alias/name-aware)
* `set filters <none|all|selector1,selector2>` (module-compatible, id/alias/name-aware)
* `select plugins <selector1,selector2>` / `select filters <selector1,selector2>` (name-based aliases)
* `add plugins <selector1,selector2>` / `remove plugins <selector1,selector2>` (incremental controls)
* `add filters <selector1,selector2>` / `remove filters <selector1,selector2>` (incremental controls)
* `set profile_preset <fast|quick|balanced|deep|max>`
* `set surface_preset <quick|balanced|deep>`
* `set extension_control <auto|manual|hybrid>`
* `help`, `clear`, `exit`

**Prompt format**

```
(console <module> ec=<mode> plugins=<set> filters=<set>)>>
```

---

## 🌍 Platform Coverage

Silicore-C ships with **70 platform manifests** under `src/platforms/`. Representative set:

Behance • Bitbucket • Blogger • BuyMeACoffee • Codeforces • CodePen • Dev.to • DeviantArt • Discord • DockerHub • Dribbble • Facebook • Flickr • GitHub • GitLab • HackerOne • HackerRank • Instagram • Kaggle • Keybase • LeetCode • LinkedIn • Mastodon • Medium • NPM • Pastebin • Patreon • Pinterest • ProductHunt • PyPI • Quora • Reddit • Replit • Roblox • Snapchat • SoundCloud • SourceForge • Spotify • StackOverflow • SteamCommunity • Telegram • Threads • TikTok • TryHackMe • Twitch • Twitter/X • Unsplash • Vimeo • WordPress • YouTube

---

## 📁 Output Structure

```
output/data/<target>/results.json
output/html/<target>.html
output/cli/<target>.txt
output/cli/<target>.csv (when --csv)
output/cli/<target>.issues.csv (when --csv)
output/cli/<target>.plugins.csv (when --csv)
output/cli/<target>.filters.csv (when --csv)
output/cli/<target>.intel-entities.csv (when --csv)
output/cli/<target>.intel-contacts.csv (when --csv)
output/logs/<target>_<timestamp>.txt
output/logs/framework.log.txt
```

---

## 🧪 Examples

```bash
silicore-c --about
silicore-c --explain
silicore-c anonymity --check
silicore-c plugins --scope all
silicore-c filters --scope all
silicore-c modules --sync --kind plugin --scope profile --limit 30
silicore-c profile alice --tor --plugin orbit_link_matrix --filter contact_canonicalizer --html
silicore-c surface example.com --plugin header_hardening_probe --filter exposure_tier_matrix --html
silicore-c fusion alice example.com --all-plugins --all-filters --html
silicore-c fusion alice example.com --plugin signal_fusion_core --filter signal_lane_fusion --html
silicore-c history --limit 20
```

---

## 🧪 Quality Gates

### Unit tests

```bash
ctest --preset windows-msvc-tests
ctest --preset linux-release-tests
```

---

**Author**: voltsparx<br>
**Contact**: voltsparx@gmail.com<br>

---

⭐ If you find Silicore-C useful, consider starring the repository!
