// PHP extension wrapping the Glaze library (https://github.com/stephenberry/glaze)
// Exposed functions: glaze_encode, glaze_decode, glaze_prettify, glaze_minify,
//   glaze_validate, glaze_beve_encode, glaze_beve_decode, glaze_cbor_encode,
//   glaze_cbor_decode, glaze_msgpack_encode, glaze_toml_encode, glaze_toml_decode,
//   glaze_yaml_encode, glaze_yaml_decode
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
#include "glaze/msgpack.hpp"
#include "glaze/toml.hpp"
#include "glaze/yaml.hpp"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <string>
#include <system_error>
#include <type_traits>
#include <variant>

#define PHP_GLAZE_VERSION  "1.0.2"
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
// Encode path: PHP value → JSON string (direct builder)
//
// glz::generic construction from user code produces wrong output because
// Glaze serializes generic_json using its own tagged representation — the
// variant's internal state must be populated by glz::read_json, not by
// direct user construction.  A direct string builder sidesteps this entirely.
// ============================================================================

static void escape_json_string(std::string &out, const char *str, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
}

static void build_json(zval *val, std::string &out, int depth)
{
    if (depth > 512) {
        out += "null";
        return;
    }

    ZVAL_DEREF(val);

    switch (Z_TYPE_P(val)) {

        case IS_NULL:
            out += "null";
            return;

        case IS_TRUE:
            out += "true";
            return;

        case IS_FALSE:
            out += "false";
            return;

        case IS_LONG:
            out += std::to_string(Z_LVAL_P(val));
            return;

        case IS_DOUBLE: {
            double d = Z_DVAL_P(val);
            if (!std::isfinite(d)) {
                out += "null";
                return;
            }
            char buf[32];
            auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), d);
            if (ec == std::errc{}) {
                out.append(buf, ptr);
            } else {
                out += "null";
            }
            return;
        }

        case IS_STRING:
            out += '"';
            escape_json_string(out, Z_STRVAL_P(val), Z_STRLEN_P(val));
            out += '"';
            return;

        case IS_ARRAY: {
            zend_array *ht = Z_ARRVAL_P(val);

            if (glaze_is_list(ht)) {
                out += '[';
                bool first = true;
                zval *entry;
                ZEND_HASH_FOREACH_VAL(ht, entry) {
                    if (!first) out += ',';
                    first = false;
                    build_json(entry, out, depth + 1);
                } ZEND_HASH_FOREACH_END();
                out += ']';
            } else {
                out += '{';
                bool first = true;
                zend_string *str_key;
                zend_ulong   num_key;
                zval        *entry;
                ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, str_key, entry) {
                    if (!first) out += ',';
                    first = false;
                    out += '"';
                    if (str_key) {
                        escape_json_string(out, ZSTR_VAL(str_key), ZSTR_LEN(str_key));
                    } else {
                        out += std::to_string(num_key);
                    }
                    out += "\":";
                    build_json(entry, out, depth + 1);
                } ZEND_HASH_FOREACH_END();
                out += '}';
            }
            return;
        }

        case IS_OBJECT: {
            out += '{';
            bool first = true;

            auto pg = get_object_props(val);
            if (pg.ht) {
                zend_string *str_key;
                zend_ulong   num_key;
                zval        *entry;
                ZEND_HASH_FOREACH_KEY_VAL(pg.ht, num_key, str_key, entry) {
                    if (Z_TYPE_P(entry) == IS_UNDEF) continue;
                    if (!first) out += ',';
                    first = false;
                    out += '"';
                    if (str_key) {
                        escape_json_string(out, ZSTR_VAL(str_key), ZSTR_LEN(str_key));
                    } else {
                        out += std::to_string(num_key);
                    }
                    out += "\":";
                    build_json(entry, out, depth + 1);
                } ZEND_HASH_FOREACH_END();
            }

            out += '}';
            return;
        }

        default:
            out += "null";
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
// php_to_generic: PHP value → glz::generic (via JSON round-trip)
//
// Used as the encode bridge for all non-JSON binary/text formats.
// ============================================================================

static bool php_to_generic(zval *value, glz::generic &g)
{
    std::string json_buf;
    build_json(value, json_buf, 0);
    return !glz::read_json(g, json_buf);
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

    std::string buf;
    build_json(value, buf, 0);

    if (flags & GLAZE_FLAG_PRETTY_PRINT) {
        buf = glz::prettify_json(buf);
    }

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
    if (!php_to_generic(value, g)) { RETURN_FALSE; }

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
    if (!php_to_generic(value, g)) { RETURN_FALSE; }

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
// MessagePack — encode only
// Glaze v7.7.0 does not support read_msgpack into glz::generic.
// glaze_msgpack_encode(mixed $value): string|false
// ============================================================================

PHP_FUNCTION(glaze_msgpack_encode)
{
    zval *value;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();

    glz::generic g{};
    if (!php_to_generic(value, g)) { RETURN_FALSE; }

    std::string buf;
    auto ec = glz::write_msgpack(g, buf);
    if (ec) { RETURN_FALSE; }
    RETURN_STRINGL(buf.c_str(), buf.size());
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
    if (!php_to_generic(value, g)) { RETURN_FALSE; }

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
    if (!php_to_generic(value, g)) { RETURN_FALSE; }

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
    // MessagePack (encode only)
    PHP_FE(glaze_msgpack_encode, arginfo_glaze_format_encode)
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
        "JSON, BEVE, CBOR, MessagePack (encode), TOML, YAML");
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
