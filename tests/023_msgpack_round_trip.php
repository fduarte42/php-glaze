<?php
assert(function_exists('glaze_msgpack_encode'), 'glaze_msgpack_encode exists');
assert(function_exists('glaze_msgpack_decode'), 'glaze_msgpack_decode exists');

$data = ['name' => 'Alice', 'scores' => [10, 20, 30], 'active' => true, 'x' => null];

$mp = glaze_msgpack_encode($data);
assert(is_string($mp), 'encode returns string');
assert(strlen($mp) > 0, 'encode returns non-empty');

$decoded = glaze_msgpack_decode($mp);
assert(is_array($decoded), 'decode returns array');
assert($decoded['name'] === 'Alice', 'string round-trips');
assert($decoded['scores'] === [10, 20, 30], 'array round-trips');
assert($decoded['active'] === true, 'bool round-trips');
assert($decoded['x'] === null, 'null round-trips');

// assoc=false returns stdClass
$obj = glaze_msgpack_decode($mp, false);
assert($obj instanceof stdClass, 'assoc=false returns stdClass');
assert($obj->name === 'Alice', 'stdClass property access');

// scalars
assert(glaze_msgpack_decode(glaze_msgpack_encode(99)) === 99, 'int scalar');
assert(glaze_msgpack_decode(glaze_msgpack_encode('world')) === 'world', 'string scalar');
assert(glaze_msgpack_decode(glaze_msgpack_encode(true)) === true, 'bool scalar');
assert(glaze_msgpack_decode(glaze_msgpack_encode(null)) === null, 'null scalar');

echo "PASS\n";
