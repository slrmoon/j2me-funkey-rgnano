#!/bin/sh
# Publishes the browser build (out/web/site, made by ./build.sh web) to the
# gh-pages branch, served by GitHub Pages:
#   https://<owner>.github.io/<repo>/
# DEMO_JAR (default: MotherLoad.jar next to the site, if present) is the
# game behind the page's "demo" button. Pushes to origin.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SITE=${SITE:-$ROOT/out/web/site}
[ -f "$SITE/runMidlet.wasm" ] || { echo "no $SITE: run ./build.sh web first" >&2; exit 1; }
[ -z "${DEMO_JAR:-}" ] || cp "$DEMO_JAR" "$SITE/MotherLoad.jar"
REV=$(git -C "$ROOT" rev-parse --short HEAD)
W=$(mktemp -d)
trap 'git -C "$ROOT" worktree remove --force "$W" 2>/dev/null || true; rm -rf "$W"' EXIT
git -C "$ROOT" fetch -q origin gh-pages
git -C "$ROOT" worktree add -q "$W" origin/gh-pages --detach
cd "$W"
git rm -rqf .
cp "$SITE"/* .
touch .nojekyll
git add -A
git commit -qm "Browser build of phoneME, from $REV" || { echo "nothing changed"; exit 0; }
git push -q origin HEAD:gh-pages
echo "published $REV"
