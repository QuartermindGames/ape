#!/bin/sh
echo "Core Engine"
cloc --include-lang="C,C++,C/C++ Header" src/acm src/common src/core src/game src/kernel/plcore src/kernel/plgraphics src/shells
echo "Editor Frontend (Forge)"
cloc --include-lang="C,C++,C/C++ Header" src/forge