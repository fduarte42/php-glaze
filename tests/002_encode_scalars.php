<?php
assert(glaze_encode(null)  === 'null');
assert(glaze_encode(true)  === 'true');
assert(glaze_encode(false) === 'false');
assert(glaze_encode(42)    === '42');
assert(glaze_encode(3.14)  === '3.14');
assert(glaze_encode('hello') === '"hello"');
assert(glaze_encode('')    === '""');
echo "PASS\n";
