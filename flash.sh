#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$ROOT_DIR/bme280-example"
BUILD_DIR="$PROJECT_DIR/build/Debug"
ELF_FILE="$BUILD_DIR/bme280.elf"
BIN_FILE="$BUILD_DIR/bme280.bin"
OPENOCD_INTERFACE="interface/stlink.cfg"
OPENOCD_TARGET="target/stm32wbx.cfg"
BUILD_ONLY=0
FLASH_ONLY=0

usage() {
  cat <<USAGE
Usage: ./flash.sh [--build-only] [--flash-only]

Builds the STM32WB55 Debug firmware and flashes it through the Nucleo ST-LINK.

Options:
  --build-only   Build and create bme280.bin, but do not flash.
  --flash-only   Flash the existing build/Debug/bme280.elf without rebuilding.
  -h, --help     Show this help.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-only)
      BUILD_ONLY=1
      ;;
    --flash-only)
      FLASH_ONLY=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

if [[ "$BUILD_ONLY" -eq 1 && "$FLASH_ONLY" -eq 1 ]]; then
  echo "--build-only and --flash-only cannot be used together." >&2
  exit 2
fi

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Required command not found: $1" >&2
    exit 1
  fi
}

find_objcopy() {
  if command -v arm-none-eabi-objcopy >/dev/null 2>&1; then
    command -v arm-none-eabi-objcopy
    return 0
  fi

  local compiler
  compiler="$(awk -F= '/^CMAKE_C_COMPILER:FILEPATH=/ { print $2 }' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null || true)"
  if [[ -n "$compiler" ]]; then
    local candidate
    candidate="$(dirname "$compiler")/arm-none-eabi-objcopy"
    if [[ -x "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  fi

  local compiler_ar
  compiler_ar="$(awk -F= '/^CMAKE_C_COMPILER_AR:FILEPATH=/ { print $2 }' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null || true)"
  if [[ -n "$compiler_ar" ]]; then
    local candidate
    candidate="$(dirname "$compiler_ar")/arm-none-eabi-objcopy"
    if [[ -x "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  fi

  local homebrew_objcopy
  homebrew_objcopy="$(find /opt/homebrew -path '*/arm-none-eabi-binutils/*/bin/arm-none-eabi-objcopy' -type f -perm -111 2>/dev/null | head -n 1)"
  if [[ -n "$homebrew_objcopy" ]]; then
    printf '%s\n' "$homebrew_objcopy"
    return 0
  fi

  echo "arm-none-eabi-objcopy not found. Run the build once, or add the ARM toolchain bin directory to PATH." >&2
  return 1
}

build_firmware() {
  require_cmd cmake

  if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    cmake --preset Debug -S "$PROJECT_DIR"
  fi

  cmake --build "$BUILD_DIR"

  local objcopy
  objcopy="$(find_objcopy)"
  "$objcopy" -O binary "$ELF_FILE" "$BIN_FILE"
  echo "Built:"
  echo "  $ELF_FILE"
  echo "  $BIN_FILE"
}

flash_firmware() {
  require_cmd openocd

  if [[ ! -f "$ELF_FILE" ]]; then
    echo "Missing firmware: $ELF_FILE" >&2
    echo "Run ./flash.sh first, or run ./flash.sh --build-only." >&2
    exit 1
  fi

  openocd \
    -f "$OPENOCD_INTERFACE" \
    -f "$OPENOCD_TARGET" \
    -c "program $ELF_FILE verify reset exit"
}

if [[ "$FLASH_ONLY" -eq 0 ]]; then
  build_firmware
fi

if [[ "$BUILD_ONLY" -eq 0 ]]; then
  flash_firmware
fi
