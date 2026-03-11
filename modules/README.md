Silicore-C module catalog

This directory provides a lightweight registry for plugin and filter modules so the CLI and reporting layers can
enumerate capabilities in a consistent way. The catalog is backed by modules/catalog.h with convenience helpers in:

- plugin-modules.cpp: plugin inventory
- filter-modules.cpp: filter inventory
- catalog.cpp: combined view
- index.cpp: lookup helpers
