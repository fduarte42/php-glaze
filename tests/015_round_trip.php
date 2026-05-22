<?php
function round_trip(mixed $value): mixed {
    return glaze_decode(glaze_encode($value));
}

assert(round_trip(null)    === null);
assert(round_trip(true)    === true);
assert(round_trip(false)   === false);
assert(round_trip(0)       === 0);
assert(round_trip(42)      === 42);
assert(round_trip(-7)      === -7);
assert(round_trip(3.14)    === 3.14);
assert(round_trip('hello') === 'hello');
assert(round_trip([])      === []);
assert(round_trip([1,2,3]) === [1,2,3]);

$complex = [
    'id'    => 1,
    'name'  => 'test',
    'tags'  => ['a', 'b'],
    'meta'  => ['ok' => true, 'val' => 0.5],
    'empty' => null,
];
assert(round_trip($complex) === $complex);

echo "PASS\n";
