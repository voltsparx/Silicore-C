# Orchestration Architecture

Silicore-C uses a thin orchestrator that delegates to the profile and surface runners. The fusion workflow combines both and enriches with plugins and filters.

Flow Overview
- runner parses CLI or prompt inputs
- execution policy sets timeouts and concurrency
- profile workflow performs manifest probes via async engine
- surface workflow performs DNS/HTTP/robots/security/RDAP collection
- fusion workflow merges results and invokes plugins and filters
- reporting renders txt/json/html artifacts

Engines
- async_engine: libcurl multi with bounded bodies and response metadata
- parallel_engine: fan-out worker scheduling for independent tasks
- thread_engine: CPU-bound work isolation
- stabilizer_engine: backpressure and jitter smoothing for network workloads

Stability Guarantees
- bounded response bodies for memory safety
- configurable concurrency and timeouts
- error classification to keep reports deterministic
