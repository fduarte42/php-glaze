<?php
$data = ['a' => 1, 'b' => [1, 2]];

$compact = glaze_encode($data);
$pretty  = glaze_encode($data, GLAZE_PRETTY_PRINT);

// Compact form has no newlines
assert(strpos($compact, "\n") === false, 'compact should have no newlines');

// Pretty form has newlines and indentation
assert(strpos($pretty, "\n") !== false, 'pretty should have newlines');
assert(strpos($pretty, '   ') !== false || strpos($pretty, "\t") !== false,
    'pretty should be indented');

// Both decode to the same value
assert(glaze_decode($compact) === glaze_decode($pretty),
    'compact and pretty should decode identically');

echo "PASS\n";
