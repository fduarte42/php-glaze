<?php
// Default: assoc=true  →  PHP array
$result = glaze_decode('{"name":"Alice","age":30}');
assert(is_array($result),               'should be array');
assert($result['name'] === 'Alice');
assert($result['age']  === 30);

// assoc=false  →  stdClass
$obj = glaze_decode('{"x":1,"y":2}', false);
assert($obj instanceof stdClass,        'should be stdClass');
assert($obj->x === 1);
assert($obj->y === 2);

// Empty object
$r = glaze_decode('{}');
assert(is_array($r) && count($r) === 0, 'empty object → empty array');

echo "PASS\n";
