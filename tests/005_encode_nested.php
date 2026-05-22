<?php
$data = [
    'name'  => 'Alice',
    'age'   => 30,
    'tags'  => ['php', 'c++'],
    'meta'  => ['active' => true, 'score' => 9.5],
];

$json = glaze_encode($data);
assert(is_string($json), 'encode should return string');

// Round-trip through PHP's own decoder to verify structure
$decoded = json_decode($json, true);
assert($decoded['name']  === 'Alice');
assert($decoded['age']   === 30);
assert($decoded['tags']  === ['php', 'c++']);
assert($decoded['meta']['active'] === true);
assert($decoded['meta']['score']  === 9.5);

echo "PASS\n";
