<?php
assert(function_exists('glaze_beve_encode'), 'glaze_beve_encode exists');
assert(function_exists('glaze_beve_decode'), 'glaze_beve_decode exists');

$data = ['name' => 'Alice', 'scores' => [9, 10, 8], 'active' => true, 'x' => null];

$beve = glaze_beve_encode($data);
assert(is_string($beve), 'encode returns string');
assert(strlen($beve) > 0, 'encode returns non-empty');

$decoded = glaze_beve_decode($beve);
assert(is_array($decoded), 'decode returns array');
assert($decoded['name'] === 'Alice', 'name round-trips');
assert($decoded['scores'] === [9, 10, 8], 'array round-trips');
assert($decoded['active'] === true, 'bool round-trips');
assert($decoded['x'] === null, 'null round-trips');

// assoc=false returns stdClass
$obj = glaze_beve_decode($beve, false);
assert($obj instanceof stdClass, 'assoc=false returns stdClass');
assert($obj->name === 'Alice', 'stdClass property access');

// scalars
assert(glaze_beve_decode(glaze_beve_encode(42)) === 42, 'int scalar');
assert(glaze_beve_decode(glaze_beve_encode('hello')) === 'hello', 'string scalar');
assert(glaze_beve_decode(glaze_beve_encode(3.14)) === 3.14, 'float scalar');
assert(glaze_beve_decode(glaze_beve_encode(true)) === true, 'bool scalar');
assert(glaze_beve_decode(glaze_beve_encode(null)) === null, 'null scalar');

echo "PASS\n";
