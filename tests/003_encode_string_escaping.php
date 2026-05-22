<?php
// Quotes and backslashes must be escaped
assert(glaze_encode('say "hi"') === '"say \\"hi\\""');
assert(glaze_encode("line1\nline2") === '"line1\nline2"');
assert(glaze_encode("tab\there")   === '"tab\there"');
assert(glaze_encode('back\\slash') === '"back\\\\slash"');
echo "PASS\n";
