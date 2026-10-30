set -euo pipefail

INSTALL_DIR="${HELPMAKE_DIR:-${HOME}/.local/share/HelpMake}"
BIN_DIR="${HOME}/.local/bin"
REPO="https://github.com/Gag91/Help_Make"
BINARY="hm"

echo "[HelpMake] Installing to ${INSTALL_DIR}"

if ! command -v git >/dev/null 2>&1; then
    echo "[HelpMake] Error: git not found in PATH." >&2
    echo "  Ubuntu/Debian: sudo apt install git" >&2
    echo "  Fedora:        sudo dnf install git" >&2
    echo "  macOS:         brew install git" >&2
    echo "  Arch:          sudo pacman -S git" >&2
    exit 1
fi

if [ -d "${INSTALL_DIR}" ]; then
    echo "[HelpMake] ${INSTALL_DIR} exists, updating..."
    if ! git -C "${INSTALL_DIR}" pull --ff-only; then
        echo "[HelpMake] Warning: git pull failed. Continuing with existing source." >&2
    fi
else
    if ! git clone --depth=1 "${REPO}" "${INSTALL_DIR}"; then
        echo "[HelpMake] Error: clone failed." >&2
        exit 1
    fi
fi

CXX=""
if command -v g++ >/dev/null 2>&1; then
    CXX="g++"
elif command -v clang++ >/dev/null 2>&1; then
    CXX="clang++"
else
    echo "[HelpMake] Error: no C++ compiler found (need g++ or clang++)." >&2
    echo "  Ubuntu/Debian: sudo apt install build-essential" >&2
    echo "  Fedora:        sudo dnf install gcc-c++" >&2
    echo "  macOS:         xcode-select --install" >&2
    echo "  Arch:          sudo pacman -S base-devel" >&2
    exit 1
fi

echo "[HelpMake] Building with ${CXX}..."

ORIG_DIR="$(pwd)"
cd "${INSTALL_DIR}"

if ! "${CXX}" -std=c++20 \
    src/cli/*.cpp \
    src/executor/*.cpp \
    src/fs/*.cpp \
    src/generator/*.cpp \
    src/getters/*.cpp \
    src/init/*.cpp \
    src/parser/*.cpp \
    src/process/*.cpp \
    src/toolchain/*.cpp \
    src/main.cpp \
    -o "${BINARY}" \
    -Iinclude
then
    echo "[HelpMake] Build failed." >&2
    cd "${ORIG_DIR}"
    exit 1
fi

cd "${ORIG_DIR}"

mkdir -p "${BIN_DIR}"
cp -f "${INSTALL_DIR}/${BINARY}" "${BIN_DIR}/${BINARY}"
chmod +x "${BIN_DIR}/${BINARY}"

case ":${PATH}:" in
    *":${BIN_DIR}:"*)
        echo "[HelpMake] ${BIN_DIR} already in PATH."
        ;;
    *)
        SHELL_NAME="$(basename "${SHELL:-bash}")"
        case "${SHELL_NAME}" in
            zsh)  PROFILE="${HOME}/.zshrc" ;;
            bash) PROFILE="${HOME}/.bashrc" ;;
            fish) PROFILE="${HOME}/.config/fish/config.fish" ;;
            *)    PROFILE="" ;;
        esac

        if [ -n "${PROFILE}" ] && [ -f "${PROFILE}" ]; then
            if ! grep -qF "${BIN_DIR}" "${PROFILE}" 2>/dev/null; then
                echo "" >> "${PROFILE}"
                echo "export PATH=\"${BIN_DIR}:\$PATH\"" >> "${PROFILE}"
                echo "[HelpMake] Added ${BIN_DIR} to ${PROFILE}."
            fi
        else
            echo "[HelpMake] Add this to your shell profile:"
            echo "  export PATH=\"${BIN_DIR}:\$PATH\""
        fi
        echo "[HelpMake] Restart your terminal (or run: source ${PROFILE:-your_profile}) for the change to take effect."
        ;;
esac

echo ""
echo "[HelpMake] Installed: ${BIN_DIR}/${BINARY}"
echo "[HelpMake] Run: hm --version"