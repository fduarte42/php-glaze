<?php
$compact = '{"a":1,"b":[2,3]}';
$pretty  = glaze_prettify($compact);

assert(is_string($pretty),                   'prettify returns string');
assert(strpos($pretty, "\n") !== false,      'prettified output has newlines');
assert(glaze_decode($compact) === glaze_decode($pretty),
    'prettify preserves value');

// Already-pretty input should round-trip cleanly
$twice = glaze_prettify($pretty);
assert(glaze_decode($twice) === glaze_decode($compact));

// Invalid JSON returns false
$result = @glaze_prettify('not json');
assert($result === false, 'invalid JSON should return false');

echo "PASS\n";
