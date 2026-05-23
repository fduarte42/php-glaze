// PHP extension wrapping the Glaze library (https://github.com/stephenberry/glaze)
// Exposed functions: glaze_encode, glaze_decode, glaze_prettify, glaze_minify,
//   glaze_validate, glaze_beve_encode, glaze_beve_decode, glaze_cbor_encode,
//   glaze_cbor_decode, glaze_msgpack_encode, glaze_msgpack_decode,
//   glaze_toml_encode, glaze_toml_decode, glaze_yaml_encode, glaze_yaml_decode
// Supports PHP 7.4 through 8.x.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
#include "ext/standard/info.h"
}

#include "glaze/json.hpp"
#include "glaze/beve.hpp"
#include "glaze/cbor.hpp"
#include "glaze/toml.hpp"
#include "glaze/yaml.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <variant>

#define PHP_GLAZE_VERSION  "1.1.0"
#define PHP_GLAZE_EXTNAME  "glaze"
#define GLAZE_LIB_VERSION  "7.7.0"
#define GLAZE_FLAG_PRETTY_PRINT 1

// ============================================================================
// Argument info
//
// ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX and ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE
// were introduced in PHP 8.0.  Use the simpler ZEND_BEGIN_ARG_INFO_EX form on
// older versions so the same source compiles against all supported headers.
// ============================================================================

#if PHP_VERSION_ID >= 80000

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_glaze_encode, 0, 1, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_INFO(0, value)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_glaze_decode, 0, 1, MAY_BE_ANY)
    ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, assoc, _IS_BOOL, 0, "true")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_glaze_prettify, 0, 1, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_glaze_minify, 0, 1, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_glaze_validate, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
ZEND_END_ARG_INFO()

// Shared arginfo for format-specific encode: (mixed $value): string|false
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_glaze_format_encode, 0, 1, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

// Shared arginfo for format-specific decode: (string $data, bool $assoc = true): mixed
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_glaze_format_decode, 0, 1, MAY_BE_ANY)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, assoc, _IS_BOOL, 0, "true")
ZEND_END_ARG_INFO()

#else  // PHP 7.x — typed-arginfo macros not available

ZEND_BEGIN_ARG_INFO_EX(arginfo_glaze_encode, 0, 0, 1)
    ZEND_ARG_INFO(0, value)
    ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_glaze_decode, 0, 0, 1)
    ZEND_ARG_INFO(0, json)
    ZEND_ARG_INFO(0, assoc)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_glaze_prettify, 0, 0, 1)
    ZEND_ARG_INFO(0, json)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_glaze_minify, 0, 0, 1)
    ZEND_ARG_INFO(0, json)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_glaze_validate, 0, 0, 1)
    ZEND_ARG_INFO(0, json)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_glaze_format_encode, 0, 0, 1)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_glaze_format_decode, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, assoc)
ZEND_END_ARG_INFO()

#endif  // PHP_VERSION_ID

// ============================================================================
// zend_array_is_list polyfill
//
// Added in PHP 8.1.  On earlier versions we replicate the logic: an array is
// a list iff it has consecutive integer keys 0, 1, 2, … n-1.
// ============================================================================

#if PHP_VERSION_ID < 80100
static bool glaze_is_list(zend_array *ht)
{
    if (zend_hash_num_elements(ht) == 0) return true;
    zend_ulong expected = 0;
    zend_string *key;
    zend_ulong   num_key;
    ZEND_HASH_FOREACH_KEY(ht, num_key, key) {
        if (key != nullptr || num_key != expected++) return false;
    } ZEND_HASH_FOREACH_END();
    return true;
}
#else
static inline bool glaze_is_list(zend_array *ht) {
    return zend_array_is_list(ht) != 0;
}
#endif

// ============================================================================
// Object property access helpers
//
// PHP 8.0 introduced zend_get_properties_for() / zend_release_properties() and
// ZEND_PROP_PURPOSE_JSON.  On PHP 7.x we fall back to the get_properties
// handler (which takes zval* in 7.x vs zend_object* in 8.x).
// ============================================================================

struct PropsGuard {
    HashTable *ht;
    bool       needs_release;

    ~PropsGuard() {
#if PHP_VERSION_ID >= 80000
        if (needs_release && ht) zend_release_properties(ht);
#else
        (void)needs_release;
#endif
    }
};

static PropsGuard get_object_props(zval *val)
{
#if PHP_VERSION_ID >= 80000
    HashTable *ht = zend_get_properties_for(val, ZEND_PROP_PURPOSE_JSON);
    return {ht, true};
#else
    auto *handlers = Z_OBJ_P(val)->handlers;
    HashTable *ht = (handlers && handlers->get_properties)
        ? handlers->get_properties(val)
        : nullptr;
    return {ht, false};
#endif
}

// ============================================================================
// Encode path: PHP value → glz::generic (direct, no JSON round-trip)
//
// glz::generic_json<f64> has direct operator= overloads for nullptr_t, bool,
// double, std::string, array_t, and object_t that assign to the internal
// variant without any JSON serialization.  We build the generic tree in one
// pass and let Glaze's optimised writer produce the output bytes.
// ============================================================================

static void build_generic(zval *val, glz::generic &g, int depth)
{
    if (depth > 512) {
        g = nullptr;
        return;
    }

    ZVAL_DEREF(val);

    switch (Z_TYPE_P(val)) {

        case IS_NULL:
            g = nullptr;
            return;

        case IS_TRUE:
            g = true;
            return;

        case IS_FALSE:
            g = false;
            return;

        case IS_LONG:
            g = static_cast<double>(Z_LVAL_P(val));
            return;

        case IS_DOUBLE:
            g = Z_DVAL_P(val);
            return;

        case IS_STRING:
            g = std::string(Z_STRVAL_P(val), Z_STRLEN_P(val));
            return;

        case IS_ARRAY: {
            zend_array *ht = Z_ARRVAL_P(val);

            if (glaze_is_list(ht)) {
                glz::generic::array_t arr;
                arr.reserve(zend_hash_num_elements(ht));
                zval *entry;
                ZEND_HASH_FOREACH_VAL(ht, entry) {
                    glz::generic elem{};
                    build_generic(entry, elem, depth + 1);
                    arr.push_back(std::move(elem));
                } ZEND_HASH_FOREACH_END();
                g.data = std::move(arr);
            } else {
                glz::generic::object_t obj;
                zend_string *str_key;
                zend_ulong   num_key;
                zval        *entry;
                ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, str_key, entry) {
                    std::string key = str_key
                        ? std::string(ZSTR_VAL(str_key), ZSTR_LEN(str_key))
                        : std::to_string(num_key);
                    glz::generic elem{};
                    build_generic(entry, elem, depth + 1);
                    obj.try_emplace(std::move(key), std::move(elem));
                } ZEND_HASH_FOREACH_END();
                g.data = std::move(obj);
            }
            return;
        }

        case IS_OBJECT: {
            glz::generic::object_t obj;
            auto pg = get_object_props(val);
            if (pg.ht) {
                zend_string *str_key;
                zend_ulong   num_key;
                zval        *entry;
                ZEND_HASH_FOREACH_KEY_VAL(pg.ht, num_key, str_key, entry) {
                    if (Z_TYPE_P(entry) == IS_UNDEF) continue;
                    std::string key = str_key
                        ? std::string(ZSTR_VAL(str_key), ZSTR_LEN(str_key))
                        : std::to_string(num_key);
                    glz::generic elem{};
                    build_generic(entry, elem, depth + 1);
                    obj.try_emplace(std::move(key), std::move(elem));
                } ZEND_HASH_FOREACH_END();
            }
            g.data = std::move(obj);
            return;
        }

        default:
            g = nullptr;
    }
}

// ============================================================================
// Decode path: glz::generic → PHP value
// ============================================================================

static void generic_to_zval(const glz::generic &j, zval *result, bool assoc)
{
    std::visit([&](const auto &val) {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            ZVAL_NULL(result);

        } else if constexpr (std::is_same_v<T, bool>) {
            ZVAL_BOOL(result, val);

        } else if constexpr (std::is_same_v<T, double>) {
            const double d = val;
            if (std::isfinite(d) && d == std::floor(d)
                && d >= static_cast<double>(ZEND_LONG_MIN)
                && d <= static_cast<double>(ZEND_LONG_MAX))
            {
                ZVAL_LONG(result, static_cast<zend_long>(d));
            } else {
                ZVAL_DOUBLE(result, d);
            }

        } else if constexpr (std::is_same_v<T, std::string>) {
            ZVAL_STRINGL(result, val.c_str(), val.size());

        } else if constexpr (std::is_same_v<T, glz::generic::array_t>) {
            array_init_size(result, static_cast<uint32_t>(val.size()));
            for (const auto &item : val) {
                zval elem;
                ZVAL_UNDEF(&elem);
                generic_to_zval(item, &elem, assoc);
                add_next_index_zval(result, &elem);
            }

        } else if constexpr (std::is_same_v<T, glz::generic::object_t>) {
            if (assoc) {
                array_init_size(result, static_cast<uint32_t>(val.size()));
                for (const auto &[key, jval] : val) {
                    zval elem;
                    ZVAL_UNDEF(&elem);
                    generic_to_zval(jval, &elem, assoc);
                    add_assoc_zval_ex(result, key.c_str(), key.size(), &elem);
                }
            } else {
                object_init(result);
                for (const auto &[key, jval] : val) {
                    zval elem;
                    ZVAL_UNDEF(&elem);
                    generic_to_zval(jval, &elem, assoc);
                    add_property_zval_ex(result, key.c_str(), key.size(), &elem);
                    zval_ptr_dtor(&elem);
                }
            }
        }
    }, j.data);
}

// ============================================================================
// php_to_generic: PHP value → glz::generic
//
// Used as the encode bridge for all non-JSON binary/text formats.
// ============================================================================

static void php_to_generic(zval *value, glz::generic &g)
{
    build_generic(value, g, 0);
}

// ============================================================================
// glaze_encode(mixed $value, int $flags = 0): string|false
// ============================================================================

PHP_FUNCTION(glaze_encode)
{
    zval      *value;
    zend_long  flags = 0;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(value)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
    ZEND_PARSE_PARAMETERS_END();

    glz::generic g{};
    build_generic(value, g, 0);

    std::string buf;
    glz::error_ctx ec;
    if (flags & GLAZE_FLAG_PRETTY_PRINT) {
        ec = glz::write<glz::opts{.prettify = true}>(g, buf);
    } else {
        ec = glz::write_json(g, buf);
    }
    if (ec) { RETURN_FALSE; }

    RETURN_STRINGL(buf.c_str(), buf.size());
}

// ============================================================================
// glaze_decode(string $json, bool $assoc = true): mixed
// ============================================================================

PHP_FUNCTION(glaze_decode)
{
    zend_string *json_str;
    zend_bool    assoc = 1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(json_str)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(assoc)
    ZEND_PARSE_PARAMETERS_END();

    std::string input(ZSTR_VAL(json_str), ZSTR_LEN(json_str));

    try {
        glz::generic parsed{};
        auto ec = glz::read_json(parsed, input);
        if (ec) {
            php_error_docref(nullptr, E_WARNING, "glaze_decode: %s",
                glz::format_error(ec, input).c_str());
            RETURN_NULL();
        }
        generic_to_zval(parsed, return_value, static_cast<bool>(assoc));
    } catch (const std::exception &e) {
        php_error_docref(nullptr, E_WARNING, "glaze_decode: %s", e.what());
        RETURN_NULL();
    }
}

// ============================================================================
// glaze_prettify(string $json): string|false
// ============================================================================

PHP_FUNCTION(glaze_prettify)
{
    zend_string *json_str;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(json_str)
    ZEND_PARSE_PARAMETERS_END();

    std::string input(ZSTR_VAL(json_str), ZSTR_LEN(json_str));

    try {
        glz::generic parsed{};
        auto ec = glz::read_json(parsed, input);
        if (ec) {
            php_error_docref(nullptr, E_WARNING, "glaze_prettify: %s",
                glz::format_error(ec, input).c_str());
            RETURN_FALSE;
        }

        std::string buf;
        auto ec2 = glz::write<glz::opts{.prettify = true}>(parsed, buf);
        if (ec2) { RETURN_FALSE; }
        RETURN_STRINGL(buf.c_str(), buf.size());
    } catch (const std::exception &e) {
        php_error_docref(nullptr, E_WARNING, "glaze_prettify: %s", e.what());
        RETURN_FALSE;
    }
}

// ============================================================================
// glaze_minify(string $json): string|false
// ============================================================================

PHP_FUNCTION(glaze_minify)
{
    zend_string *json_str;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(json_str)
    ZEND_PARSE_PARAMETERS_END();

    std::string input(ZSTR_VAL(json_str), ZSTR_LEN(json_str));

    try {
        glz::generic parsed{};
        auto ec = glz::read_json(parsed, input);
        if (ec) {
            php_error_docref(nullptr, E_WARNING, "glaze_minify: %s",
                glz::format_error(ec, input).c_str());
            RETURN_FALSE;
        }

        std::string buf;
        auto ec2 = glz::write_json(parsed, buf);
        if (ec2) { RETURN_FALSE; }
        RETURN_STRINGL(buf.c_str(), buf.size());
    } catch (const std::exception &e) {
        php_error_docref(nullptr, E_WARNING, "glaze_minify: %s", e.what());
        RETURN_FALSE;
    }
}

// ============================================================================
// glaze_validate(string $json): bool
// ============================================================================

PHP_FUNCTION(glaze_validate)
{
    zend_string *json_str;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(json_str)
    ZEND_PARSE_PARAMETERS_END();

    std::string input(ZSTR_VAL(json_str), ZSTR_LEN(json_str));
    glz::generic parsed{};
    auto ec = glz::read_json(parsed, input);
    RETURN_BOOL(!ec);
}

// ============================================================================
// BEVE — Glaze's own compact binary format
// glaze_beve_encode(mixed $value): string|false
// glaze_beve_decode(string $data, bool $assoc = true): mixed
// ============================================================================

PHP_FUNCTION(glaze_beve_encode)
{
    zval *value;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    glz::generic g{};
    php_to_generic(value, g);

    std::string buf;
    auto ec = glz::write_beve(g, buf);
    if (ec) { RETURN_FALSE; }
    RETURN_STRINGL(buf.c_str(), buf.size());
}

PHP_FUNCTION(glaze_beve_decode)
{
    zend_string *data;
    zend_bool    assoc = 1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(data)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(assoc)
    ZEND_PARSE_PARAMETERS_END();

    std::string buf(ZSTR_VAL(data), ZSTR_LEN(data));
    glz::generic g{};
    auto ec = glz::read_beve(g, buf);
    if (ec) {
        php_error_docref(nullptr, E_WARNING, "glaze_beve_decode: parse error");
        RETURN_NULL();
    }
    generic_to_zval(g, return_value, static_cast<bool>(assoc));
}

// ============================================================================
// CBOR — Concise Binary Object Representation (RFC 7049)
// glaze_cbor_encode(mixed $value): string|false
// glaze_cbor_decode(string $data, bool $assoc = true): mixed
// ============================================================================

PHP_FUNCTION(glaze_cbor_encode)
{
    zval *value;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    glz::generic g{};
    php_to_generic(value, g);

    std::string buf;
    auto ec = glz::write_cbor(g, buf);
    if (ec) { RETURN_FALSE; }
    RETURN_STRINGL(buf.c_str(), buf.size());
}

PHP_FUNCTION(glaze_cbor_decode)
{
    zend_string *data;
    zend_bool    assoc = 1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(data)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(assoc)
    ZEND_PARSE_PARAMETERS_END();

    std::string buf(ZSTR_VAL(data), ZSTR_LEN(data));
    glz::generic g{};
    auto ec = glz::read_cbor(g, buf);
    if (ec) {
        php_error_docref(nullptr, E_WARNING, "glaze_cbor_decode: parse error");
        RETURN_NULL();
    }
    generic_to_zval(g, return_value, static_cast<bool>(assoc));
}

// ============================================================================
// MessagePack — native codec
//
// glz::write_msgpack / glz::read_msgpack cannot be used with glz::generic:
// Glaze serialises the variant as a tagged array [type_name, value] rather
// than native msgpack type bytes, making the output non-standard and
// glz::read_msgpack failing to compile for the complex nested types inside
// the variant.  We implement a compact native msgpack encoder/decoder
// directly, bypassing glz::generic for the msgpack path entirely.
// ============================================================================

// ---- native encode helpers ----

static void mp_write_uint(std::string &o, uint64_t v)
{
    if (v <= 0x7f)      { o += static_cast<char>(v); }
    else if (v <= 0xff) { o += '\xcc'; o += static_cast<char>(v); }
    else if (v <= 0xffff) {
        o += '\xcd';
        o += static_cast<char>(v >> 8); o += static_cast<char>(v);
    } else if (v <= 0xffffffff) {
        o += '\xce';
        for (int s = 24; s >= 0; s -= 8) o += static_cast<char>(v >> s);
    } else {
        o += '\xcf';
        for (int s = 56; s >= 0; s -= 8) o += static_cast<char>(v >> s);
    }
}

static void mp_write_int(std::string &o, int64_t v)
{
    if (v >= 0)         { mp_write_uint(o, static_cast<uint64_t>(v)); return; }
    if (v >= -32)       { o += static_cast<char>(v & 0xff); return; }
    if (v >= -128)      { o += '\xd0'; o += static_cast<char>(v); return; }
    if (v >= -32768) {
        o += '\xd1';
        o += static_cast<char>(v >> 8); o += static_cast<char>(v);
        return;
    }
    if (v >= -2147483648LL) {
        o += '\xd2';
        for (int s = 24; s >= 0; s -= 8) o += static_cast<char>(v >> s);
        return;
    }
    o += '\xd3';
    for (int s = 56; s >= 0; s -= 8) o += static_cast<char>(v >> s);
}

static void mp_write_str(std::string &o, const char *s, size_t n)
{
    if (n <= 31) {
        o += static_cast<char>(0xa0 | n);
    } else if (n <= 0xff) {
        o += '\xd9'; o += static_cast<char>(n);
    } else if (n <= 0xffff) {
        o += '\xda'; o += static_cast<char>(n >> 8); o += static_cast<char>(n);
    } else {
        o += '\xdb';
        for (int s = 24; s >= 0; s -= 8) o += static_cast<char>(n >> s);
    }
    o.append(s, n);
}

static void mp_write_array_hdr(std::string &o, uint32_t n)
{
    if (n <= 15)        { o += static_cast<char>(0x90 | n); }
    else if (n <= 0xffff) {
        o += '\xdc'; o += static_cast<char>(n >> 8); o += static_cast<char>(n);
    } else {
        o += '\xdd';
        for (int s = 24; s >= 0; s -= 8) o += static_cast<char>(n >> s);
    }
}

static void mp_write_map_hdr(std::string &o, uint32_t n)
{
    if (n <= 15)        { o += static_cast<char>(0x80 | n); }
    else if (n <= 0xffff) {
        o += '\xde'; o += static_cast<char>(n >> 8); o += static_cast<char>(n);
    } else {
        o += '\xdf';
        for (int s = 24; s >= 0; s -= 8) o += static_cast<char>(n >> s);
    }
}

static void msgpack_encode_val(zval *val, std::string &out, int depth)
{
    if (depth > 512) { out += '\xc0'; return; }
    ZVAL_DEREF(val);
    switch (Z_TYPE_P(val)) {
        case IS_NULL:  out += '\xc0'; return;
        case IS_TRUE:  out += '\xc3'; return;
        case IS_FALSE: out += '\xc2'; return;
        case IS_LONG:  mp_write_int(out, Z_LVAL_P(val)); return;
        case IS_DOUBLE: {
            double d = Z_DVAL_P(val);
            out += '\xcb';
            uint64_t bits; std::memcpy(&bits, &d, 8);
            for (int s = 56; s >= 0; s -= 8) out += static_cast<char>(bits >> s);
            return;
        }
        case IS_STRING:
            mp_write_str(out, Z_STRVAL_P(val), Z_STRLEN_P(val));
            return;
        case IS_ARRAY: {
            zend_array *ht = Z_ARRVAL_P(val);
            uint32_t n = zend_hash_num_elements(ht);
            if (glaze_is_list(ht)) {
                mp_write_array_hdr(out, n);
                zval *entry;
                ZEND_HASH_FOREACH_VAL(ht, entry) {
                    msgpack_encode_val(entry, out, depth + 1);
                } ZEND_HASH_FOREACH_END();
            } else {
                mp_write_map_hdr(out, n);
                zend_string *str_key; zend_ulong num_key; zval *entry;
                ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, str_key, entry) {
                    if (str_key) mp_write_str(out, ZSTR_VAL(str_key), ZSTR_LEN(str_key));
                    else { auto ks = std::to_string(num_key); mp_write_str(out, ks.c_str(), ks.size()); }
                    msgpack_encode_val(entry, out, depth + 1);
                } ZEND_HASH_FOREACH_END();
            }
            return;
        }
        case IS_OBJECT: {
            auto pg = get_object_props(val);
            uint32_t n = 0;
            if (pg.ht) {
                zval *entry;
                ZEND_HASH_FOREACH_VAL(pg.ht, entry) {
                    if (Z_TYPE_P(entry) != IS_UNDEF) ++n;
                } ZEND_HASH_FOREACH_END();
            }
            mp_write_map_hdr(out, n);
            if (pg.ht) {
                zend_string *str_key; zend_ulong num_key; zval *entry;
                ZEND_HASH_FOREACH_KEY_VAL(pg.ht, num_key, str_key, entry) {
                    if (Z_TYPE_P(entry) == IS_UNDEF) continue;
                    if (str_key) mp_write_str(out, ZSTR_VAL(str_key), ZSTR_LEN(str_key));
                    else { auto ks = std::to_string(num_key); mp_write_str(out, ks.c_str(), ks.size()); }
                    msgpack_encode_val(entry, out, depth + 1);
                } ZEND_HASH_FOREACH_END();
            }
            return;
        }
        default: out += '\xc0';
    }
}

// ---- native decode helpers ----

static bool msgpack_decode_val(const uint8_t *&p, const uint8_t *end,
                               zval *result, bool assoc, int depth)
{
    if (p >= end || depth > 512) { ZVAL_NULL(result); return p < end || depth <= 512; }
    uint8_t b = *p++;

    // nil / bool
    if (b == 0xc0) { ZVAL_NULL(result); return true; }
    if (b == 0xc2) { ZVAL_FALSE(result); return true; }
    if (b == 0xc3) { ZVAL_TRUE(result);  return true; }

    // positive fixint
    if (b <= 0x7f) { ZVAL_LONG(result, b); return true; }
    // negative fixint
    if (b >= 0xe0) { ZVAL_LONG(result, static_cast<int8_t>(b)); return true; }

    auto need = [&](size_t n) { return static_cast<size_t>(end - p) >= n; };
    auto read_be = [&](int n) -> uint64_t {
        uint64_t v = 0;
        for (int i = 0; i < n; ++i) v = (v << 8) | *p++;
        return v;
    };

    // uint
    if (b == 0xcc) { if (!need(1)) return false; ZVAL_LONG(result, read_be(1)); return true; }
    if (b == 0xcd) { if (!need(2)) return false; ZVAL_LONG(result, read_be(2)); return true; }
    if (b == 0xce) { if (!need(4)) return false; ZVAL_LONG(result, (zend_long)read_be(4)); return true; }
    if (b == 0xcf) {
        if (!need(8)) return false;
        uint64_t v = read_be(8);
        if (v <= (uint64_t)ZEND_LONG_MAX) ZVAL_LONG(result, (zend_long)v);
        else ZVAL_DOUBLE(result, (double)v);
        return true;
    }
    // int
    if (b == 0xd0) { if (!need(1)) return false; ZVAL_LONG(result, (int8_t)read_be(1)); return true; }
    if (b == 0xd1) { if (!need(2)) return false; ZVAL_LONG(result, (int16_t)read_be(2)); return true; }
    if (b == 0xd2) { if (!need(4)) return false; ZVAL_LONG(result, (int32_t)read_be(4)); return true; }
    if (b == 0xd3) { if (!need(8)) return false; ZVAL_LONG(result, (int64_t)read_be(8)); return true; }
    // float
    if (b == 0xca) {
        if (!need(4)) return false;
        uint32_t bits = (uint32_t)read_be(4); float f; std::memcpy(&f, &bits, 4);
        ZVAL_DOUBLE(result, f); return true;
    }
    if (b == 0xcb) {
        if (!need(8)) return false;
        uint64_t bits = read_be(8); double d; std::memcpy(&d, &bits, 8);
        ZVAL_DOUBLE(result, d); return true;
    }
    // str
    auto do_str = [&](size_t n) -> bool {
        if (!need(n)) return false;
        ZVAL_STRINGL(result, reinterpret_cast<const char *>(p), n); p += n; return true;
    };
    if ((b & 0xe0) == 0xa0) return do_str(b & 0x1f);
    if (b == 0xd9) { if (!need(1)) return false; return do_str((size_t)read_be(1)); }
    if (b == 0xda) { if (!need(2)) return false; return do_str((size_t)read_be(2)); }
    if (b == 0xdb) { if (!need(4)) return false; return do_str((size_t)read_be(4)); }
    // bin — treat as string
    if (b == 0xc4) { if (!need(1)) return false; return do_str((size_t)read_be(1)); }
    if (b == 0xc5) { if (!need(2)) return false; return do_str((size_t)read_be(2)); }
    if (b == 0xc6) { if (!need(4)) return false; return do_str((size_t)read_be(4)); }
    // array
    auto do_array = [&](uint32_t n) -> bool {
        array_init_size(result, n);
        for (uint32_t i = 0; i < n; ++i) {
            zval elem; ZVAL_UNDEF(&elem);
            if (!msgpack_decode_val(p, end, &elem, assoc, depth + 1)) return false;
            add_next_index_zval(result, &elem);
        }
        return true;
    };
    if ((b & 0xf0) == 0x90) return do_array(b & 0x0f);
    if (b == 0xdc) { if (!need(2)) return false; return do_array((uint32_t)read_be(2)); }
    if (b == 0xdd) { if (!need(4)) return false; return do_array((uint32_t)read_be(4)); }
    // map
    auto do_map = [&](uint32_t n) -> bool {
        if (assoc) array_init_size(result, n); else object_init(result);
        for (uint32_t i = 0; i < n; ++i) {
            zval kv; ZVAL_UNDEF(&kv);
            if (!msgpack_decode_val(p, end, &kv, assoc, depth + 1)) return false;
            // key must be a string or integer
            zval vv; ZVAL_UNDEF(&vv);
            if (!msgpack_decode_val(p, end, &vv, assoc, depth + 1)) {
                zval_ptr_dtor(&kv); return false;
            }
            if (Z_TYPE(kv) == IS_STRING) {
                if (assoc) add_assoc_zval_ex(result, Z_STRVAL(kv), Z_STRLEN(kv), &vv);
                else       { add_property_zval_ex(result, Z_STRVAL(kv), Z_STRLEN(kv), &vv); zval_ptr_dtor(&vv); }
            } else {
                // numeric key: coerce to string for assoc, skip for object
                zval str_key; ZVAL_UNDEF(&str_key);
                convert_to_string_ex(&kv);
                if (assoc) add_assoc_zval_ex(result, Z_STRVAL(kv), Z_STRLEN(kv), &vv);
                else       { add_property_zval_ex(result, Z_STRVAL(kv), Z_STRLEN(kv), &vv); zval_ptr_dtor(&vv); }
                (void)str_key;
            }
            zval_ptr_dtor(&kv);
        }
        return true;
    };
    if ((b & 0xf0) == 0x80) return do_map(b & 0x0f);
    if (b == 0xde) { if (!need(2)) return false; return do_map((uint32_t)read_be(2)); }
    if (b == 0xdf) { if (!need(4)) return false; return do_map((uint32_t)read_be(4)); }

    // ext / unknown — skip and return null
    ZVAL_NULL(result);
    return true;
}

PHP_FUNCTION(glaze_msgpack_encode)
{
    zval *value;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    std::string buf;
    msgpack_encode_val(value, buf, 0);
    RETURN_STRINGL(buf.c_str(), buf.size());
}

PHP_FUNCTION(glaze_msgpack_decode)
{
    zend_string *data;
    zend_bool    assoc = 1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(data)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(assoc)
    ZEND_PARSE_PARAMETERS_END();

    const uint8_t *p   = reinterpret_cast<const uint8_t *>(ZSTR_VAL(data));
    const uint8_t *end = p + ZSTR_LEN(data);
    if (!msgpack_decode_val(p, end, return_value, static_cast<bool>(assoc), 0)) {
        php_error_docref(nullptr, E_WARNING, "glaze_msgpack_decode: malformed msgpack");
        ZVAL_NULL(return_value);
    }
}

// ============================================================================
// TOML — Tom's Obvious, Minimal Language
// glaze_toml_encode(mixed $value): string|false
// glaze_toml_decode(string $data, bool $assoc = true): mixed
// ============================================================================

PHP_FUNCTION(glaze_toml_encode)
{
    zval *value;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    glz::generic g{};
    php_to_generic(value, g);

    std::string buf;
    auto ec = glz::write_toml(g, buf);
    if (ec) { RETURN_FALSE; }
    RETURN_STRINGL(buf.c_str(), buf.size());
}

PHP_FUNCTION(glaze_toml_decode)
{
    zend_string *data;
    zend_bool    assoc = 1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(data)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(assoc)
    ZEND_PARSE_PARAMETERS_END();

    std::string buf(ZSTR_VAL(data), ZSTR_LEN(data));
    glz::generic g{};
    auto ec = glz::read_toml(g, buf);
    if (ec) {
        php_error_docref(nullptr, E_WARNING, "glaze_toml_decode: parse error");
        RETURN_NULL();
    }
    generic_to_zval(g, return_value, static_cast<bool>(assoc));
}

// ============================================================================
// YAML — YAML Ain't Markup Language
// glaze_yaml_encode(mixed $value): string|false
// glaze_yaml_decode(string $data, bool $assoc = true): mixed
// ============================================================================

PHP_FUNCTION(glaze_yaml_encode)
{
    zval *value;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    glz::generic g{};
    php_to_generic(value, g);

    std::string buf;
    auto ec = glz::write_yaml(g, buf);
    if (ec) { RETURN_FALSE; }
    RETURN_STRINGL(buf.c_str(), buf.size());
}

PHP_FUNCTION(glaze_yaml_decode)
{
    zend_string *data;
    zend_bool    assoc = 1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(data)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(assoc)
    ZEND_PARSE_PARAMETERS_END();

    std::string buf(ZSTR_VAL(data), ZSTR_LEN(data));
    glz::generic g{};
    auto ec = glz::read_yaml(g, buf);
    if (ec) {
        php_error_docref(nullptr, E_WARNING, "glaze_yaml_decode: parse error");
        RETURN_NULL();
    }
    generic_to_zval(g, return_value, static_cast<bool>(assoc));
}

// ============================================================================
// Module boilerplate
// ============================================================================

static const zend_function_entry glaze_functions[] = {
    // JSON
    PHP_FE(glaze_encode,        arginfo_glaze_encode)
    PHP_FE(glaze_decode,        arginfo_glaze_decode)
    PHP_FE(glaze_prettify,      arginfo_glaze_prettify)
    PHP_FE(glaze_minify,        arginfo_glaze_minify)
    PHP_FE(glaze_validate,      arginfo_glaze_validate)
    // BEVE
    PHP_FE(glaze_beve_encode,   arginfo_glaze_format_encode)
    PHP_FE(glaze_beve_decode,   arginfo_glaze_format_decode)
    // CBOR
    PHP_FE(glaze_cbor_encode,   arginfo_glaze_format_encode)
    PHP_FE(glaze_cbor_decode,   arginfo_glaze_format_decode)
    // MessagePack
    PHP_FE(glaze_msgpack_encode, arginfo_glaze_format_encode)
    PHP_FE(glaze_msgpack_decode, arginfo_glaze_format_decode)
    // TOML
    PHP_FE(glaze_toml_encode,   arginfo_glaze_format_encode)
    PHP_FE(glaze_toml_decode,   arginfo_glaze_format_decode)
    // YAML
    PHP_FE(glaze_yaml_encode,   arginfo_glaze_format_encode)
    PHP_FE(glaze_yaml_decode,   arginfo_glaze_format_decode)
    PHP_FE_END
};

PHP_MINIT_FUNCTION(glaze)
{
    REGISTER_LONG_CONSTANT("GLAZE_PRETTY_PRINT", GLAZE_FLAG_PRETTY_PRINT,
                           CONST_CS | CONST_PERSISTENT);
    return SUCCESS;
}

PHP_MINFO_FUNCTION(glaze)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "glaze support",         "enabled");
    php_info_print_table_row(2,    "extension version",     PHP_GLAZE_VERSION);
    php_info_print_table_row(2,    "glaze library version", GLAZE_LIB_VERSION);
    php_info_print_table_row(2,    "formats",
        "JSON, BEVE, CBOR, MessagePack, TOML, YAML");
    php_info_print_table_end();
}

zend_module_entry glaze_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_GLAZE_EXTNAME,
    glaze_functions,
    PHP_MINIT(glaze),
    nullptr,               // MSHUTDOWN
    nullptr,               // RINIT
    nullptr,               // RSHUTDOWN
    PHP_MINFO(glaze),
    PHP_GLAZE_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_GLAZE
extern "C" {
    ZEND_GET_MODULE(glaze)
}
#endif
