<?php
assert(function_exists('glaze_toml_encode'), 'glaze_toml_encode exists');
assert(function_exists('glaze_toml_decode'), 'glaze_toml_decode exists');

$data = ['name' => 'Charlie', 'score' => 42, 'ratio' => 3.14, 'active' => true];

$toml = glaze_toml_encode($data);
assert(is_string($toml), 'encode returns string');
assert(strlen($toml) > 0, 'encode returns non-empty');
// TOML output should contain key = value pairs
assert(strpos($toml, 'name') !== false, 'TOML contains key');

$decoded = glaze_toml_decode($toml);
assert(is_array($decoded), 'decode returns array');
assert($decoded['name'] === 'Charlie', 'name round-trips');
assert($decoded['score'] === 42, 'int round-trips');
assert(abs($decoded['ratio'] - 3.14) < 1e-10, 'float round-trips');
assert($decoded['active'] === true, 'bool round-trips');

// assoc=false returns stdClass
$obj = glaze_toml_decode($toml, false);
assert($obj instanceof stdClass, 'assoc=false returns stdClass');
assert($obj->name === 'Charlie', 'stdClass property access');

// nested object
$nested = ['server' => ['host' => 'localhost', 'port' => 8080]];
$toml2 = glaze_toml_encode($nested);
assert(is_string($toml2), 'nested encodes');
$dec2 = glaze_toml_decode($toml2);
assert($dec2['server']['host'] === 'localhost', 'nested host round-trips');
assert($dec2['server']['port'] === 8080, 'nested port round-trips');

echo "PASS\n";
