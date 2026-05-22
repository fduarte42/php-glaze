<?php
$functions = [
    'glaze_encode',
    'glaze_decode',
    'glaze_prettify',
    'glaze_minify',
    'glaze_validate',
];

foreach ($functions as $fn) {
    if (!function_exists($fn)) {
        fwrite(STDERR, "FAIL: function $fn not found\n");
        exit(1);
    }
}

if (!defined('GLAZE_PRETTY_PRINT')) {
    fwrite(STDERR, "FAIL: constant GLAZE_PRETTY_PRINT not defined\n");
    exit(1);
}

echo "PASS\n";
