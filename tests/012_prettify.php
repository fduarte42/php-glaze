<?php
$compact = '{"a":1,"b":[2,3]}';
$pretty  = glaze_prettify($compact);

assert(is_string($pretty),                   'prettify returns string');
assert(strpos($pretty, "\n") !== false,      'prettified output has newlines');
assert(json_decode($compact, true) === json_decode($pretty, true),
    'prettify preserves value');

// Already-pretty input should round-trip cleanly
$twice = glaze_prettify($pretty);
assert(json_decode($twice, true) === json_decode($compact, true));

// Invalid JSON returns false
$result = @glaze_prettify('not json');
assert($result === false, 'invalid JSON should return false');

echo "PASS\n";
