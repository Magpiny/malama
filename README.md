<div align="center">

<img src="https://raw.githubusercontent.com/Magpiny/malama/main/assets/malama.png" alt="Malama logo" width="120">

# Malama

**A lightweight, private, and fully native Linux desktop client for chatting with large language models running locally through Ollama.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C.svg?logo=cplusplus&logoColor=white)](#build-requirements)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-FCC624.svg?logo=linux&logoColor=black)](#build-requirements)
[![wxWidgets](https://img.shields.io/badge/UI-wxWidgets%203.3-2F6DB2.svg)](https://www.wxwidgets.org/)
[![Ollama](https://img.shields.io/badge/backend-Ollama-white.svg?logo=ollama&logoColor=black)](https://ollama.com/)
[![Version](https://img.shields.io/badge/version-0.3.1-0A7EA4.svg)](https://github.com/Magpiny/malama/releases)

[![CI](https://github.com/Magpiny/malama/actions/workflows/ci_cd.yml/badge.svg)](https://github.com/Magpiny/malama/actions/workflows/ci_cd.yml)
[![Latest Release](https://img.shields.io/github/v/release/Magpiny/malama?display_name=tag&sort=semver)](https://github.com/Magpiny/malama/releases/latest)

**Private by default. Native by design.**

</div>

---

<div align="center">
  <img src="https://raw.githubusercontent.com/Magpiny/malama/main/assets/malama_ui.png" alt="Malama v0.3.1 user interface" width="100%">
</div>

---

## Table of Contents

- [Overview](#overview)
- [Why Malama?](#why-malama)
- [What's New in v0.3.1](#whats-new-in-v0.3.1)
  - [Multimodal Asset Ingestion](#multimodal-asset-ingestion)
  - [Context Isolation Boundaries](#context-isolation-boundaries)
  - [Extended Ollama Context Allocation](#extended-ollama-context-allocation)
  - [Bounded File Processing](#bounded-file-processing)
  - [Attachment Queue Management](#attachment-queue-management)
  - [Accurate Sidebar Hit Testing](#accurate-sidebar-hit-testing)
- [Key Features](#key-features)
- [Supported Attachments](#supported-attachments)
- [Architecture](#architecture)
  - [Ingestion Subsystem](#ingestion-subsystem)
  - [Network Payload Serialization](#network-payload-serialization)
  - [Memory and Style Policy](#memory-and-style-policy)
- [Build Requirements](#build-requirements)
- [Setup Guide](#setup-guide)
  - [1. Install Ollama](#1-install-ollama)
  - [2. Pull a Model](#2-pull-a-model)
  - [3. Install Build Dependencies](#3-install-build-dependencies)
  - [4. Clone and Build Malama](#4-clone-and-build-malama)
  - [5. Run Malama](#5-run-malama)
- [Automated Testing](#automated-testing)
- [CI/CD Pipeline](#cicd-pipeline)
- [Usage](#usage)
- [Privacy](#privacy)
- [License](#license)

---

## Overview

**Malama** is a lightweight, native Linux desktop chat client built with **C++23** and **wxWidgets**.

It connects directly to a local [Ollama](https://ollama.com/) instance over TCP and provides a complete LLM chat workspace without Electron, an embedded browser engine, or a required external cloud service.

Malama idles at approximately **40 MB of RAM**, leaving more system resources available for model inference.

> [!IMPORTANT]
> Malama is designed for local inference. Conversations and attached documents remain on your machine unless your Ollama configuration or selected model explicitly communicates with an external service.

---

## Why Malama?

Many desktop AI clients are browser applications packaged as desktop programs. Malama takes a different approach:

- **Native Linux interface** built with wxWidgets
- **Local-first operation** through Ollama
- **Low memory usage** without an Electron runtime
- **Persistent conversations** stored with SQLite
- **Real-time token streaming** over asynchronous TCP
- **Document and image attachments**
- **Modern C++23 architecture** with explicit ownership and error handling
- **No browser or cloud account required**

---

## What's New in v0.3.1

### memory
    - Switch from single-prompt requests to full conversation replay so the model sees prior turns.
    - The model can now reference previous conversation in a session and use the history(memory/brain) as context
       to provide more accurate answers
### website
   The App has a new beautiful home 

### Multimodal Asset Ingestion (v0.2.7)

Malama now includes a factory-routed attachment manager capable of processing images, documents, spreadsheets, e-books, and plain-text files.

```text
Images:       .png, .jpg, .jpeg, .webp
Documents:    .pdf, .docx, .odt
Spreadsheets: .xlsx, .ods
E-books:      .epub
Text:         .txt, .md
```

Files can be attached directly to the active conversation before a prompt is submitted.

### Context Isolation Boundaries

Extracted document content is enclosed within explicit context markers:

```text
[START OF CONTEXT DOCUMENT: filename]

Extracted document content

[END OF CONTEXT DOCUMENT: filename]
```

These boundaries help the model distinguish user instructions from attached reference material.

### Extended Ollama Context Allocation

To reduce the likelihood of long documents being silently truncated, Malama's asynchronous request pipeline explicitly sets:

```json
{
  "num_ctx": 32000
}
```

> [!NOTE]
> Effective context capacity still depends on the selected model and the memory available on your system.

### Bounded File Processing

Individual image and PDF attachments are subject to a **4 MB processing limit**. This boundary helps protect local systems from excessive RAM or VRAM use, unexpected allocation failures, and oversized document payloads.

### Attachment Queue Management

Attached files appear in a horizontal staging tray beneath the prompt editor. Each attachment is represented by a compact chip containing:

- A file-type icon
- The attachment name
- An independent remove button
- Its current staging state

This makes it possible to review and remove individual files before sending a prompt.

### Accurate Sidebar Hit Testing

Sidebar interaction uses localized `HitTest` checks through `wxEVT_MOTION`. Hand cursors and navigation tooltips are displayed only when the pointer is over a valid conversation entry, avoiding inaccurate interactions caused by broad widget-level bounding boxes.

---

## Key Features

| Feature | Description |
| --- | --- |
| 🪶 **Minimal Footprint** | Idles at approximately 40 MB of RAM, leaving resources available for model inference. |
| 🔒 **Local-First Operation** | Connects directly to a local Ollama instance without requiring a cloud account or browser session. |
| 💾 **Persistent Sessions** | Stores conversation history locally with SQLite3 and supports pinning, renaming, and deleting sessions. |
| ⚡ **Low-Latency Streaming** | Uses Boost.Asio for asynchronous TCP communication and real-time token delivery. |
| 🖼️ **Multimodal Routing** | Validates supported images and prepares Base64 payloads for compatible vision-language models. |
| 📄 **Document Extraction** | Extracts text from documents, spreadsheets, e-books, markup files, and plain-text assets. |
| ❌ **Selective Queue Control** | Allows individual attachments to be removed before a request is submitted. |
| 📌 **Precise Sidebar Interaction** | Uses coordinate-aware hit testing for reliable conversation selection and hover behavior. |
| 📝 **Native Markdown Rendering** | Includes a native Markdown canvas and JSON-configurable syntax highlighting for C++, Rust, Python, and other languages. |
| 🧠 **Reasoning Filter** | Optionally removes `<think>` blocks produced by reasoning-oriented models through a thread-isolated state machine. |
| 🧩 **Decoupled Architecture** | Separates interface widgets, networking workers, storage repositories, and file parsers into focused components. |

---

## Supported Attachments

| Category | Formats | Processing Strategy |
| --- | --- | --- |
| Images | PNG, JPEG, JPG, WebP | Validation and Base64 encoding for multimodal models |
| PDF documents | PDF | Text extraction through `poppler-cpp` |
| Office documents | DOCX, XLSX | Archive extraction and XML traversal |
| OpenDocument files | ODT, ODS | Archive extraction and XML traversal |
| E-books | EPUB | Archive extraction and structured text traversal |
| Plain text | TXT, MD | Direct UTF-8 text ingestion |

> [!WARNING]
> Image attachments require a model with vision capabilities. A text-only model cannot interpret image payloads.

---

## Architecture

Malama separates the user interface, attachment processing, asynchronous networking, persistent storage, and local model runtime.

```text
┌──────────────────────────────────────────────────────────┐
│                   UI Thread / Main Frame                 │
│                                                          │
│   ┌──────────────────────┐   ┌────────────────────────┐  │
│   │   ChatPanel Canvas   │   │   SidebarPanel List    │  │
│   └──────────┬───────────┘   └────────────▲───────────┘  │
└──────────────┼─────────────────────────────┼───────────────┘
               │                             │
        on_send_action()                wxQueueEvent
               │                        token updates
               ▼                             │
┌──────────────────────────┐    ┌────────────┴─────────────┐
│    AttachmentManager     │    │       StreamWorker       │
│                          │    │                          │
│  Factory-routed parsers  │    │  Boost.Asio TCP socket  │
│  libarchive + pugixml    │    │  Glaze JSON processing  │
└─────────────┬────────────┘    └────────────┬─────────────┘
              │                              │
              └───────────┬──────────────────┘
                          │
                   POST /api/chat
                   num_ctx: 32000
                          │
                          ▼
              ┌────────────────────────┐
              │ Local Ollama Instance  │
              └────────────────────────┘
```

### Ingestion Subsystem

The `AttachmentManager` acts as the central entry point for file ingestion. When a file enters the application, its extension is classified and routed to the appropriate parser through `ParserFactory`.

#### `PlainTextParser`

Reads UTF-8 text files directly into the reference context:

```text
.txt
.md
```

#### `PdfParser`

Uses `poppler-cpp` to read text from PDF pages while excluding embedded binary content from the resulting prompt context.

#### `XmlParser` and `ArchiveReader`

Processes ZIP-based document formats:

```text
.docx
.xlsx
.odt
.ods
.epub
```

`ArchiveReader` uses `libarchive` to inspect and extract archive entries. `XmlParser` then traverses relevant XML nodes using `pugixml`.

The extraction process includes boundary checks intended to reduce the risk of malformed archives and archive-expansion attacks.

#### `ImageParser`

Uses Boost.GIL to inspect image metadata and dimensions. Format-specific validation is performed through `libpng` and `libjpeg` before supported assets are prepared for multimodal requests.

### Network Payload Serialization

Extracted text is cached to avoid repeating expensive parsing work. When a request is submitted:

1. Staged attachments are validated.
2. Text is extracted from supported documents.
3. Extracted content is wrapped in context-isolation markers.
4. Images are encoded into Base64 payloads.
5. The prompt, attachments, and model options are serialized into JSON.
6. `num_ctx` is set to `32000`.
7. The request is streamed to Ollama over a Boost.Asio TCP connection.
8. Generated tokens are returned to the UI using queued, thread-safe events.

### Memory and Style Policy

Malama follows a modern C++ ownership and error-handling policy:

- C++23 language features
- `std::expected` for explicit error propagation
- No raw owning pointers
- RAII-managed resources
- `std::unique_ptr` for exclusive dynamic ownership
- Non-throwing allocation paths where appropriate
- Explicit comparisons such as `== false` and `== nullptr`
- Separation of UI, storage, parsing, and networking responsibilities

---

## Build Requirements

Malama currently targets **native Linux distributions**.

### Compiler

Use one of the following:

- **GCC 14 or newer**
- **Clang 16 or newer**

The compiler must support the C++23 features used by the project.

### Build System

- **CMake 3.28 or newer**
- **Ninja** (recommended)

### Core Libraries

| Library | Version Requirement | Purpose |
| --- | --- | --- |
| [wxWidgets](https://www.wxwidgets.org/) | ≥ 3.3 | Native GTK-backed interface toolkit |
| [SQLite3](https://sqlite.org/) | Stable release | Local conversation-history storage |
| [Boost.Asio](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html) | ≥ 1.74 | Asynchronous socket I/O |
| [Boost.GIL](https://www.boost.org/doc/libs/release/libs/gil/) | ≥ 1.74 | Image metadata and layout validation |
| [libarchive](https://libarchive.org/) | ≥ 3.6 | Archive inspection and decompression |
| [pugixml](https://pugixml.org/) | ≥ 1.12 | Lightweight XML traversal |
| [libpng](http://www.libpng.org/pub/png/libpng.html) | ≥ 1.6 | PNG binary validation |
| [libjpeg-turbo](https://libjpeg-turbo.org/) | Stable release | JPEG/JPG binary validation |
| [poppler-cpp](https://poppler.freedesktop.org/) | Stable release | PDF text extraction |
| [Glaze](https://github.com/stephenberry/glaze) | ≥ 2.0 | High-performance JSON serialization and parsing |
| [spdlog](https://github.com/gabime/spdlog) | ≥ 1.12 | Application logging |
| [Catch2](https://github.com/catchorg/Catch2) | v3 | Automated unit, stress, and benchmark tests |

---

## Setup Guide

### 1. Install Ollama

Install and start Ollama using the appropriate method for your distribution.

#### Arch Linux / CachyOS

```bash
sudo pacman -S ollama
sudo systemctl enable --now ollama
```

#### Debian / Ubuntu

```bash
sudo apt install ollama
```

If Ollama is unavailable in your configured repositories, use the installation method documented by Ollama.

#### Fedora / RHEL

```bash
curl -fsSL https://ollama.com/install.sh | sh
```

Verify the installation:

```bash
ollama --version
```

If the Ollama service is not already running, start it with:

```bash
ollama serve
```

### 2. Pull a Model

#### Text and Code Model

```bash
ollama pull qwen2.5-coder:7b
```

#### Multimodal Model

```bash
ollama pull qwen2-vl:7b
```

List locally installed models:

```bash
ollama list
```

> [!TIP]
> Model names and availability may vary between Ollama releases. Choose a model that fits within your available system RAM or GPU VRAM.

### 3. Install Build Dependencies

#### Arch Linux / CachyOS

```bash
sudo pacman -S --needed \
  git cmake ninja pkgconf gcc \
  wxwidgets-gtk3 sqlite poppler-cpp \
  libarchive pugixml libpng libjpeg-turbo \
  spdlog boost boost-libs
```

#### Debian / Ubuntu / Linux Mint

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config \
  libwxgtk3.2-dev sqlite3 libsqlite3-dev \
  libpoppler-cpp-dev libarchive-dev libpugixml-dev \
  libpng-dev libjpeg-dev libspdlog-dev \
  libboost-all-dev libboost-context-dev libboost-system-dev
```

> [!NOTE]
> Package names can differ between distribution releases. In particular, wxWidgets development packages may use different version numbers.

### 4. Clone and Build Malama

```bash
git clone https://github.com/Magpiny/malama.git
cd malama
```

Configure a release build with testing enabled:

```bash
cmake -B build \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON
```

Compile the application targets:

```bash
cmake --build build --parallel "$(nproc)"
```

### 5. Run Malama

```bash
./build/malama
```

Ensure the Ollama service is running before starting a conversation.

---

## Automated Testing

Malama includes an isolated, offline unit and stress-test suite built with **Catch2 v3** and integrated into CMake through **CTest**.

### Execution and Verification

Run the full automated test suite after building:

```bash
ctest --test-dir build --output-on-failure
```

Run selected test groups directly:

```bash
./build/tests/malama_tests "[unit]"
./build/tests/malama_tests "[stress],[performance]" --benchmark-samples 1000
```

### Test Suite Coverage

- **Attachment Queue Boundaries** — validates empty staging queues, invalid file paths, and out-of-range access.
- **Base64 Encoding Engine** — ensures corruption-free binary-to-text encoding for multimodal payloads.
- **MIME and Image Validation** — confirms malformed image assets fail gracefully without crashing the UI thread.
- **Prompt Enclosure Rules** — verifies context boundaries so untrusted attachment content remains segregated from instructions.
- **Stream Performance Metrics** — benchmarks high-throughput token accumulator and residual-buffer performance.

---

## CI/CD Pipeline

Continuous integration and deployment are managed through GitHub Actions using a containerized **Arch Linux** environment (`archlinux:base-devel`). This provides toolchain parity with modern GCC, C++23, and unified Boost dependency resolution.

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                      GitHub Actions Runner Host                         │
│                           (ubuntu-latest)                               │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │               Job Container (archlinux:base-devel)                │  │
│  │                                                                   │  │
│  │  1. pacman dependency provisioning                               │  │
│  │  2. Parallel compilation with CMake and Ninja                    │  │
│  │  3. CTest automated test execution                               │  │
│  │  4. AppImage assembly with linuxdeploy                           │  │
│  └───────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Workflow Architecture

The workflow is defined in [`.github/workflows/ci_cd.yml`](.github/workflows/ci_cd.yml).

#### Automated Verification (`build-test`)

- Runs on every push to `main` and on all pull requests.
- Provisions core packages using `pacman`.
- Compiles `malama_core` and the main application targets with `-Wall -Wextra -Werror`.
- Executes the complete Catch2 test matrix through `ctest`.

#### Automated Release Deployment (`package-appimage`)

- Runs when a version tag such as `v0.2.9` is pushed.
- Compiles optimized release binaries.
- Packages a standalone AppImage through `scripts/build_appimage.sh`.
- Publishes `Malama-v*-x86_64.AppImage` to GitHub Releases.

---

## Usage

### Attach Context Files

Click the **paperclip button 📎** next to the prompt editor and select one or more supported documents or images. Selected files appear in the attachment staging tray before submission.

### Remove Staged Files

Click the remove button (**X**) on an attachment chip. The file will be excluded from the next request without affecting the remaining staged attachments.

### Open a Previous Conversation

Move the pointer over an entry in the conversation sidebar. Valid entries display a hand cursor and tooltip. Select an entry to restore its conversation history in the active chat workspace.

### Filter Reasoning Blocks

Open:

```text
Settings → Preferences → Filter reasoning blocks
```

Enable the option to remove structural `<think>` blocks produced by supported reasoning models. The setting is applied to active response streams without requiring an application restart.

### Use Image Attachments

Attach a supported image, select a multimodal model, and submit a prompt such as:

```text
Describe the architecture shown in the attached diagram.
```

### Use Documents as Reference Context

Attach a supported document and provide a clear instruction:

```text
Summarize the attached document and list its main technical decisions.
```

For better results, identify the attached material as reference context rather than relying on the model to infer your intent.

---

## Privacy

Malama is designed around local model execution:

- Conversation history is stored locally.
- Requests are sent to your configured Ollama instance.
- No Malama cloud account is required.
- No embedded browser engine is used.
- Attached documents are processed on the host system.

Your overall privacy still depends on your operating system, Ollama configuration, installed models, and network environment.

---

## License

Malama is distributed under the terms of the [GNU General Public License v3.0](LICENSE).

<div align="center">

**Private by default. Native by design.**

</div>
