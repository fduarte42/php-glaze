<?php
assert(function_exists('glaze_msgpack_encode'), 'glaze_msgpack_encode exists');

$data = ['key' => 'value', 'n' => 42, 'flag' => true];

$mp = glaze_msgpack_encode($data);
assert(is_string($mp), 'encode returns string');
assert(strlen($mp) > 0, 'encode returns non-empty binary');

// Encode different types
assert(is_string(glaze_msgpack_encode(null)), 'null encodes');
assert(is_string(glaze_msgpack_encode(true)), 'bool encodes');
assert(is_string(glaze_msgpack_encode(42)), 'int encodes');
assert(is_string(glaze_msgpack_encode('hello')), 'string encodes');
assert(is_string(glaze_msgpack_encode([1, 2, 3])), 'array encodes');

// Different calls produce the same bytes for same input
assert(glaze_msgpack_encode($data) === glaze_msgpack_encode($data), 'deterministic');

echo "PASS\n";
