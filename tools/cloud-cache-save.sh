#!/bin/bash
# Saves the compiler cache (ccache's directory) to the private repository
# CACHE_REPO (default slrmoon/j2me-build-cache) as build-cache.tar.zst.NN
# (45 MB parts: GitHub warns from 50 MB, refuses 100 MB), in one commit that replaces the
# previous one (force push: the repository keeps no history of caches).
# Run it after building in a container set up by tools/cloud-setup.sh.

set -eu
TC=${TC:-/root/tc}
CACHE_REPO=${CACHE_REPO:-slrmoon/j2me-build-cache}
. "$TC/env.sh"

[ -d "$CCACHE_DIR" ] || { echo "no cache in $CCACHE_DIR" >&2; exit 1; }
ccache -c >/dev/null 2>&1 || true      # trim to CCACHE_MAXSIZE
ccache -s | grep -i "cache size" || true

R=$TC/cache-repo
if [ ! -d "$R/.git" ]; then
    rm -rf "$R"
    git clone -q --depth 1 "https://github.com/$CACHE_REPO" "$R" 2>/dev/null ||
        { mkdir -p "$R" && git -C "$R" init -q &&
          git -C "$R" remote add origin "https://github.com/$CACHE_REPO"; }
fi
rm -f "$R"/build-cache.tar.zst.*
tar -C "$TC" -cf - ccache | zstd -q -T0 -10 | split -b 45M -d - "$R/build-cache.tar.zst."
du -ch "$R"/build-cache.tar.zst.* | tail -n 1

cat > "$R/README.md" <<TXT
Compiler cache (ccache) of slrmoon/j2me-funkey-rgnano for fresh cloud
containers: tools/cloud-setup.sh restores it, tools/cloud-cache-save.sh
replaces it (one commit, force-pushed). Nothing here is needed to build;
it only makes builds faster.
TXT
cd "$R"
git checkout -q --orphan new-cache
git add -A
git -c user.name="build cache" -c user.email="build-cache@localhost" \
    commit -q -m "build cache $(date -u +%Y-%m-%d)"
git push -q -f origin new-cache:main
git branch -q -M main
echo "saved to $CACHE_REPO"
