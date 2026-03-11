#!/usr/bin/env bash
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
VCPKG_ROOT_DEFAULT="${REPO_ROOT}/external/vcpkg"
VCPKG_ROOT="${VCPKG_ROOT:-${VCPKG_ROOT_DEFAULT}}"

NO_INSTALL=0

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

parse_args() {
  while (($#)); do
    case "$1" in
      --no-install)
        NO_INSTALL=1
        ;;
      --help|-h)
        cat <<EOF
Usage:
  ./${SCRIPT_NAME} [--no-install]

Options:
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

install_packages_macos() {
  if ! command -v brew >/dev/null 2>&1; then
    if [[ "$NO_INSTALL" -eq 1 ]]; then
      die "Homebrew is required to auto-install dependencies."
    fi
    info "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  fi
  info "Installing build dependencies..."
  brew install cmake ninja git curl pkg-config openssl@3
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

configure_build() {
  cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
}

build_project() {
  cmake --build "${BUILD_DIR}" --config Release
}

main() {
  parse_args "$@"
  local total=5
  local step=1

  progress_bar "$step" "$total" "Checking dependencies"
  if ! command -v cmake >/dev/null 2>&1 || ! command -v git >/dev/null 2>&1; then
    if [[ "$NO_INSTALL" -eq 1 ]]; then
      die "Missing dependencies. Install cmake and git or remove --no-install."
    fi
    install_packages_macos
  fi

  step=$((step+1))
  progress_bar "$step" "$total" "Preparing vcpkg"
  ensure_vcpkg

  step=$((step+1))
  progress_bar "$step" "$total" "Configuring build"
  configure_build

  step=$((step+1))
  progress_bar "$step" "$total" "Building Silicore-C"
  build_project

  step=$((step+1))
  progress_bar "$step" "$total" "Done"
  info "Build completed. Binary: ${BUILD_DIR}/silicore-c"
}

main "$@"
