#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build

if command -v tectonic >/dev/null 2>&1; then
    engine="tectonic"
elif [ -x .tools/tectonic ]; then
    engine=".tools/tectonic"
elif command -v xelatex >/dev/null 2>&1; then
    for pass in 1 2 3; do
        xelatex -interaction=nonstopmode -halt-on-error -output-directory=build parking_report.tex
    done
    cp build/parking_report.pdf parking_report.pdf
    echo "PDF ready: $(pwd)/parking_report.pdf"
    exit 0
else
    echo "Không tìm thấy tectonic hoặc xelatex." >&2
    echo "Cài tectonic: https://tectonic-typesetting.github.io/" >&2
    exit 1
fi

"$engine" --keep-logs --keep-intermediates --outdir build parking_report.tex
cp build/parking_report.pdf parking_report.pdf
echo "PDF ready: $(pwd)/parking_report.pdf"
