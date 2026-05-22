---
title: Text Formats
nav_order: 6
---

# Text Formats
{: .no_toc }

php-glaze supports two human-readable text formats beyond JSON: TOML and YAML. Both are commonly used for configuration files.

<details open markdown="block">
  <summary>Table of contents</summary>
  {: .text-delta }
1. TOC
{:toc}
</details>

---

## TOML

TOML (Tom's Obvious, Minimal Language) is a minimal configuration file format that maps unambiguously to a hash table. It is the configuration format used by Cargo (Rust), uv (Python), and many modern tools.

### Encoding

```php
$config = [
    'host'    => 'localhost',
    'port'    => 8080,
    'debug'   => false,
    'timeout' => 30.5,
];

echo glaze_toml_encode($config);
```

Output:

```toml
host = "localhost"
port = 8080
debug = false
timeout = 30.5
```

### Nested Tables

```php
$config = [
    'database' => [
        'host' => 'db.example.com',
        'port' => 5432,
        'name' => 'myapp',
    ],
    'cache' => [
        'ttl'    => 300,
        'driver' => 'redis',
    ],
];

echo glaze_toml_encode($config);
```

Output:

```toml
[database]
host = "db.example.com"
port = 5432
name = "myapp"

[cache]
ttl = 300
driver = "redis"
```

### Decoding

```php
$toml = <<<'TOML'
[server]
host = "0.0.0.0"
port = 9000
workers = 4
TOML;

$config = glaze_toml_decode($toml);
echo $config['server']['host'];    // 0.0.0.0
echo $config['server']['workers']; // 4
```

### Round-trip

```php
$original = ['app' => ['name' => 'myapp', 'version' => '1.0.0']];
$toml      = glaze_toml_encode($original);
$restored  = glaze_toml_decode($toml);

assert($restored['app']['name'] === 'myapp');
```

---

## YAML

YAML (YAML Ain't Markup Language) is a human-readable data serialisation format commonly used for configuration (Kubernetes, GitHub Actions, Ansible, etc.).

### Encoding

```php
$data = [
    'name'   => 'Alice',
    'active' => true,
    'scores' => [9, 10, 8],
];

echo glaze_yaml_encode($data);
```

Output:

```yaml
name: Alice
active: true
scores:
  - 9
  - 10
  - 8
```

### Nested Structures

```php
$manifest = [
    'apiVersion' => 'apps/v1',
    'kind'       => 'Deployment',
    'metadata'   => [
        'name'   => 'my-app',
        'labels' => ['app' => 'my-app'],
    ],
    'spec' => [
        'replicas' => 3,
    ],
];

echo glaze_yaml_encode($manifest);
```

Output:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  labels:
    app: my-app
spec:
  replicas: 3
```

### Decoding

```php
$yaml = <<<'YAML'
server:
  host: 0.0.0.0
  port: 9000
  tls: true
YAML;

$config = glaze_yaml_decode($yaml);
echo $config['server']['host'];    // 0.0.0.0
var_dump($config['server']['tls']); // bool(true)
```

### Round-trip

```php
$original = ['key' => 'value', 'number' => 42, 'flag' => true];
$yaml     = glaze_yaml_encode($original);
$restored = glaze_yaml_decode($yaml);

assert($restored['key'] === 'value');
assert($restored['number'] === 42);
```

---

## Comparison

| Feature | TOML | YAML |
|---------|------|------|
| Encode | ✓ | ✓ |
| Decode | ✓ | ✓ |
| Human-readable | ✓ | ✓ |
| Comments | ✓ | ✓ |
| Best for | Configuration files | Config, manifests, structured data |
| Ecosystem | Cargo, uv, Hugo | Kubernetes, GitHub Actions, Ansible |
