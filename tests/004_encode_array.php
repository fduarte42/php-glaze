<?php
assert(glaze_encode([]) === '[]');
assert(glaze_encode([1, 2, 3]) === '[1,2,3]');
assert(glaze_encode(['a', 'b']) === '["a","b"]');
assert(glaze_encode([true, null, 1]) === '[true,null,1]');

// Associative array → object
$result = glaze_encode(['key' => 'value']);
assert($result === '{"key":"value"}', "got: $result");

$result = glaze_encode(['a' => 1, 'b' => 2]);
assert($result === '{"a":1,"b":2}', "got: $result");

echo "PASS\n";
