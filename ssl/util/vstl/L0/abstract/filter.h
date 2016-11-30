#pragma once
namespace filter_l0 {
    template <class T>
    class filter_pin {
    public:
        virtual T read(char *data, T len) = 0;
        virtual void connect(filter_pin *next) = 0;
    };
    template <class T>
    class filter_pout {
    public:
        virtual T write(const char *data, T len) = 0;
        virtual void connect(filter_pout *next) = 0;
    };
}