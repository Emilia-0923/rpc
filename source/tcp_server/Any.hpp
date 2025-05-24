#pragma once

#include <iostream>
#include <typeinfo>
#include <memory>


class Any
{
private:
    class holder
    {
    public:
        virtual ~holder() {}

        virtual const std::type_info& type() = 0;

        virtual holder* clone() = 0;
    };

    template<typename T>
    class placeholder : public holder
    {
    public:
        placeholder(const T& _val) : val(_val) {}

        virtual ~placeholder() {}

        virtual const std::type_info& type() {
            return typeid(T);
        }

        virtual holder* clone() {
            return new placeholder<T>(*this);  // 使用*this来拷贝
        }
    public:
        T val;
    };

    std::unique_ptr<holder> content;
public:
    Any()
        : content(nullptr) {}

    template<typename T>
    Any(const T& _val)
        : content(std::make_unique<placeholder<T>>(_val)) {}

    Any(const Any& _other)
        : content(_other.content ? _other.content->clone() : nullptr) {}

    ~Any() = default;

    Any& swap(Any& _other) {
        std::swap(content, _other.content);
        return *this;
    }

    template<typename T>
    T* get() {
        typeid(T);
        content->type();
        if (typeid(T) != content->type()) {
            logging.fatal("Any类型不匹配!");
            return nullptr;
        }
        else {
            return &dynamic_cast<placeholder<T>*>(content.get())->val;
        }
    }

    template<typename T>
    Any& operator=(const T& _val) {
        Any(_val).swap(*this);
        return *this;
    }

    Any& operator=(const Any& _other) {
        Any(_other).swap(*this);
        return *this;
    }
};