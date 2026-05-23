# php-glaze

A PHP extension written in C++ that wraps the [Glaze](https://github.com/stephenberry/glaze) library — one of the fastest JSON libraries available, achieving ~1.4 GB/s read and write throughput.

## Supported Formats

| Format | Encode | Decode | Notes |
|---|---|---|---|
| JSON | ✓ | ✓ | Default format |
| BEVE | ✓ | ✓ | Glaze's own compact binary format |
| CBOR | ✓ | ✓ | Concise Binary Object Representation (RFC 7049) |
| MessagePack | ✓ | ✓ | Native codec (Glaze's built-in msgpack unsupported for dynamic types) |
| TOML | ✓ | ✓ | Tom's Obvious, Minimal Language |
| YAML | ✓ | ✓ | YAML Ain't Markup Language |

## Functions

### JSON

| Function | Signature | Description |
|---|---|---|
| `glaze_encode` | `(mixed $value, int $flags = 0): string\|false` | Encode a PHP value to a JSON string |
| `glaze_decode` | `(string $json, bool $assoc = true): mixed` | Decode a JSON string to a PHP value |
| `glaze_prettify` | `(string $json): string\|false` | Reformat JSON with indentation |
| `glaze_minify` | `(string $json): string\|false` | Strip whitespace from JSON |
| `glaze_validate` | `(string $json): bool` | Check whether a string is valid JSON |

### BEVE

| Function | Signature | Description |
|---|---|---|
| `glaze_beve_encode` | `(mixed $value): string\|false` | Encode to BEVE binary |
| `glaze_beve_decode` | `(string $data, bool $assoc = true): mixed` | Decode from BEVE binary |

### CBOR

| Function | Signature | Description |
|---|---|---|
| `glaze_cbor_encode` | `(mixed $value): string\|false` | Encode to CBOR binary |
| `glaze_cbor_decode` | `(string $data, bool $assoc = true): mixed` | Decode from CBOR binary |

### MessagePack

| Function | Signature | Description |
|---|---|---|
| `glaze_msgpack_encode` | `(mixed $value): string\|false` | Encode to MessagePack binary |
| `glaze_msgpack_decode` | `(string $data, bool $assoc = true): mixed` | Decode from MessagePack binary |

### TOML

| Function | Signature | Description |
|---|---|---|
| `glaze_toml_encode` | `(mixed $value): string\|false` | Encode to TOML text |
| `glaze_toml_decode` | `(string $data, bool $assoc = true): mixed` | Decode from TOML text |

### YAML

| Function | Signature | Description |
|---|---|---|
| `glaze_yaml_encode` | `(mixed $value): string\|false` | Encode to YAML text |
| `glaze_yaml_decode` | `(string $data, bool $assoc = true): mixed` | Decode from YAML text |

### Constants

| Constant | Value | Usage |
|---|---|---|
| `GLAZE_PRETTY_PRINT` | `1` | Pass as `$flags` to `glaze_encode` for indented output |

### Quick example

```php
// JSON — encode and decode
$json = glaze_encode(['name' => 'Alice', 'scores' => [9, 10, 8]]);
// {"name":"Alice","scores":[9,10,8]}

$data = glaze_decode($json);
echo $data['name']; // Alice

// BEVE — compact binary round-trip
$beve = glaze_beve_encode(['key' => 'value', 'n' => 42]);
$data = glaze_beve_decode($beve);
echo $data['key']; // value

// CBOR — binary with RFC 7049 interoperability
$cbor = glaze_cbor_encode(['ok' => true]);
$data = glaze_cbor_decode($cbor);
var_dump($data['ok']); // bool(true)

// MessagePack — encode and decode
$mp = glaze_msgpack_encode(['x' => 1, 'y' => 2]);
$point = glaze_msgpack_decode($mp);
echo $point['x']; // 1

// TOML — configuration format
$toml = glaze_toml_encode(['host' => 'localhost', 'port' => 8080]);
// host = "localhost"
// port = 8080
$config = glaze_toml_decode($toml);

// YAML — human-readable format
$yaml = glaze_yaml_encode(['name' => 'Alice', 'score' => 10]);
// name: Alice
// score: 10
$data = glaze_yaml_decode($yaml);

// Pretty-print / Minify / Validate (JSON)
$pretty   = glaze_prettify('{"a":1,"b":2}');
$minified = glaze_minify($pretty);
glaze_validate('{"valid":true}'); // true
```

## Building

**Requirements:**
- PHP 7.4+ development headers (`php-dev` / `phpize`)
- CMake ≥ 3.18
- GCC ≥ 13 or Clang ≥ 18 (C++23 required by Glaze)
- Internet access during first build (CMake downloads Glaze v7.7.0 via FetchContent)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The extension is built at `build/glaze.so`.

### Loading the extension

```bash
# One-off
php -d extension=build/glaze.so your_script.php

# Permanent — add to php.ini
extension=glaze.so
```

### Install system-wide

```bash
sudo cmake --install build
```

## Running tests

```bash
bash run_tests.sh build/glaze.so
```

## CI

GitHub Actions runs the test suite on PHP 7.4, 8.0, 8.1, 8.2, 8.3, and 8.4 (Ubuntu 24.04) and PHP 8.4 (macOS). See [`.github/workflows/ci.yml`](.github/workflows/ci.yml).

## License

MIT — see [LICENSE](LICENSE).
