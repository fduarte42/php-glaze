#!/usr/bin/env bash
set -uo pipefail

EXT="${1:-build/glaze.so}"
PASS=0
FAIL=0

if [ ! -f "$EXT" ]; then
    echo "Extension not found: $EXT"
    exit 1
fi

for test_file in tests/*.php; do
    output=$(php -n -d "extension=$EXT" "$test_file" 2>&1) || true
    exit_code=$?

    if [ "$output" = "PASS" ]; then
        echo "PASS  $test_file"
        PASS=$((PASS + 1))
    else
        echo "FAIL  $test_file"
        [ -n "$output" ] && echo "      $output"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
