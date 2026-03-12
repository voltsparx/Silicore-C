# Silicore-C

Silicore-C is a C++20, Linux-first port of Silica-X. It keeps the same UX/UI (sky-blue theme), the same platform coverage, and compatible report keys while delivering a native binary with plugin and filter ABIs.

Highlights
- Profile, Surface, and Fusion workflows
- 70 embedded platform manifests compiled to C++
- Async HTTP engine (libcurl multi) with parallel scheduling and a stabilizer engine
- JSON, HTML, and CLI text outputs
- Plugin and filter ABI with dynamic loading
- Prompt mode plus flag-driven CLI

Build Dependencies
- CMake 3.25+ and a C++20 compiler
- libcurl, OpenSSL, and nlohmann-json (vcpkg manifest in `vcpkg.json`)
- Set `VCPKG_ROOT` or place vcpkg under `external/vcpkg`

Quick Start (Windows)
- cmake --preset windows-msvc
- cmake --build --preset windows-msvc
- .\\build\\windows-msvc\\silicore-c.exe help

Quick Start (Linux)
- cmake --preset linux-release
- cmake --build --preset linux-release
- ./build/linux-release/silicore-c help

Usage
- silicore-c profile <username> [flags]
- silicore-c surface <domain> [flags]
- silicore-c fusion <username> <domain> [flags]
- silicore-c orchestrate <mode> <target> [flags]
- silicore-c wizard [flags]
- silicore-c quicktest [flags]
- silicore-c plugins | filters | modules | history | keywords
- silicore-c live <target> [--port]
- silicore-c anonymity [flags]
- silicore-c about | explain | prompt | help

Common Flags
- --preset safe|fast|quick|balanced|deep|aggressive|max
- --timeout <seconds>
- --concurrency <n>
- --proxy <url>
- --tor
- --txt --json --html
- --out <dir>
- --plugin a,b --all-plugins
- --filter a,b --all-filters

Prompt Mode
Run without arguments (or `silicore-c prompt`) to enter the interactive prompt. It will ask for the workflow, output format (comma-separated), and output directory. Defaults are txt and the current working directory.

Tor Routing
- --tor routes traffic via socks5h://127.0.0.1:9050.
- If Tor is running, Silicore-C uses it.
- If Tor is not running, Silicore-C can auto-install, configure, and start Tor with confirmation.

Outputs
Reports are written to the output directory using Silica-X compatible keys.
- txt: CLI summary
- json: structured report for downstream tooling
- html: minimal shareable report

Project Info
- Version: 1.0 (Crystal Lattice)
- Author: Voltsparx (voltsparx@gmail.com, voltsparx303@gmail.com)
- Repository: https://github.com/voltsparx/Silica-X

Layout
- include/: public and internal headers
- src/: core runtime, engines, interface, reporting
- src/platforms/: compiled platform manifests
- src/plugins/: dynamic analysis plugins
- src/filters/plugins/: dynamic filter modules
- resources/: parity scaffolding and intel snapshots
- building-scripts/: OS install scripts
- cmake/: CMake helper lists
- docs/: architecture, release notes, and source-layout.md
- tests/: unit tests

Security and Ethics
Use Silicore-C only on targets you are authorized to test. Follow local laws and platform policies.
