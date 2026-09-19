#!/usr/bin/env bash
set -Eeuo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
output=${1:-"$root/dist"}
version=9.0.0
repository_name=$(basename "$root")
parent_directory=$(dirname "$root")
archive="$output/hsl-$version-source.tar.gz"
checksum="$archive.sha256"

mkdir -p "$output"
rm -f "$archive" "$checksum"

tar \
    --exclude="$repository_name/.git" \
    --exclude="$repository_name/build" \
    --exclude="$repository_name/build-*" \
    --exclude="$repository_name/dist" \
    --exclude="$repository_name/release-install" \
    --exclude="$repository_name/compile_commands.json" \
    --exclude="$repository_name/README.md.backup" \
    --exclude="$repository_name/stage" \
    -czf "$archive" \
    -C "$parent_directory" \
    "$repository_name"

sha256sum "$archive" > "$checksum"

printf '%s\n' "$archive"
