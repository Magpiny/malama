malama/
├── CMakeLists.txt                # Root configuration (Generates compile_commands.json)
├── .clang-format                 # Strict layout controls (Attached braces, right pointers)
├── .clang-tidy                   # Static analysis (Enforces m_ naming rules)
├── libs/                         # Local internal dependencies / Vendor source tree
│   ├── glaze/                    # Pinned header-only JSON deserialization engine
│   ├── spdlog/                   # Structured asynchronous logging engine
│   └── boost/                    # Stripped, header-only Boost.Asio & Boost.Beast subsets
├── src/                          # Main application codebase
│   ├── main.cpp                  # Main application entry (wxApp implementation)
│   ├── common/                   # Global data entities (types.hpp, forward.hpp)
│   ├── core/                     # Business domain (config_manager, chat_engine)
│   ├── network/                  # Transport wrappers (ollama_client, stream_worker)
│   └── ui/                       # Graphic presentation panels (main_frame, chat_view, etc.)
└── tests/                        # Tiered verification architecture
    ├── CMakeLists.txt            # Dedicated testing compilation framework
    ├── main_test.cpp             # Framework runner entry point
    ├── unit/                     # Component isolation tests (test_config, test_types)
    ├── integration/              # Multi-module & Cross-Distribution validation
    │   ├── test_network_ui.cpp   # Validates thread event routing boundaries
    │   └── distro_matrix/        # Headless automated environment validations
    │       ├── run_matrix.sh     # Controller script for execution sweeps
    │       ├── ubuntu.dockerfile # Emulates Ubuntu runtime layout & GTK bindings
    │       ├── arch.dockerfile   # Emulates generic upstream Arch Linux baseline
    │       └── fedora.dockerfile # Emulates Fedora workspace configuration
    └── stress/                   # Resource exhaustion, leaking, and scaling metrics
