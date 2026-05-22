<?php
// UTF-8 strings must survive a round-trip
$strings = [
    'Hello, 世界',
    'こんにちは',
    'Привет мир',
    'مرحبا',
    "emoji: \u{1F600}",
];

foreach ($strings as $s) {
    $rt = glaze_decode(glaze_encode($s));
    assert($rt === $s, "round-trip failed for: $s");
}

echo "PASS\n";
