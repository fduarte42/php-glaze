<?php
$pretty = "{\n  \"a\": 1,\n  \"b\": [2, 3]\n}";
$min    = glaze_minify($pretty);

assert(is_string($min),                     'minify returns string');
assert(strpos($min, "\n") === false,        'minified output has no newlines');
assert(strpos($min, '  ') === false,        'minified output has no extra spaces');
assert(glaze_decode($pretty) === glaze_decode($min),
    'minify preserves value');

// Already-minified input is a no-op
$compact = '{"x":1}';
assert(glaze_minify($compact) === $compact, 'already-minified is unchanged');

// Invalid JSON returns false
$result = @glaze_minify('{bad}');
assert($result === false, 'invalid JSON should return false');

echo "PASS\n";
