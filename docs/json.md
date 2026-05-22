---
title: JSON
nav_order: 4
---

# JSON
{: .no_toc }

<details open markdown="block">
  <summary>Table of contents</summary>
  {: .text-delta }
1. TOC
{:toc}
</details>

JSON is the default format in php-glaze and provides the most complete feature set, including prettify, minify, and validation utilities.

## Encoding

```php
// Basic types
glaze_encode(null);           // null
glaze_encode(true);           // true
glaze_encode(42);             // 42
glaze_encode(3.14);           // 3.14
glaze_encode("hello");        // "hello"

// Arrays
glaze_encode([1, 2, 3]);      // [1,2,3]

// Associative arrays (objects)
glaze_encode(['a' => 1, 'b' => 2]); // {"a":1,"b":2}

// Nested structures
glaze_encode([
    'user' => [
        'name'   => 'Alice',
        'active' => true,
        'scores' => [9, 10, 8],
    ],
]);
// {"user":{"name":"Alice","active":true,"scores":[9,10,8]}}
```

## Pretty-printing

Pass `GLAZE_PRETTY_PRINT` as the second argument to get indented output:

```php
$data = ['user' => ['name' => 'Alice', 'scores' => [9, 10, 8]]];

echo glaze_encode($data, GLAZE_PRETTY_PRINT);
// {
//   "user": {
//     "name": "Alice",
//     "scores": [
//       9,
//       10,
//       8
//     ]
//   }
// }
```

Or use `glaze_prettify` on an existing JSON string:

```php
$compact = '{"a":1,"b":[1,2]}';
echo glaze_prettify($compact);
// {
//   "a": 1,
//   "b": [
//     1,
//     2
//   ]
// }
```

## Decoding

```php
$json = '{"name":"Alice","active":true,"scores":[9,10,8]}';
$data = glaze_decode($json);

echo $data['name'];          // Alice
var_dump($data['active']);   // bool(true)
print_r($data['scores']);    // Array ( [0] => 9 [1] => 10 [2] => 8 )
```

By default, JSON objects are decoded as PHP associative arrays. This matches the behaviour of `json_decode($json, true)`.

## Minifying

Strip whitespace from any JSON string:

```php
$pretty = "{\n  \"a\": 1,\n  \"b\": 2\n}";
echo glaze_minify($pretty);
// {"a":1,"b":2}
```

## Validating

Check JSON validity without fully decoding:

```php
glaze_validate('{"ok":true}');   // true
glaze_validate('{"bad":}');      // false
glaze_validate('null');          // true
glaze_validate('');              // false
```

## Special Values

| PHP value | JSON output |
|-----------|-------------|
| `null` | `null` |
| `true` | `true` |
| `false` | `false` |
| `INF` | not supported — avoid |
| `NAN` | not supported — avoid |
| Integer arrays | JSON arrays `[...]` |
| Associative arrays | JSON objects `{...}` |

## Error Handling

`glaze_encode` and `glaze_decode` return `false` on failure. `glaze_prettify` and `glaze_minify` return `false` if the input is not valid JSON.

```php
$result = glaze_decode('{not valid}');
if ($result === false) {
    // handle parse error
}
```
