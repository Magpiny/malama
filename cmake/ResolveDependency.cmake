# /////////////////////////////////////////////////////////////////////////////
# Name:        cmake/ResolveDependency.cmake
# Purpose:     Interactive, cross-distro auto-installer with confirmation hooks
# Author:      Wanjare S. <samuelwanjare@protonmail.com>
# Created:     2026-07-16
# Copyright:   (c) 2026 Magpiny. All rights reserved.
# Licence:     GPL-3.0-or-later
# /////////////////////////////////////////////////////////////////////////////

include(CMakeParseArguments)

# Setup human-friendly ANSI color terminal control codes with names > 3 chars
string(ASCII 27 AnsiEscapeChar)
set(ColorReset   "${AnsiEscapeChar}[0m")
set(ColorBlue    "${AnsiEscapeChar}[34m")
set(ColorYellow  "${AnsiEscapeChar}[33m")
set(ColorRed     "${AnsiEscapeChar}[31m")

function(resolve_dependency)
    set(OptionsList "")
    set(SingleValueArgs TARGET_NAME FIND_NAME REASON PACMAN_NAME APT_NAME DNF_NAME)
    set(MultiValueArgs "")
    
    cmake_parse_arguments(PARAM 
        "${OptionsList}" "${SingleValueArgs}" "${MultiValueArgs}" ${ARGN}
    )

    # Step 1: Query host registry silently before triggering prompts
    message(STATUS 
        "${ColorBlue}🔵 [ℹ️ INFO] => Auditing host for ${PARAM_TARGET_NAME}...${ColorReset}"
    )
    find_package(${PARAM_FIND_NAME} QUIET)

    set(LookupFoundFlag "${PARAM_FIND_NAME}_FOUND")

    # FIXED: Replaced expression evaluation with structural positive variable tests
    if(${LookupFoundFlag})
        message(STATUS 
            "${ColorBlue}🔵 [ℹ️ INFO] => ${PARAM_TARGET_NAME} located cleanly.${ColorReset}"
        )
    else()
        message(STATUS "")
        message(STATUS 
            "${ColorYellow}🟡 [⚠️ NEEDED] => '${PARAM_TARGET_NAME}' is missing!${ColorReset}"
        )
        message(STATUS "${ColorBlue}   Description: ${PARAM_REASON}${ColorReset}")
        message(STATUS "")

        # Step 2: Auto-detect active host system package manager binaries
        set(InstallCommand "")
        find_program(PACMAN_EXEC pacman)
        find_program(APT_EXEC apt-get)
        find_program(DNF_EXEC dnf)

        if(PACMAN_EXEC)
            set(InstallCommand "sudo pacman -S --noconfirm ${PARAM_PACMAN_NAME}")
        elseif(APT_EXEC)
            set(InstallCommand "sudo apt-get install -y ${PARAM_APT_NAME}")
        elseif(DNF_EXEC)
            set(InstallCommand "sudo dnf install -y ${PARAM_DNF_NAME}")
        else()
            # Fallback route using curl source downpour streams if PM missing
            set(InstallCommand 
                "curl -fsSL https://magpiny.dev/deps/install_${PARAM_TARGET_NAME}.sh | sh"
            )
        endif()

        # Step 3: Launch interactive user terminal input prompt loop
        set(UserResponseString "n")
        
        set(PromptCommand "read -p '👉 Install ${PARAM_TARGET_NAME}? (y/N): ' ")
        string(APPEND PromptCommand "ResponseBuffer < /dev/tty && echo \$ResponseBuffer")
        
        execute_process(
            COMMAND bash -c "${PromptCommand}"
            OUTPUT_VARIABLE UserResponseString
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        string(TOLOWER "${UserResponseString}" UserResponseLower)

        # Step 4: Evaluate confirmation flag options matching strict semantic models
        if(UserResponseLower STREQUAL "y" OR UserResponseLower STREQUAL "yes")
            message(STATUS 
                "${ColorBlue}🔵 [ℹ️ INFO] => Executing: ${InstallCommand}${ColorReset}"
            )
            
            execute_process(
                COMMAND bash -c "${InstallCommand} < /dev/tty"
                RESULT_VARIABLE ExecutionResultCode
            )

            if(ExecutionResultCode EQUAL 0)
                message(STATUS 
                    "${ColorBlue}🔵 [ℹ️ INFO] => ${PARAM_TARGET_NAME} setup complete.${ColorReset}"
                )
                find_package(${PARAM_FIND_NAME} REQUIRED)
            else()
                message(FATAL_ERROR 
                    "${ColorRed}🔴 [❌ ERROR] => Failure code: ${ExecutionResultCode}${ColorReset}"
                )
            endif()
        else()
            message(FATAL_ERROR 
                "${ColorRed}🔴 [❌ ERROR] => Aborted. ${PARAM_TARGET_NAME} is required.${ColorReset}"
            )
        endif()
    endif()
endfunction()
