<?php
assert(function_exists('glaze_yaml_encode'), 'glaze_yaml_encode exists');
assert(function_exists('glaze_yaml_decode'), 'glaze_yaml_decode exists');

$data = ['name' => 'Diana', 'score' => 99, 'ratio' => 2.71, 'active' => false];

$yaml = glaze_yaml_encode($data);
assert(is_string($yaml), 'encode returns string');
assert(strlen($yaml) > 0, 'encode returns non-empty');
// YAML output should contain key: value pairs
assert(strpos($yaml, 'name') !== false, 'YAML contains key');

$decoded = glaze_yaml_decode($yaml);
assert(is_array($decoded), 'decode returns array');
assert($decoded['name'] === 'Diana', 'name round-trips');
assert($decoded['score'] === 99, 'int round-trips');
assert(abs($decoded['ratio'] - 2.71) < 1e-10, 'float round-trips');
assert($decoded['active'] === false, 'bool false round-trips');

// assoc=false returns stdClass
$obj = glaze_yaml_decode($yaml, false);
assert($obj instanceof stdClass, 'assoc=false returns stdClass');
assert($obj->name === 'Diana', 'stdClass property access');

// array values
$list = ['items' => ['a', 'b', 'c']];
$yaml2 = glaze_yaml_encode($list);
assert(is_string($yaml2), 'array encodes');
$dec2 = glaze_yaml_decode($yaml2);
assert($dec2['items'] === ['a', 'b', 'c'], 'array round-trips');

echo "PASS\n";
