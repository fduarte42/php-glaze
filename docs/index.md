---
title: Home
nav_order: 1
---

# php-glaze

A PHP extension written in C++ that wraps the [Glaze](https://github.com/stephenberry/glaze) library — one of the fastest JSON libraries available, achieving **~1.4 GB/s** read and write throughput.

## Supported Formats

| Format | Encode | Decode | Notes |
|--------|--------|--------|-------|
| JSON | ✓ | ✓ | Default format |
| BEVE | ✓ | ✓ | Glaze's own compact binary format |
| CBOR | ✓ | ✓ | Concise Binary Object Representation (RFC 7049) |
| MessagePack | ✓ | ✓ | Native codec (Glaze's built-in msgpack unsupported for dynamic types) |
| TOML | ✓ | ✓ | Tom's Obvious, Minimal Language |
| YAML | ✓ | ✓ | YAML Ain't Markup Language |

## Quick Start

```php
<?php

// JSON — encode and decode
$json = glaze_encode(['name' => 'Alice', 'scores' => [9, 10, 8]]);
// {"name":"Alice","scores":[9,10,8]}

$data = glaze_decode($json);
echo $data['name']; // Alice

// Pretty-print
$pretty = glaze_encode(['name' => 'Alice'], GLAZE_PRETTY_PRINT);

// BEVE — compact binary round-trip
$beve = glaze_beve_encode(['key' => 'value', 'n' => 42]);
$data = glaze_beve_decode($beve);
echo $data['key']; // value

// MessagePack — encode and decode
$mp = glaze_msgpack_encode(['x' => 1, 'y' => 2]);
$point = glaze_msgpack_decode($mp);
echo $point['x']; // 1

// TOML — configuration format
$toml = glaze_toml_encode(['host' => 'localhost', 'port' => 8080]);
$config = glaze_toml_decode($toml);

// YAML — human-readable format
$yaml = glaze_yaml_encode(['name' => 'Alice', 'score' => 10]);
$data = glaze_yaml_decode($yaml);
```

## Next Steps

- [Installation](installation) — build requirements and loading the extension
- [API Reference](api-reference) — complete function and constant reference
- [JSON](json) — JSON encoding, decoding, and utilities
- [Binary Formats](binary-formats) — BEVE, CBOR, and MessagePack
- [Text Formats](text-formats) — TOML and YAML
