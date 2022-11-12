#ifndef ASSIGNMENT_2_COMMON_HPP
#define ASSIGNMENT_2_COMMON_HPP

struct MyString {

    static constexpr size_t SIZE = 16;
    char data[SIZE];

    MyString() {
        std::memset(data, 0, SIZE);
    }

    // Copy constructor
    MyString(const MyString& str) {
        std::memcpy(this->data, str.data, SIZE);
    }

    // Move constructor
    MyString(MyString&& str) {
        std::memcpy(this->data, str.data, SIZE);
        std::memset(str.data, 0, SIZE);
    }

    friend bool operator==(const MyString& lhs, const MyString& rhs) {
        return std::memcmp(lhs.data, rhs.data, SIZE) == 0;
    }

    // user-defined copy assignment (copy-and-swap idiom)
    MyString& operator=(MyString other) {
        std::memcpy(this->data, other.data, SIZE);
        return *this;
    }

    static MyString from_string(std::string from) {
        MyString str;
        if (from.size() > SIZE) {
            std::memcpy(str.data, from.c_str(), SIZE);
        }
        else {
            std::memcpy(str.data, from.c_str(), from.size());
        }
        return str;
    }

};

template <>
struct std::hash<MyString> {
    size_t operator()(MyString const& s) const noexcept {
        size_t hash = 2166136261u;
        for (char i : s.data) {
            hash ^= static_cast<uint8_t>(i);
            hash *= 16777619;
        }
        return hash;
    }
};

#endif //ASSIGNMENT_2_COMMON_HPP
