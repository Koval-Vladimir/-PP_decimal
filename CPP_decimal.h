#ifndef CPP_DECIMAL_H
#define CPP_DECIMAL_H

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <initializer_list>
using namespace std;

namespace CPP_Decimal
{

bool valid_value(float value);
bool is_float_zero(float value);
bool is_negative_zero(float value);

class Decimal
{
public:
    enum
    {               
        LESS                                           = -1,
        EQUAL                                          = 0,
        GREATER                                        = 1,

        SIGN_PLUS                                      = 0,
        SIGN_MINUS                                     = 1,
    };
private: 
    enum 
    {
        OK                                             = 0, 
        NUMBER_IS_TOO_LARGE_OR_EQUAL_INFINITY          = 1, 
        NUMBER_IS_TOO_SMALL_OR_EQUAL_NEGATIVE_INFINITY = 2,
        PARTS_OF_DECIMAL                               = 4,
        PARTS_OF_LONG_DECIMAL                          = 8,
        
        MIN_EXPONENT                                   = 0,
        MAX_EXPONENT                                   = 28,
        MAX_BITS_IN_MANTISSA_LONG_DECIMAL              = 224,
        BINARY_BASE                                    = 2,
        DIVISION_BY_ZERO                               = 3
    };
    
    int bits[PARTS_OF_DECIMAL]; // { bits[3], bits[2], bits[1], bits[0] }
    
    // Вложенный класс Long_decimal выполняет роль "широкого" decimal для работы и хранения промежуточного результата операций
    class Long_decimal
    {
        public: // Члены Long_decimal специально установлены как public, для удобства обращения к ним, сам же класс private
            int bits[PARTS_OF_LONG_DECIMAL]; // { bits[7], bits[6], bits[5], bits[4], bits[3], bits[2], bits[1], bits[0] }
        
        // Конструкторы класса Long_decimal
            Long_decimal(); // Конструктор по умолчанию
            Long_decimal(initializer_list<unsigned> il);

        // Арифметические операторы (+ - * /)
            Long_decimal operator+(Long_decimal value_2) const;
            Long_decimal operator-(Long_decimal value_2) const;
            Long_decimal operator*(Long_decimal value_2) const;
            Long_decimal operator/(Long_decimal value_2) const;

        // Методы для работы со знаком числа Long_decimal
            int get_sign() const;
            void set_sign(int sign);

        // Методы для работы с экспонентой числа Long_decimal
            int get_exponent() const;
            void set_exponent(int exponent);

        // Методы для работы с мантиссой числа Long_decimal
            int get_bit(int bit_number) const;
            void set_bit(int bit_number, int bit);
            int identify_part_of_mantissa(int bit_number) const;

        // Вывод значения Long_decimal в десятичной системе счисления
            void output_in_dec() const;

        // Вспомогательные методы Long_decimal
            static int align_dividend_and_divider(const Long_decimal & dividend, Long_decimal & divider);
            static Long_decimal * get_less_module(const Long_decimal & value_1, const Long_decimal & value_2);
            static void make_equal_exponents(Long_decimal & val_1, Long_decimal & val_2);
            int convert_to_decimal(Decimal & dst);
            void bank_rounding(const Long_decimal & fractional);
            bool is_zero() const;
            void clear();
            int compare(Long_decimal value_2) const;
            void invert_mantissa();
            void increment_mantissa();          
            void to_up_exponent(int need_exponent);
            Long_decimal to_down_exponent();
            void mult_mantissa_on_10();
            Long_decimal div_mantissa_on_10();
            void shift_to_left(int shift);
            void shift_to_right(int shift);            
            void discard_zeroes();
    }; // end class Long_decimal

// Вспомогательные методы Decimal
    void clear();
    Long_decimal convert_to_Long_decimal() const;
    int compare(Decimal & value_2);
    bool is_zero() const;

public:

// Конструкторы класса Decimal
    Decimal(); // Конструктор по умолчанию
    Decimal(initializer_list<unsigned> il);
    explicit Decimal(int src);   // Конструктор преобразования из int
    explicit Decimal(float src); // Конструктор преобразования из float

// Арифметические операторы (+ - * /)
    Decimal operator+(Decimal value_2) const;
    Decimal operator-(Decimal value_2) const;
    Decimal operator*(Decimal value_2) const;
    Decimal operator/(Decimal value_2) const;

// Операторы сравнения (< <= > >= == !=)
    bool operator< (Decimal value_2) const;
    bool operator<=(Decimal value_2) const;
    bool operator> (Decimal value_2) const;
    bool operator>=(Decimal value_2) const;
    bool operator==(Decimal value_2) const;
    bool operator!=(Decimal value_2) const;

// Преобразователи
    explicit operator int() const;   // Преобразует Decimal в int
    explicit operator float() const; // Преобразует Decimal в float

// Другие функции
    Decimal floor() const;
    Decimal round() const;
    Decimal truncate() const;
    Decimal negate() const;

// Вывод значения Decimal в десятичной системе счисления
    friend ostream & operator<<(ostream & os, Decimal value); 

// Вывод значения Decimal в бинарном виде
    void output_binary() const;

// Методы для работы со знаком числа Decimal
    int get_sign() const;
    void set_sign(int sign);

// Методы для работы с экспонентой числа Decimal
    int get_exponent() const;
    void set_exponent(int exponent);

}; // end class Decimal

} // end namespace CPP_Decimal
#endif // CPP_DECIMAL_H
