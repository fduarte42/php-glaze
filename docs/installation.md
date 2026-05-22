---
title: Installation
nav_order: 2
---

# Installation
{: .no_toc }

<details open markdown="block">
  <summary>Table of contents</summary>
  {: .text-delta }
1. TOC
{:toc}
</details>

## Requirements

| Dependency | Minimum Version | Notes |
|------------|-----------------|-------|
| PHP | 7.4 | Development headers required (`php-dev`) |
| CMake | 3.18 | Build system |
| GCC | 13 | C++23 required by Glaze |
| Clang | 18 | Alternative to GCC |
| Internet | — | First build downloads Glaze v7.7.0 via FetchContent |

## Building from Source

### 1. Clone the repository

```bash
git clone https://github.com/fduarte42/php-glaze.git
cd php-glaze
```

### 2. Install system dependencies

**Ubuntu / Debian:**

```bash
sudo apt-get update
sudo apt-get install -y cmake ninja-build build-essential php-dev
```

**macOS (Homebrew):**

```bash
brew install cmake ninja gcc@14 php
export CC=gcc-14 CXX=g++-14
```

### 3. Configure and build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The extension is built at `build/glaze.so`.

{: .note }
> The first build downloads Glaze v7.7.0 automatically. Subsequent builds use the CMake cache — no network access needed.

## Loading the Extension

### One-off (single script)

```bash
php -d extension=build/glaze.so your_script.php
```

### Permanent (php.ini)

Find your `php.ini` location:

```bash
php --ini
```

Add the following line:

```ini
extension=glaze.so
```

Then copy the built extension to your PHP extension directory:

```bash
php -r "echo ini_get('extension_dir');"
# e.g. /usr/lib/php/20230831
sudo cp build/glaze.so /usr/lib/php/20230831/
```

### System-wide install

```bash
sudo cmake --install build
```

This copies `glaze.so` directly to the PHP extension directory detected at build time.

## Verify Installation

```bash
php -d extension=glaze.so -r "var_dump(extension_loaded('glaze'));"
# bool(true)
```

## Running Tests

```bash
bash run_tests.sh build/glaze.so
```

The test suite covers all 14 functions across JSON, BEVE, CBOR, TOML, and YAML.

## CI Compatibility

The extension is tested in CI against:

| OS | PHP Versions |
|----|--------------|
| Ubuntu 24.04 | 7.4, 8.0, 8.1, 8.2, 8.3, 8.4, 8.5 |
| macOS (latest) | 7.4, 8.0, 8.1, 8.2, 8.3, 8.4, 8.5 |
