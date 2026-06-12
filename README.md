# malama

**malama** is a high-performance, local-first LLM interface designed for engineers and minimalists. Built from the ground up using native C++23 and wxWidgets, it provides a lightning-fast, resource-efficient experience for interacting with local language models, ensuring your development environment stays lightweight and responsive.

[![Licence: Apache-2.0](https://img.shields.io/badge/licence-Apache--2.0-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](#)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](#)
[![Built with wxWidgets](https://img.shields.io/badge/UI-wxWidgets%203.3-green.svg)](#)

---

## 🚀 Key Features

* **Ultra-Minimalist Footprint:** Consumes ~40MB of RAM, ensuring maximum hardware resources are dedicated to model inference.
* **Native C++ Performance:** Zero-overhead UI rendering using native OS controls.
* **Privacy-Focused:** A local-first architecture—your data never leaves your machine.
* **Low-Latency Streaming:** Asynchronous, non-blocking TCP socket implementation for real-time code and text generation.
* **Integrated Workflow:** Native clipboard support, system notifications, and a distraction-free workspace.

---

## 🎥 Introduction

See **malama** v0.1.0 in action:

[Video of using_malamav0.1.0]
<video width="640" height="360" controls>
  <source src="assets/malama_v0.1.0mvp.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

[Watch the malama v0.1.0 introduction](assets/malama_v0.1.0mvp.mp4)
---

## 🏗 Architecture & Engineering

`malama` is engineered for stability and speed. Key architectural pillars include:

* **Memory Safety:** Exception-free `std::nothrow` allocation strategy to ensure the application remains stable under extreme memory pressure.
* **Concurrency:** Uses a dedicated background `std::jthread` and `Boost.Asio` event loop to manage high-throughput socket communication without locking the UI thread.
* **Thread Safety:** Implements a global event bus for thread-safe UI updates, preventing race conditions common in multi-threaded interface applications.

---

## 🛠 Build Requirements

`malama` is designed for Linux-based environments. You will need:

* **Compiler:** A C++23 compliant compiler (GCC 13+ or Clang 16+).
* **Dependencies:**
* `wxWidgets` (3.2+)
* `Boost.Asio`
* `spdlog` (for logging)
* `glaze` (for JSON processing)


* **Build System:** `CMake` (3.28+)

---

## ⚙️ Build Instructions

```bash
# Clone the repository
git clone https://github.com/Magpiny/malama.git
cd malama

# Configure and build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Execute
./malama

```

---

## 📋 Roadmap

* **v0.1.1:** Enhanced UI aesthetics (rounded corners, improved typography) and interaction refinement.
* **v0.2.0:** Persistent session storage and chat history management.
* **v0.3.0:** Dynamic model switching and advanced API configuration.
* **v1.0.0:** Stable release candidate and refined toolset for professional engineering workflows.

---

## ⚖️ Licence

Distributed under the Apache-2.0 Licence. See `LICENCE` for more information.

---

*Engineered with precision for the minimalist developer.*
