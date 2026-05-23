---
title: Binary Formats
nav_order: 5
---

# Binary Formats
{: .no_toc }

php-glaze supports three binary serialization formats: BEVE, CBOR, and MessagePack. Binary formats are smaller and faster to parse than text-based formats.

<details open markdown="block">
  <summary>Table of contents</summary>
  {: .text-delta }
1. TOC
{:toc}
</details>

---

## BEVE

BEVE is Glaze's native binary format. It is the fastest and most compact option for round-trips when both endpoints use Glaze (or php-glaze).

### Encoding and Decoding

```php
$data = [
    'name'   => 'Alice',
    'active' => true,
    'scores' => [9, 10, 8],
    'meta'   => ['version' => 1],
];

$binary = glaze_beve_encode($data);
// $binary is a raw binary string — not human-readable

$decoded = glaze_beve_decode($binary);
// $decoded === $data (associative arrays)
```

### When to Use BEVE

- Both producer and consumer use php-glaze or a Glaze-based library
- You need maximum throughput and minimum payload size
- No need for cross-platform binary interoperability (use CBOR instead)

---

## CBOR

CBOR (Concise Binary Object Representation) is defined in [RFC 7049](https://www.rfc-editor.org/rfc/rfc7049). It is broadly supported across languages, platforms, and embedded systems.

### Encoding and Decoding

```php
$data = ['ok' => true, 'count' => 42, 'tags' => ['php', 'binary']];

$cbor = glaze_cbor_encode($data);
// Raw CBOR bytes

$decoded = glaze_cbor_decode($cbor);
var_dump($decoded['ok']);    // bool(true)
echo $decoded['count'];     // 42
```

### When to Use CBOR

- Interoperability with other languages (Python, Go, Rust, JavaScript, etc.)
- IoT / embedded systems that speak CBOR
- You need a standardised binary format (RFC 7049)

---

## MessagePack

MessagePack is a widely adopted binary serialization format used in Redis, Fluentd, and many RPC systems.

### Encoding and Decoding

```php
$data = ['x' => 1, 'y' => 2, 'label' => 'point'];

$mp = glaze_msgpack_encode($data);
// Raw MessagePack bytes

$decoded = glaze_msgpack_decode($mp);
echo $decoded['label'];  // point
```

### Interoperability

MessagePack output from `glaze_msgpack_encode` can be consumed by any standard MessagePack library. For example, with a Python client:

```python
import msgpack
data = msgpack.unpackb(php_output, raw=False)
```

### When to Use MessagePack

- Interoperability with systems that require MessagePack (Redis Streams, Fluentd, RPC frameworks, etc.)
- Cross-language binary payloads with wide ecosystem support

---

## Comparison

| Feature | BEVE | CBOR | MessagePack |
|---------|------|------|-------------|
| Standard | Glaze-specific | RFC 7049 | MessagePack spec |
| Encode | ✓ | ✓ | ✓ |
| Decode | ✓ | ✓ | ✓ |
| Cross-language | Limited | Broad | Broad |
| Best for | Internal Glaze round-trips | Interoperability | Wide ecosystem compatibility |
