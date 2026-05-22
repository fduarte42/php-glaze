<?php
assert(function_exists('glaze_cbor_encode'), 'glaze_cbor_encode exists');
assert(function_exists('glaze_cbor_decode'), 'glaze_cbor_decode exists');

$data = ['name' => 'Bob', 'scores' => [1, 2, 3], 'active' => false, 'x' => null];

$cbor = glaze_cbor_encode($data);
assert(is_string($cbor), 'encode returns string');
assert(strlen($cbor) > 0, 'encode returns non-empty');

$decoded = glaze_cbor_decode($cbor);
assert(is_array($decoded), 'decode returns array');
assert($decoded['name'] === 'Bob', 'name round-trips');
assert($decoded['scores'] === [1, 2, 3], 'array round-trips');
assert($decoded['active'] === false, 'bool false round-trips');
assert($decoded['x'] === null, 'null round-trips');

// assoc=false returns stdClass
$obj = glaze_cbor_decode($cbor, false);
assert($obj instanceof stdClass, 'assoc=false returns stdClass');
assert($obj->name === 'Bob', 'stdClass property access');

// scalars
assert(glaze_cbor_decode(glaze_cbor_encode(99)) === 99, 'int scalar');
assert(glaze_cbor_decode(glaze_cbor_encode('world')) === 'world', 'string scalar');
assert(glaze_cbor_decode(glaze_cbor_encode(true)) === true, 'bool scalar');

echo "PASS\n";
