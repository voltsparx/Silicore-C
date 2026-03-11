# Source Layout

The src directory mirrors the Silica-X layout while remaining C++20-native.

Key Modules
- orchestrator.cpp: high-level workflow coordination
- runner.cpp: CLI + prompt dispatch
- execution_policy.cpp: fast|balanced|deep|max tuning
- collect/: platform schema, platform registry, profile scanning, surface collection
- engines/: async HTTP engine, parallel scheduler, fusion engine, stabilizer
- extensions/: plugin loader and filter loader
- reporting/: JSON, HTML, and CLI rendering
- interface/: banner, help, explain, prompt, colors
- domain/: entity models
- foundation/: metadata and utilities

Design Notes
- All manifests are compiled into the binary.
- Plugins and filters remain dynamic to keep the core small and extensible.
- Output schema mirrors Silica-X for compatibility.
