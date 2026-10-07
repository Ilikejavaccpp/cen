#ifndef CEN__STR_HPP
#define CEN__STR_HPP

#include <string>
#include <string_view>
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif
#include "cstr.h"
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace cen {

class string : public str {
public:
    inline string() {
        this->pointer = nullptr;
        this->length = 0;
    }

    inline string(const char* c_str) {
        if (c_str) {
            str temp = str_init(c_str);
            this->pointer = temp.pointer;
            this->length = temp.length;
        } else {
            this->pointer = nullptr;
            this->length = 0;
        }
    }

    inline string(const std::string& std_str) {
        str temp = str_init(std_str.c_str());
        this->pointer = temp.pointer;
        this->length = temp.length;
    }

    inline string(std::string_view sv) {
        std::string temp_std(sv);
        str temp = str_init(temp_std.c_str());
        this->pointer = temp.pointer;
        this->length = temp.length;
    }

    inline ~string() {
        str_destroy(static_cast<str*>(this));
    }

    // --- Method wrappers for CEN string functions ---

    inline string* append(const char* c_string, str_t* arena) {
        str_append(static_cast<str*>(this), c_string, arena);
        return this;
    }

    inline string* append_heap(const char* c_string) {
        str_append_heap(static_cast<str*>(this), c_string);
        return this;
    }

    inline string* remove_chars(int index, int number) {
        str_delete(static_cast<str*>(this), index, number);
        return this;
    }

    inline string* change(const char* c_string, int index, str_t* arena) {
        str_change(static_cast<str*>(this), c_string, index, arena);
        return this;
    }

    inline bool is_empty() {
        return str_isempty(static_cast<str*>(this));
    }

    inline void prints(FILE* out) {
        str_print(static_cast<str*>(this), out);
    }

    // --- Operators & Conversions ---

    inline operator std::string_view() const {
        if (this->pointer && this->length > 0) {
            return std::string_view(this->pointer, static_cast<size_t>(this->length));
        }
        return std::string_view();
    }

    inline operator std::string() const {
        if (this->pointer && this->length > 0) {
            return std::string(this->pointer, static_cast<size_t>(this->length));
        }
        return std::string();
    }

    inline bool operator==(const string& other) const {
        str s1 = *static_cast<const str*>(this);
        str s2 = *static_cast<const str*>(&other);
        return str_cmp(const_cast<str*>(&s1), const_cast<str*>(&s2)) == EQUAL;
    }

    inline bool operator==(const char* other) const {
        return str_isequalto(const_cast<str*>(static_cast<const str*>(this)), other);
    }
};

} // namespace cen

#endif // __cplusplus

#endif // CEN__STR_HPP
