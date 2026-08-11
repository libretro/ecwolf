#!/bin/sh
# Convert ecwolf's libretro-common submodule into a vendored in-tree copy.
#
# Run from the root of an ecwolf checkout.  Takes the 63 files the build
# actually reaches (the transitive gcc -MM closure over SOURCES_C and
# SOURCES_CXX) out of libretro-common at LC_REV, drops the submodule and
# the stray second tree at src/libretro-common, then leaves everything
# staged for you to inspect and commit.
#
# Also makes the three edits this needs: Makefile.common loses the
# src/libretro-common include path and repoints rdds.c, and
# .gitlab-ci.yml stops recursing a submodule that no longer exists.

set -e

LC_URL=${LC_URL:-https://github.com/libretro/libretro-common.git}
LC_REV=${LC_REV:-879c8d507b0b52e77e27d759239c2b5df1e26dfd}
SUB=src/libretro/libretro-common

[ -d src/libretro ] || { echo "run me from the ecwolf checkout root" >&2; exit 1; }

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

echo "fetching libretro-common $LC_REV"
git clone -q "$LC_URL" "$TMP/lc"
git -C "$TMP/lc" checkout -q "$LC_REV"

echo "removing the submodule"
git submodule deinit -f "$SUB" >/dev/null 2>&1 || true
git rm -q --cached "$SUB" 2>/dev/null || true
rm -f .gitmodules
rm -rf .git/modules/src/libretro/libretro-common "$SUB"

echo "vendoring the 63 files the build reaches"
mkdir -p "$SUB"
for f in \
    compat/compat_posix_string.c \
    compat/compat_snprintf.c \
    compat/compat_strl.c \
    compat/fopen_utf8.c \
    encodings/encoding_crc32.c \
    encodings/encoding_crc32_tables.h \
    encodings/encoding_deflate.c \
    encodings/encoding_utf.c \
    features/features_cpu.c \
    file/file_path.c \
    file/file_path_io.c \
    file/retro_dirent.c \
    formats/7z/r7z_archive.c \
    formats/7z/r7z_bcj2.c \
    formats/7z/r7z_filters.c \
    formats/7z/r7z_lzma.c \
    formats/7z/r7z_lzma2.c \
    formats/7z/r7z_lzma_stream.c \
    formats/dds/rdds.c \
    formats/jpeg/rjpeg.c \
    formats/vorbis/rvorbis.c \
    include/7z/r7z_archive.h \
    include/7z/r7z_bcj2.h \
    include/7z/r7z_filters.h \
    include/7z/r7z_lzma.h \
    include/7z/r7z_lzma2.h \
    include/7z/r7z_lzma_stream.h \
    include/boolean.h \
    include/compat/fopen_utf8.h \
    include/compat/msvc.h \
    include/compat/posix_string.h \
    include/compat/strcasestr.h \
    include/compat/strl.h \
    include/encodings/crc32.h \
    include/encodings/deflate.h \
    include/encodings/utf.h \
    include/features/features_cpu.h \
    include/file/file_path.h \
    include/formats/image.h \
    include/formats/rdds.h \
    include/formats/rjpeg.h \
    include/formats/rvorbis.h \
    include/libretro.h \
    include/retro_atomic.h \
    include/retro_common.h \
    include/retro_common_api.h \
    include/retro_dirent.h \
    include/retro_endianness.h \
    include/retro_environment.h \
    include/retro_inline.h \
    include/retro_miscellaneous.h \
    include/retro_timers.h \
    include/streams/file_stream.h \
    include/string/stdstring.h \
    include/time/rtime.h \
    include/vfs/vfs.h \
    include/vfs/vfs_hybrid.h \
    include/vfs/vfs_implementation.h \
    streams/file_stream.c \
    string/stdstring.c \
    time/rtime.c \
    vfs/vfs_hybrid.c \
    vfs/vfs_implementation.c
do
    mkdir -p "$SUB/$(dirname "$f")"
    cp "$TMP/lc/$f" "$SUB/$f"
done

echo "removing the stray second tree"
rm -rf src/libretro-common

echo "repointing Makefile.common and .gitlab-ci.yml"
M=src/libretro/Makefile.common
sed -i.bak \
    -e '\|-I$(CORE_DIR)/src/libretro-common/include|d' \
    -e 's|$(CORE_DIR)/src/libretro-common/formats/dds/rdds.c|$(CORE_DIR)/src/libretro/libretro-common/formats/dds/rdds.c|' \
    -e 's|^# RetroArch libretro-common DDS decoder (rdds), vendored under$|# libretro-common DDS decoder (rdds), backing the FDDSTexture glue.|' \
    -e '\|^# src/libretro-common so the FDDSTexture glue is self-contained\.$|d' \
    "$M"
rm -f "$M.bak"

sed -i.bak -e '/GIT_SUBMODULE_STRATEGY/d' -e '/GIT_SUBMODULE_PATHS/d' .gitlab-ci.yml
rm -f .gitlab-ci.yml.bak

git add -A

echo
echo "vendored $(find "$SUB" -type f | wc -l) files at $LC_REV"
echo "review with git status / git diff --cached, then commit."
