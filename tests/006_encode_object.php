<?php
$obj = new stdClass();
$obj->name = 'Bob';
$obj->score = 7;

$json = glaze_encode($obj);
assert(is_string($json), 'encode should return string');

$decoded = json_decode($json, true);
assert($decoded['name']  === 'Bob');
assert($decoded['score'] === 7);

echo "PASS\n";
