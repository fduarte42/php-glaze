---
title: API Reference
nav_order: 3
---

# API Reference
{: .no_toc }

Complete reference for all functions and constants exposed by the php-glaze extension.

<details open markdown="block">
  <summary>Table of contents</summary>
  {: .text-delta }
1. TOC
{:toc}
</details>

---

## Constants

### `GLAZE_PRETTY_PRINT`

```php
const GLAZE_PRETTY_PRINT = 1;
```

Pass as the `$flags` argument to `glaze_encode` to produce indented JSON output.

---

## JSON Functions

### `glaze_encode`

```php
glaze_encode(mixed $value, int $flags = 0): string|false
```

Encodes a PHP value to a JSON string.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `$value` | mixed | The value to encode (null, bool, int, float, string, array, object) |
| `$flags` | int | Optional. Pass `GLAZE_PRETTY_PRINT` for indented output. Default `0`. |

**Returns:** JSON string on success, `false` on failure.

**Example:**

```php
glaze_encode(['a' => 1, 'b' => true]);
// {"a":1,"b":true}

glaze_encode(['a' => 1], GLAZE_PRETTY_PRINT);
// {
//   "a": 1
// }
```

---

### `glaze_decode`

```php
glaze_decode(string $json, bool $assoc = true): mixed
```

Decodes a JSON string into a PHP value.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `$json` | string | A valid JSON string |
| `$assoc` | bool | When `true` (default), objects are returned as associative arrays |

**Returns:** Decoded PHP value, or `false` on parse error.

**Example:**

```php
$data = glaze_decode('{"name":"Alice","active":true}');
echo $data['name'];   // Alice
var_dump($data['active']); // bool(true)
```

---

### `glaze_prettify`

```php
glaze_prettify(string $json): string|false
```

Reformats a compact JSON string with indentation.

**Returns:** Indented JSON string, or `false` if input is invalid JSON.

**Example:**

```php
glaze_prettify('{"a":1,"b":[1,2,3]}');
// {
//   "a": 1,
//   "b": [
//     1,
//     2,
//     3
//   ]
// }
```

---

### `glaze_minify`

```php
glaze_minify(string $json): string|false
```

Strips all unnecessary whitespace from a JSON string.

**Returns:** Compact JSON string, or `false` if input is invalid JSON.

**Example:**

```php
glaze_minify("{\n  \"a\": 1\n}");
// {"a":1}
```

---

### `glaze_validate`

```php
glaze_validate(string $json): bool
```

Checks whether a string is valid JSON without fully decoding it.

**Returns:** `true` if valid JSON, `false` otherwise.

**Example:**

```php
glaze_validate('{"valid":true}');  // true
glaze_validate('{invalid}');       // false
glaze_validate('null');            // true
```

---

## BEVE Functions

BEVE is Glaze's own compact binary format. It is faster and smaller than JSON, CBOR, or MessagePack for round-trips within the Glaze ecosystem.

### `glaze_beve_encode`

```php
glaze_beve_encode(mixed $value): string|false
```

Encodes a PHP value to BEVE binary.

**Returns:** Raw binary string (not human-readable), or `false` on failure.

---

### `glaze_beve_decode`

```php
glaze_beve_decode(string $data, bool $assoc = true): mixed
```

Decodes a BEVE binary string to a PHP value.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `$data` | string | Raw BEVE binary data |
| `$assoc` | bool | When `true` (default), objects are returned as associative arrays |

**Returns:** Decoded PHP value, or `false` on failure.

---

## CBOR Functions

CBOR (Concise Binary Object Representation) is defined in [RFC 7049](https://www.rfc-editor.org/rfc/rfc7049). It is broadly supported across languages and platforms.

### `glaze_cbor_encode`

```php
glaze_cbor_encode(mixed $value): string|false
```

Encodes a PHP value to CBOR binary.

**Returns:** Raw CBOR binary string, or `false` on failure.

---

### `glaze_cbor_decode`

```php
glaze_cbor_decode(string $data, bool $assoc = true): mixed
```

Decodes a CBOR binary string to a PHP value.

**Returns:** Decoded PHP value, or `false` on failure.

---

## MessagePack Functions

### `glaze_msgpack_encode`

```php
glaze_msgpack_encode(mixed $value): string|false
```

Encodes a PHP value to MessagePack binary.

**Returns:** Raw MessagePack binary string, or `false` on failure.

{: .warning }
> MessagePack **decode** is not supported in Glaze v7.7.0 with dynamic types. Use `glaze_cbor_decode` or `glaze_beve_decode` as alternatives if a round-trip is needed within php-glaze.

---

## TOML Functions

TOML (Tom's Obvious, Minimal Language) is a configuration file format designed to be easy to read.

### `glaze_toml_encode`

```php
glaze_toml_encode(mixed $value): string|false
```

Encodes a PHP value to a TOML string.

**Returns:** TOML-formatted string, or `false` on failure.

**Example:**

```php
glaze_toml_encode(['host' => 'localhost', 'port' => 8080, 'debug' => false]);
// host = "localhost"
// port = 8080
// debug = false
```

---

### `glaze_toml_decode`

```php
glaze_toml_decode(string $data, bool $assoc = true): mixed
```

Decodes a TOML string to a PHP value.

**Returns:** Decoded PHP value, or `false` on failure.

---

## YAML Functions

### `glaze_yaml_encode`

```php
glaze_yaml_encode(mixed $value): string|false
```

Encodes a PHP value to a YAML string.

**Returns:** YAML-formatted string, or `false` on failure.

**Example:**

```php
glaze_yaml_encode(['name' => 'Alice', 'scores' => [9, 10, 8]]);
// name: Alice
// scores:
//   - 9
//   - 10
//   - 8
```

---

### `glaze_yaml_decode`

```php
glaze_yaml_decode(string $data, bool $assoc = true): mixed
```

Decodes a YAML string to a PHP value.

**Returns:** Decoded PHP value, or `false` on failure.
