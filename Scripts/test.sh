#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 Scripts/compile_chapter.py --check
python3 Scripts/compile_world.py --check
python3 Scripts/compile_tags.py --check
mkdir -p .test-bin
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g -ISource/WroclawTheGame Tests/progression.cpp -o .test-bin/progression
.test-bin/progression
g++ -std=c++20 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer -g -I Source/WroclawTheGame Tests/openworld.cpp -o .test-bin/openworld
.test-bin/openworld
"${CXX:-g++}" -std=c++20 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g -ISource/WroclawTheGame Tests/races.cpp -o .test-bin/races
.test-bin/races
python3 -m unittest discover -s Tests -p 'test_*.py' -v
python3 -m compileall -q Scripts
