#!/bin/sh
# 커밋 전 검사기를 이 저장소에 깐다: Build/GitHooks/pre-commit → .git/hooks/pre-commit
#   sh Build/GitHooks/install.sh   (Git Bash, 저장소 아무 곳에서나)
root=$(git rev-parse --show-toplevel) || exit 1
hooks=$(git rev-parse --git-path hooks) || exit 1
mkdir -p "$hooks"
cp "$root/Build/GitHooks/pre-commit" "$hooks/pre-commit"
chmod +x "$hooks/pre-commit" 2>/dev/null
echo "깔았음: $hooks/pre-commit"
