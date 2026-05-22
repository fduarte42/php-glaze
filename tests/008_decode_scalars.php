<?php
assert(glaze_decode('null')  === null);
assert(glaze_decode('true')  === true);
assert(glaze_decode('false') === false);
assert(glaze_decode('42')    === 42);
assert(glaze_decode('3.14')  === 3.14);
assert(glaze_decode('"hello"') === 'hello');
assert(glaze_decode('""')      === '');
echo "PASS\n";
