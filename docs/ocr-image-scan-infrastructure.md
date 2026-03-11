# OCR Image Scan Infrastructure

Status: Planned (not in v1.0)

This document outlines a future OCR pipeline that can ingest images, run OCR extraction, and merge textual signals into the profile and surface analysis. It is intentionally deferred to keep v1.0 focused on network workflows.

Proposed Flow
- ingest image paths or URLs
- normalize to grayscale and resize
- OCR extraction (tesseract or equivalent)
- entity extraction and confidence scoring
- merge into fusion graph

Security Note
OCR ingestion must validate file types and use bounded memory to avoid unsafe payloads.
