#!/usr/bin/env bash
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
VCPKG_ROOT_DEFAULT="${REPO_ROOT}/external/vcpkg"
VCPKG_ROOT="${VCPKG_ROOT:-${VCPKG_ROOT_DEFAULT}}"

DEFAULT_INSTALL_DIR="${PREFIX}/opt/silicore-c"
SYMLINK_PATH="${PREFIX}/bin/silicore-c"
BIN_NAME="silicore-c"
BIN_PATH="${BUILD_DIR}/${BIN_NAME}"

NO_INSTALL=0
MODE=""
INSTALL_DIR=""

info() {
  printf '[INFO] %s\n' "$*"
}

warn() {
  printf '[WARN] %s\n' "$*" >&2
}

die() {
  printf '[ERROR] %s\n' "$*" >&2
  exit 1
}

progress_bar() {
  local current="$1"
  local total="$2"
  local label="$3"
  local width=24
  local filled=$(( current * width / total ))
  local empty=$(( width - filled ))
  local bar=""
  local pad=""
  local body=$(( filled > 0 ? filled - 1 : 0 ))
  for ((i=0; i<body; i++)); do
    bar+="="
  done
  bar+=">"
  for ((i=0; i<empty; i++)); do
    pad+=" "
  done
  printf '[%s%s] %s\n' "$bar" "$pad" "$label"
}

prompt_yes_no() {
  local prompt="$1"
  local default_yes="${2:-y}"
  local reply
  while true; do
    if [[ "$default_yes" == "y" ]]; then
      read -r -p "${prompt} [Y/n]: " reply
      reply=${reply:-y}
    else
      read -r -p "${prompt} [y/N]: " reply
      reply=${reply:-n}
    fi
    case "${reply}" in
      y|Y|yes|YES) return 0 ;;
      n|N|no|NO) return 1 ;;
      *) echo "Please answer yes or no." ;;
    esac
  done
}

resolve_path() {
  if command -v realpath >/dev/null 2>&1; then
    realpath "$1"
    return
  fi
  if command -v readlink >/dev/null 2>&1; then
    readlink -f "$1" 2>/dev/null || echo "$1"
    return
  fi
  echo "$1"
}

find_install_dir() {
  local cmd
  cmd=$(command -v "$BIN_NAME" 2>/dev/null || true)
  if [[ -n "$cmd" ]]; then
    local resolved
    resolved=$(resolve_path "$cmd")
    if [[ -f "$resolved" ]]; then
      echo "$(dirname "$resolved")"
      return 0
    fi
  fi
  if [[ -f "${DEFAULT_INSTALL_DIR}/${BIN_NAME}" ]]; then
    echo "$DEFAULT_INSTALL_DIR"
    return 0
  fi
  return 1
}

prompt_install_dir() {
  if prompt_yes_no "Install to default (${DEFAULT_INSTALL_DIR})?" "y"; then
    echo "$DEFAULT_INSTALL_DIR"
    return
  fi
  read -r -p "Enter custom install path: " custom
  if [[ -z "$custom" ]]; then
    echo "$DEFAULT_INSTALL_DIR"
  else
    echo "$custom"
  fi
}

parse_args() {
  while (($#)); do
    case "$1" in
      --no-install)
        NO_INSTALL=1
        ;;
      --mode)
        shift
        MODE="${1:-}"
        ;;
      --mode=*)
        MODE="${1#*=}"
        ;;
      --help|-h)
        cat <<EOF
Usage:
  ./${SCRIPT_NAME} [--mode install|update|test|uninstall] [--no-install]

Options:
  --mode         Select mode without interactive menu.
  --no-install   Do not auto-install missing packages.
EOF
        exit 0
        ;;
      *)
        die "Unknown option: $1"
        ;;
    esac
    shift
  done
}

prompt_mode() {
  if [[ -n "$MODE" ]]; then
    return
  fi
  echo "Select mode:"
  echo "  1) Install"
  echo "  2) Update"
  echo "  3) Test"
  echo "  4) Uninstall"
  read -r -p "Enter choice: " choice
  case "$choice" in
    1|install|Install) MODE="install" ;;
    2|update|Update) MODE="update" ;;
    3|test|Test) MODE="test" ;;
    4|uninstall|Uninstall) MODE="uninstall" ;;
    *) die "Invalid selection." ;;
  esac
}

install_packages_termux() {
  command -v pkg >/dev/null 2>&1 || die "Termux pkg command not found."
  info "Installing build dependencies..."
  pkg update -y
  pkg install -y git cmake ninja clang make openssl curl libcurl pkg-config
}

ensure_vcpkg() {
  if [[ -d "${VCPKG_ROOT}" ]]; then
    info "Using vcpkg at ${VCPKG_ROOT}"
    return
  fi
  if [[ "$NO_INSTALL" -eq 1 ]]; then
    die "vcpkg not found. Set VCPKG_ROOT or remove --no-install to bootstrap."
  fi
  info "Cloning vcpkg into ${VCPKG_ROOT}"
  mkdir -p "$(dirname "${VCPKG_ROOT}")"
  git clone https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}"
  "${VCPKG_ROOT}/bootstrap-vcpkg.sh"
}

ensure_ports() {
  "${VCPKG_ROOT}/vcpkg" install curl openssl nlohmann-json
}

configure_build() {
  cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
}

build_project() {
  cmake --build "${BUILD_DIR}" --config Release
}

build_binary() {
  local total=4
  local step=1
  progress_bar "$step" "$total" "Checking dependencies"
  if ! command -v cmake >/dev/null 2>&1 || ! command -v git >/dev/null 2>&1; then
    if [[ "$NO_INSTALL" -eq 1 ]]; then
      die "Missing dependencies. Install cmake and git or remove --no-install."
    fi
    install_packages_termux
  fi

  step=$((step+1))
  progress_bar "$step" "$total" "Preparing vcpkg"
  ensure_vcpkg

  step=$((step+1))
  progress_bar "$step" "$total" "Installing vcpkg ports"
  ensure_ports

  step=$((step+1))
  progress_bar "$step" "$total" "Configuring and building"
  configure_build
  build_project

  if [[ ! -x "$BIN_PATH" ]]; then
    die "Build failed: ${BIN_PATH} not found."
  fi
}

install_binary() {
  mkdir -p "$INSTALL_DIR"
  install -m 755 "$BIN_PATH" "${INSTALL_DIR}/${BIN_NAME}"
  mkdir -p "$(dirname "$SYMLINK_PATH")"
  ln -sf "${INSTALL_DIR}/${BIN_NAME}" "$SYMLINK_PATH"
}

mode_install() {
  local existing
  if existing=$(find_install_dir); then
    warn "Silicore-C already installed at ${existing}."
    if ! prompt_yes_no "Overwrite existing installation?" "n"; then
      info "Installation aborted."
      exit 0
    fi
  fi

  INSTALL_DIR=$(prompt_install_dir)
  build_binary
  install_binary
  info "Installed Silicore-C to ${INSTALL_DIR}."
  info "Symlink: ${SYMLINK_PATH}"
}

mode_update() {
  local existing
  if ! existing=$(find_install_dir); then
    warn "Silicore-C not found."
    if prompt_yes_no "Install it now?" "y"; then
      mode_install
      return
    fi
    exit 0
  fi
  INSTALL_DIR="$existing"
  build_binary
  install_binary
  info "Updated Silicore-C at ${INSTALL_DIR}."
}

mode_test() {
  build_binary
  info "Test build ready. Binary: ${BIN_PATH}"
}

mode_uninstall() {
  local existing
  if ! existing=$(find_install_dir); then
    warn "Silicore-C not installed."
    exit 0
  fi
  INSTALL_DIR="$existing"
  if ! prompt_yes_no "Uninstall Silicore-C from ${INSTALL_DIR}?" "n"; then
    info "Uninstall aborted."
    exit 0
  fi
  if [[ -e "$SYMLINK_PATH" ]]; then
    rm -f "$SYMLINK_PATH"
  fi
  rm -rf "$INSTALL_DIR"
  info "Uninstalled Silicore-C."
}

main() {
  parse_args "$@"
  prompt_mode
  case "$MODE" in
    install) mode_install ;;
    update) mode_update ;;
    test) mode_test ;;
    uninstall) mode_uninstall ;;
    *) die "Unknown mode: $MODE" ;;
  esac
}

main "$@"
