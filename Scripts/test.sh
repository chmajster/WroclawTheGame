#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 Scripts/compile_chapter.py --check
mkdir -p .test-bin
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g -ISource/WroclawTheGame Tests/progression.cpp -o .test-bin/progression
.test-bin/progression
python3 -m unittest discover -s Tests -p 'test_*.py' -v
python3 -m py_compile Scripts/prepare_content.py Scripts/make_source_assets.py
