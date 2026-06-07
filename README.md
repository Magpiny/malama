# Malama

> Native Linux chat client for local LLMs — no cloud, no browser, no compromise.

[![Licence: Apache-2.0](https://img.shields.io/badge/licence-Apache--2.0-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](#)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](#)
[![Built with wxWidgets](https://img.shields.io/badge/UI-wxWidgets%203.3-green.svg)](#)

Malama is a fast, native desktop frontend for [Ollama](https://ollama.com), built with C++23 and wxWidgets 3.3. Think Open Web UI or LM Studio — but compiled to a native Linux binary with no Electron, no Node, no browser engine overhead.

The name blends **Mag**piny and Oll**ama** — the author's open-source identity and the runtime it wraps.

---

## Features

- Real-time streaming responses via Ollama's NDJSON API
- Switch between models (qwen2.5-coder, gemma, and any model Ollama serves) without restarting
- Full conversation history with per-session persistence
- Configurable Ollama endpoint (default: `http://127.0.0.1:11434`)
- Keyboard-first input — send with `Ctrl+Enter`, multi-line with `Enter`
- Zero telemetry, zero accounts — your prompts never leave your machine

---

## Architecture

```
malama::ui   →   malama::core (libmalama)   →   OllamaClient   →   Ollama API
wxWidgets        ChatEngine · ModelManager       libcurl              localhost
                 SettingsStore · ILLMBackend      NDJSON streaming
```

`libmalama` (the `malama::core` library) has **zero UI dependencies** and can be reused or tested independently of wxWidgets.

---

## Requirements

| Dependency   | Version  | Install                                    |
|--------------|----------|--------------------------------------------|
| CMake        | ≥ 4.2    | `sudo pacman -S cmake`                     |
| Clang / GCC  | C++23    | `sudo pacman -S clang`                     |
| wxWidgets    | 3.3      | `sudo pacman -S wxwidgets-gtk3`            |
| libcurl      | any      | `sudo pacman -S curl`                      |
| Ollama       | any      | [ollama.com/download](https://ollama.com/download) |

`spdlog` and `nlohmann_json` are fetched and pinned by CMake — no manual install needed.

---

## Build

```bash
git clone https://github.com/Magpiny/malama.git
cd malama
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/malama
```

Ollama must be running locally before launch:

```bash
ollama serve          # if not running as a systemd service
```

---

## Licence

Copyright © 2026 Magpiny (Wanjare). Licensed under the [Apache-2.0 Licence](LICENSE).

---
