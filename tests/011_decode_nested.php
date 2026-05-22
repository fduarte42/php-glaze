<?php
$json = '{"users":[{"id":1,"name":"Alice"},{"id":2,"name":"Bob"}],"count":2}';
$data = glaze_decode($json);

assert(is_array($data));
assert($data['count'] === 2);
assert(count($data['users']) === 2);
assert($data['users'][0]['name'] === 'Alice');
assert($data['users'][1]['id']   === 2);

echo "PASS\n";
