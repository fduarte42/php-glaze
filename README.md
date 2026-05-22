# php-glaze

A PHP extension written in C++ that wraps the [Glaze](https://github.com/stephenberry/glaze) library — one of the fastest JSON libraries available, achieving ~1.4 GB/s read and write throughput.

## Functions

| Function | Signature | Description |
|---|---|---|
| `glaze_encode` | `(mixed $value, int $flags = 0): string\|false` | Encode a PHP value to a JSON string |
| `glaze_decode` | `(string $json, bool $assoc = true): mixed` | Decode a JSON string to a PHP value |
| `glaze_prettify` | `(string $json): string\|false` | Reformat JSON with indentation |
| `glaze_minify` | `(string $json): string\|false` | Strip whitespace from JSON |
| `glaze_validate` | `(string $json): bool` | Check whether a string is valid JSON |

### Constants

| Constant | Value | Usage |
|---|---|---|
| `GLAZE_PRETTY_PRINT` | `1` | Pass as `$flags` to `glaze_encode` for indented output |

### Quick example

```php
// Encode
$json = glaze_encode(['name' => 'Alice', 'scores' => [9, 10, 8]]);
// {"name":"Alice","scores":[9,10,8]}

// Pretty-print
echo glaze_encode(['key' => 'value'], GLAZE_PRETTY_PRINT);
// {
//    "key": "value"
// }

// Decode
$data = glaze_decode('{"ok":true,"n":42}');
var_dump($data['ok'], $data['n']); // bool(true) int(42)

// Decode to stdClass
$obj = glaze_decode('{"x":1}', false);
echo $obj->x; // 1

// Validate
glaze_validate('{"valid":true}'); // true
glaze_validate('{bad}');          // false

// Prettify / Minify
$pretty   = glaze_prettify('{"a":1,"b":2}');
$minified = glaze_minify($pretty);
```

## Building

**Requirements:**
- PHP 8.2+ development headers (`php-dev` / `phpize`)
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

GitHub Actions runs the test suite on PHP 8.2, 8.3, and 8.4 (Ubuntu 24.04) and PHP 8.4 (macOS). See [`.github/workflows/ci.yml`](.github/workflows/ci.yml).

## License

MIT — see [LICENSE](LICENSE).
