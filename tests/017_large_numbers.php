<?php
// Integer boundaries
assert(glaze_decode('0')  === 0);
assert(glaze_decode('-1') === -1);
assert(glaze_decode('100000') === 100000);

// Floats
assert(glaze_decode('1.5')  === 1.5);
assert(glaze_decode('-1.5') === -1.5);
assert(glaze_decode('1e2')  === 100.0 || glaze_decode('1e2') === 100);
assert(glaze_decode('1.5e3') === 1500.0 || glaze_decode('1.5e3') === 1500);

echo "PASS\n";
