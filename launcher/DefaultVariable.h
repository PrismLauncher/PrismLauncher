#pragma once

template <typename T>
class DefaultVariable {
   public:
    DefaultVariable(const T& value) { defaultValue = value; }
    DefaultVariable<T>& operator=(const T& value)
    {
        currentValue = value;
        is_default = currentValue == defaultValue;
        is_explicit = true;
        return *this;
    }
    operator const T&() const { return is_default ? defaultValue : currentValue; }
    bool isDefault() const { return is_default; }
    bool isExplicit() const { return is_explicit; }

   private:
    T currentValue;
    T defaultValue;
    bool is_default = true;
    bool is_explicit = false;
};
