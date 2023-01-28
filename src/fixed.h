#pragma once

template<typename T, typename T2>
struct Fixed {
    using TYPE=Fixed<T, T2>;
    constexpr static T ONE = 1 << 8;
    Fixed() : value_{}
    {
    }

    //explicit Fixed(TYPE const& x) {
        //value_ = x.value_;
    //}
    Fixed(float const x) {
        value_ = static_cast<T>(x * ONE);
    }
    explicit Fixed(T const x) {
        value_ = x;
    }
    const TYPE operator=(const TYPE &other) { value_=other.value_; return *this; }
    const TYPE operator=(const float x) {
        value_ = static_cast<T>(x * ONE);
        return *this;
    }
    const TYPE operator+(const TYPE &other) const {
        TYPE x(T(value_ + other.value_));
        return x;
    }
    const TYPE& operator+=(const TYPE &other) { value_ += other.value_; return *this; }
    const TYPE operator*(const TYPE &other) const
    {
        T2 val = static_cast<T2>(value_) * other.value_;
        return Fixed(T(val / ONE));
    }
    explicit inline operator float() const {
        return static_cast<float>(value_) / (1 << 8);
    }

    T value_;
};

using Fixed16 = Fixed<int16_t, int32_t>;
