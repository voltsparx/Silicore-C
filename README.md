# Silicore-C

Silicore-C is a C++20, Linux-first port of Silica-X. It keeps the same UX/UI (sky-blue theme), the same platform coverage, and compatible report keys while delivering a native binary with plugin and filter ABIs.

Highlights
- Profile, Surface, and Fusion workflows
- 70 embedded platform manifests compiled to C++
- Async HTTP engine (libcurl multi) with parallel scheduling and a stabilizer engine
- JSON, HTML, and CLI text outputs
- Plugin and filter ABI with dynamic loading
- Prompt mode plus flag-driven CLI

Quick Start (Linux)
- cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
- cmake --build build --config Release
- ./build/silicore-c help

Usage
- silicore-c profile <username> [flags]
- silicore-c surface <domain> [flags]
- silicore-c fusion <username> <domain> [flags]
- silicore-c show plugins | filters | platforms

Common Flags
- --preset fast|balanced|deep|max
- --timeout <ms>
- --concurrency <n>
- --proxy <url>
- --tor
- --txt --json --html
- --out <dir>
- --plugins a,b --all-plugins
- --filters a,b --all-filters

Prompt Mode
Run without arguments to enter the interactive prompt. It will ask for the workflow, output format (comma-separated), and output directory. Defaults are txt and the current working directory.

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
- src/: core runtime, engines, interface, reporting
- platforms/: compiled platform manifests
- plugins/: dynamic analysis plugins
- filters/: dynamic filter modules
- building-scripts/: OS install scripts
- docs/: architecture and release notes

Security and Ethics
Use Silicore-C only on targets you are authorized to test. Follow local laws and platform policies.
