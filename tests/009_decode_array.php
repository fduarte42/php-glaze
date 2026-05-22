<?php
$result = glaze_decode('[1, 2, 3]');
assert($result === [1, 2, 3], 'integer array');

$result = glaze_decode('["a","b","c"]');
assert($result === ['a', 'b', 'c'], 'string array');

$result = glaze_decode('[]');
assert($result === [], 'empty array');

$result = glaze_decode('[true, null, 1.5]');
assert($result[0] === true);
assert($result[1] === null);
assert($result[2] === 1.5);

echo "PASS\n";
