<?php
// Valid JSON
assert(glaze_validate('null')           === true);
assert(glaze_validate('true')           === true);
assert(glaze_validate('42')             === true);
assert(glaze_validate('"hello"')        === true);
assert(glaze_validate('[]')             === true);
assert(glaze_validate('{}')             === true);
assert(glaze_validate('[1,2,3]')        === true);
assert(glaze_validate('{"a":1}')        === true);

// Invalid JSON
assert(glaze_validate('')               === false);
assert(glaze_validate('undefined')      === false);
assert(glaze_validate('{bad}')          === false);
assert(glaze_validate('[1,2,]')         === false);
assert(glaze_validate('{"key":}')       === false);

echo "PASS\n";
