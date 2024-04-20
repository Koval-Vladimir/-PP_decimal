#include "CPP_decimal.h"

namespace CPP_Decimal
{

bool valid_value(float value)
{ // Функция возвращает false, если value является +-NAN или +-INF; true - если нормальное число
    unsigned long long temp = (*((unsigned long long*)(&value)) >> 23);   
    return (temp & 0xFF) != 255;
}

bool is_float_zero(float value)
{
    bool result = false;                                 
    if((*((unsigned*)(&value)) & 0x7FFFFFFF) == 0)
        result = true;
    return result;
}

bool is_negative_zero(float value)
{ // Функция возвращает true, если х == -0.0 (но не просто 0)
    bool result = false;                                          
    if((*((unsigned*)(&value)) & 0x7FFFFFFF) == 0 && (*((unsigned*)(&value)) >> 31) == 1)
        result = true;
    return result;
}


/*************************************************************************************************/
/***************************************** Decimal ***********************************************/
/*************************************************************************************************/

// Конструкторы класса

Decimal::Decimal() // Конструктор по умолчанию
{
    for(int i = 0; i < PARTS_OF_DECIMAL; ++i)
        this->bits[i] = 0;
}

Decimal::Decimal(initializer_list<unsigned> il)
{ // Конструктор позволяет инициализировать части Decimal в удобном для человека порядке, т.е. [3][2][1][0], а не [0][1][2][3]
    if(0 <= il.size() && il.size() <= PARTS_OF_DECIMAL)
    {
        int i = PARTS_OF_DECIMAL - 1;
        for(auto iter = il.begin(); iter < il.end(); ++iter, --i)
            this->bits[i] = *iter; 
        if((this->bits[PARTS_OF_DECIMAL - 1] & 0x7FE0FFFF) != 0)
        {
            cout << "Bad initializer_list<unsigned> in ctor (Error value in bits[PARTS_OF_DECIMAL - 1]).\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
        else if(((this->bits[PARTS_OF_DECIMAL - 1] & 0x001F0000) >> 16) > MAX_EXPONENT)
        {
            cout << "Bad initializer_list<unsigned> in ctor (Invalid exponent: exp > MAX_EXPONENT).\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        cout << "Bad initializer_list<unsigned> in ctor.\nProgram terminate.\n";
        exit(EXIT_FAILURE);  
    }
}

Decimal::Decimal(int src) : Decimal() // Конструктор преобразования из int 
{
    if(src < 0) // если число отрицательное
    {
        this->set_sign(SIGN_MINUS); // значит и результат отрицательный
        src *= -1;                // делаем исходный int положительным
    }
    this->bits[0] = src;
}

Decimal::Decimal(float src) : Decimal() // Конструктор преобразования из float
{
    char str_float[16] = { 0 };
    int integer = 0;
    int exponent = 0;
    int i = 0;

    if(valid_value(src))
    {
        if(is_float_zero(src))
        {
            this->clear();
            if(is_negative_zero(src))
                this->set_sign(SIGN_MINUS);
        }
        // если (|src| > 79,228,162,514,264,337,593,543,950,335), т.е слишком большое
        else if((unsigned long)(fabs(fabs(src) - 79228162514264330000000000000.0)) >= 8796093022208ul)
        {
            cout << "src in ctor Decimal(float src) is very big.\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
        else
        {
            sprintf(str_float, "%.6e", src);               // преобразуем float в строковое представление
            while(str_float[i] != '.')                     // ищем точку
                ++i;
            for(int j = 1; j <= 6; ++j, ++i)               // делаем строковое число целым перенося точку вправо
            {
                char temp = str_float[i + 1];
                str_float[i + 1] = str_float[i];
                str_float[i] = temp;
            }
            str_float[i] = '\0';                           // делим строковое число на число и экспоненту "1e2" => "1\02"
            sscanf(str_float, "%d", &integer);             // получаем целое число
            sscanf(&str_float[i + 2], "%d", &exponent);    // получаем экспоненту

            // если (|src| > 79,228,162,514,264,337,593,543,950,335), т.е слишком большое
            if(exponent > MAX_EXPONENT || (exponent == MAX_EXPONENT && integer > 7922816))   
            {
                cout << "src in ctor Decimal(float src) is very big.\nProgram terminate.\n";
                exit(EXIT_FAILURE);
            }
            else if(exponent < -MAX_EXPONENT)              // если (0 < |src| < 1e-28), т.е. слишком маленькое, но != 0
                this->clear();
            else // если число входит в диапазон
            {
                exponent -= 6;                             // поскольку делали число целым, то нужно вычислить исходное место точки
                *this = Decimal(integer);
                Long_decimal temp_dst = this->convert_to_Long_decimal();
                if(exponent > 0)                           // если число целое (т.е. без экспоненты)
                {
                    for(int ctr = 0; ctr < exponent; ++ctr)
                        temp_dst.mult_mantissa_on_10();
                }
                else                                       // если число дробное (т.е. c экспонентой), включая те, которые могут 
                {                                          // быть преобразованы к целому, путем отбрасывания дробной части == 0
                    exponent *= -1;
                    temp_dst.set_exponent(exponent);
                }
                temp_dst.discard_zeroes();
                temp_dst.convert_to_decimal(*this);
            }
        }
    }
    else
    {
        cout << "Bad src in ctor Decimal(float src).\nProgram terminate.\n";
        exit(EXIT_FAILURE);  
    }
} 

// Арифметические операторы (+ - * /)

Decimal Decimal::operator+(Decimal value_2) const
{
    Decimal result;
    Decimal value_1 = *this;
    if(value_1.is_zero() && value_2.is_zero()) // если оба числа 0(+- и ^n), то и результат будет 0(+- и ^n)
    {
        int sign_val_1 = value_1.get_sign();
        int sign_val_2 = value_2.get_sign();
        int exp_val_1 = value_1.get_exponent();
        int exp_val_2 = value_2.get_exponent();
        result.set_sign(sign_val_1 + sign_val_2 != SIGN_MINUS ? SIGN_PLUS : SIGN_MINUS);
        result.set_exponent(exp_val_1 > exp_val_2 ? exp_val_1 : exp_val_2);
    }
    else if(value_1.is_zero() || value_2.is_zero()) // если одно число == 0, то сумма будет равна другому числу
        result = value_1.is_zero() ? value_2 : value_1; 
    else // если ни одно из чисел != 0
    {
        Long_decimal val_1 = value_1.convert_to_Long_decimal();
        Long_decimal val_2 = value_2.convert_to_Long_decimal();
        Long_decimal res;
        if(val_1.get_sign() == val_2.get_sign()) // если у чисел при сложении одинаковые знаки,
            res = val_1 + val_2;  // то это действительно сложение
        else // если у чисел при сложении разные знаки,
            res = val_1 - val_2;  // то это на самом деле вычитание
        int result_convert = res.convert_to_decimal(result);
        if(result_convert != OK)
        {
            cout << "Overflow in operator+().\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
    }
    return result;
}

Decimal Decimal::operator-(Decimal value_2) const
{
    Decimal value_1 = *this;
    if(value_1.is_zero() && value_2.is_zero())
    {
        if(value_2.get_sign() == SIGN_MINUS)
            value_2.set_sign(SIGN_PLUS);
    }
    else // меняем операцию вычитания на операцию сложения с изменением знака вычитаемого: 5 - 3 => 5 + (-3)
        value_2.set_sign((value_2.get_sign() == SIGN_PLUS) ? SIGN_MINUS : SIGN_PLUS); // поменяли знак вычитаемого
    return value_1 + value_2; // заменили operator-() на operator+();
}

Decimal Decimal::operator*(Decimal value_2) const
{
    Decimal result;
    Decimal value_1 = *this;
    // любое число умноженное на 0 дает в результате 0
    if(value_1.is_zero() || value_2.is_zero())
    {
        result.clear();
        int sign_val_1 = value_1.get_sign();
        int sign_val_2 = value_2.get_sign();
        int exp_val_1 = value_1.get_exponent();
        int exp_val_2 = value_2.get_exponent();
        result.set_sign(sign_val_1 + sign_val_2 != SIGN_MINUS ? SIGN_PLUS : SIGN_MINUS);
        result.set_exponent(exp_val_1 + exp_val_2);
    }
    else
    {
        Long_decimal val_1 = value_1.convert_to_Long_decimal();
        Long_decimal val_2 = value_2.convert_to_Long_decimal();
        Long_decimal res;
        res = val_1 * val_2;
        int result_convert = res.convert_to_decimal(result);
        if(result_convert != OK)
        {
            cout << "Overflow in operator*().\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
    }
    return result;
}

Decimal Decimal::operator/(Decimal value_2) const
{
    Decimal result;
    Decimal value_1 = *this;
    if(value_1.is_zero() || value_2.is_zero()) 
    {
        if(value_2.is_zero()) // деление на 0 недопустимо
        {
            cout << "Error in operator/(value_2 == 0).\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
        //else // ноль разделить на любое число == 0
    }
    else
    {
        Long_decimal val_1 = value_1.convert_to_Long_decimal();
        Long_decimal val_2 = value_2.convert_to_Long_decimal();
        Long_decimal res;
        res = val_1 / val_2;
        int result_convert = res.convert_to_decimal(result);
        if(result_convert != OK)
        {
            cout << "Overflow in operator/().\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
    }
    return result;
}

// Операторы сравнения (< <= > >= == !=)

bool Decimal::operator< (Decimal value_2) const
{
    Decimal value_1 = *this;
    return value_1.compare(value_2) == LESS ? true : false;
}

bool Decimal::operator<=(Decimal value_2) const
{
    Decimal value_1 = *this;
    return value_1.compare(value_2) != GREATER ? true : false;    
}

bool Decimal::operator> (Decimal value_2) const
{
    Decimal value_1 = *this;
    return value_1.compare(value_2) == GREATER ? true : false;
}

bool Decimal::operator>=(Decimal value_2) const
{
    Decimal value_1 = *this;
    return value_1.compare(value_2) != LESS ? true : false;
}

bool Decimal::operator==(Decimal value_2) const
{
    Decimal value_1 = *this;
    return value_1.compare(value_2) == EQUAL ? true : false;
}

bool Decimal::operator!=(Decimal value_2) const
{
    Decimal value_1 = *this;
    return value_1.compare(value_2) != EQUAL ? true : false;
}

// Преобразователи

Decimal::operator int() const
{
    int ret_val;
    if(this->is_zero())
        ret_val = 0;
    else
    {
        Decimal temp = *this;
        temp = temp.truncate();                     // получаем целую часть
        if(temp.bits[2] == 0 && temp.bits[1] == 0)  // если целое число входит в диапазон одного int
        {
            ret_val = temp.bits[0];
            if(this->get_sign() == SIGN_MINUS)      // если исходный Decimal отрицательный, то и int тоже отрицательный
                ret_val *= -1;                          // делаем результат отрицательным
        }
        else // число выходит за диапазон типа int
        {
            cout << "Decimal is very big for convert in int.\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }            
    }
    return ret_val;
}

Decimal::operator float() const{
    float ret_val = 0.0;
    enum {LAST_CHAR = 29, SIZE};
    char  prev_str_float[SIZE] = { 0 };
    char final_str_float[SIZE] = { 0 };
    
    Long_decimal temp_src = this->convert_to_Long_decimal();
    temp_src.discard_zeroes();
    int prev_exponent = temp_src.get_exponent();
    temp_src.set_exponent(MIN_EXPONENT);
    temp_src.set_sign(SIGN_PLUS);
    int exponent = 0;
    int cur_char = LAST_CHAR;

    while(!temp_src.is_zero())
    {
        Long_decimal temp_fractional = temp_src.div_mantissa_on_10();
        prev_str_float[cur_char--] = temp_fractional.bits[0] + '0';
        ++exponent;
    }
    if(exponent > prev_exponent)  // если целая часть числа > 0
    {
        prev_str_float[cur_char--] = '.';
        prev_str_float[cur_char--] = temp_src.bits[0] + '0';
        strcpy(final_str_float, &prev_str_float[cur_char] + 1);
        int temp_exponent = exponent - prev_exponent;
        cur_char = LAST_CHAR;
        while(temp_exponent != 0)
        {
            prev_str_float[cur_char--] = (temp_exponent % 10) + '0';
            temp_exponent /= 10;
        }
        strcat(final_str_float, "e");
        strcat(final_str_float, &prev_str_float[cur_char] + 1);
    }
    else   // если целая часть числа == 0
    {
        for(int i = prev_exponent - exponent; i > 0; --i)
            prev_str_float[cur_char--] = '0';
        prev_str_float[cur_char--] = '.';
        prev_str_float[cur_char--] = temp_src.bits[0] + '0';
        strcat(final_str_float, &prev_str_float[cur_char] + 1);
    }
    sscanf(final_str_float, "%f", &ret_val);
    if(this->get_sign() == SIGN_MINUS)
        ret_val *= -1;  
    return ret_val;
}

// Другие функции

Decimal Decimal::floor() const
{ // Функция округляет указанное Decimal число до ближайшего целого числа в сторону отрицательной бесконечности
  // Если число отрицательно то округляем в сторону отрицательной бесконечности, т.е. из -2.5 -> -3, но
  // если положительно, то просто отбрасываем дробную часть, т.е. из 2.5 -> 2
    Decimal result; 
    int exponent = this->get_exponent();
    if(exponent != MIN_EXPONENT) // если есть дробная часть по которой программа будет округлять
    {
        Long_decimal temp_val = this->convert_to_Long_decimal();
        Long_decimal integer_part    = temp_val;
        Long_decimal fractional_part = temp_val;
        // получаем целую часть
        while(exponent != MIN_EXPONENT)              
        {
            integer_part.to_down_exponent();
            --exponent;
        }
        // получаем дробную часть
        fractional_part = temp_val - integer_part;
        // округляем      
        if(!fractional_part.is_zero() && integer_part.get_sign() == SIGN_MINUS)                                               
            integer_part.increment_mantissa();
        
        int result_convert = integer_part.convert_to_decimal(result);
        if(result_convert != OK)
        {
            cout << "Overflow in floor().\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
    }
    else // если нет дробной части, то и округлять нечего
        result = *this;
    return result;
}

Decimal Decimal::round() const
{ //  Функция округляет Decimal до ближайшего целого числа 
  // (в библиотеке cmath round() использует математическое округление, поэтому оно здесь и реализовано)
    Decimal result;
    int exponent = this->get_exponent();
    if(exponent != MIN_EXPONENT) // если есть дробная часть по которой программа будет округлять
    {
        Long_decimal temp_val = this->convert_to_Long_decimal();
        Long_decimal integer_part    = temp_val;
        Long_decimal fractional_part = temp_val;
        Long_decimal five = { 0, 0, 0, 0, 0, 0, 0, 5 };
        five.set_exponent(1); // получаем 0.5
        // получаем целую часть
        while(exponent != MIN_EXPONENT)              
        {
            integer_part.to_down_exponent();
            --exponent;
        }
        // получаем дробную часть с положительным знаком
        fractional_part = temp_val - integer_part;
        fractional_part.set_sign(SIGN_PLUS);
        // округляем      
        Long_decimal::make_equal_exponents(five, fractional_part);
        if(fractional_part.compare(five) != LESS) // если fractional_part >= five
            integer_part.increment_mantissa();
        int result_convert = integer_part.convert_to_decimal(result);
        if(result_convert != OK)
        {
            cout << "Overflow in floor().\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
    }
    else // если нет дробной части, то и округлять нечего
        result = *this;
    return result;
}

Decimal Decimal::truncate() const
{ //  Функция возвращает целые цифры указанного Decimal числа; любые дробные цифры отбрасываются, включая конечные нули  
    Decimal result; 
    int exponent = this->get_exponent();
    if(exponent != MIN_EXPONENT) // если есть дробная часть по которой программа будет округлять
    {
        Long_decimal integer_part = this->convert_to_Long_decimal();
        // получаем целую часть
        while(exponent != MIN_EXPONENT)              
        {
            integer_part.to_down_exponent();
            --exponent;
        }
        integer_part.discard_zeroes();
        int result_convert = integer_part.convert_to_decimal(result);
        if(result_convert != OK)
        {
            cout << "Overflow in floor().\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
    }
    else // если нет дробной части, то и округлять нечего
        result = *this;
    return result;
}

Decimal Decimal::negate() const
{
    Decimal result = *this; 
    // меняем знак на противоположный
    result.set_sign(this->get_sign() == SIGN_PLUS ? SIGN_MINUS : SIGN_PLUS);
    return result;
}

// Вспомогательные методы

void Decimal::clear()
{
    for(int i = 0; i < PARTS_OF_DECIMAL; i++)
        this->bits[i] = 0;
}

Decimal::Long_decimal Decimal::convert_to_Long_decimal() const
{
    Long_decimal result;
    result.clear();
    if(!this->is_zero())
    {
        for(int part = 0; part < PARTS_OF_DECIMAL - 1; ++part) // копируем мантиссу
            result.bits[part] = this->bits[part];
    }
    result.set_exponent(this->get_exponent());
    result.set_sign(this->get_sign());
    return result;
}

int Decimal::compare(Decimal & value_2)
{
    // Функция возвращает: -1 если value_1 < value_2; 0 если value_1 == value_2; 1 если value_1 > value_2
    int ret_val;

    int sign_val_1 = this->get_sign();
    int sign_val_2 = value_2.get_sign();

    if(this->is_zero() && value_2.is_zero())
        ret_val = EQUAL;
    else if(sign_val_1 == sign_val_2)
    {
        Long_decimal val_1 = this->convert_to_Long_decimal();
        Long_decimal val_2 = value_2.convert_to_Long_decimal();
        ret_val = val_1.compare(val_2);
    }
    else
        ret_val = (sign_val_1 == SIGN_MINUS) ? LESS : GREATER;
    return ret_val;
}

bool Decimal::is_zero() const
{
    bool is_true = true;
    for(int part = 0; part < PARTS_OF_DECIMAL - 1; part++)
    {
        if(this->bits[part] != 0)
        {
            is_true = false;
            break;
        }
    }
    return is_true;
}

// Вывод значения Decimal в десятичной системе счисления

ostream & operator<<(ostream & os, Decimal value)     
{         
    value.convert_to_Long_decimal().output_in_dec();
    return os;
}   

// Вывод значения Decimal в бинарном виде

void Decimal::output_binary() const
{
    // bits[3] 0XXXXXXX-ХХХ00000-XXXXXXXX-XXXXXXXX  // где X - бит не используется
    // bits[2] 00000000-00000000-00000000-00000000
    // bits[1] 00000000-00000000-00000000-00000000
    // bits[0] 00000000-00000000-00000000-00000000
    
    char buffer[36] = { 0 };
    for(int i = PARTS_OF_DECIMAL - 1; i >= 0; i--)
    {
        unsigned int temp_num = this->bits[i];
        for(int j = 34; j >= 0; j--)
        {
            if(j == 26 || j == 17 || j == 8)
            {
                buffer[j] = '-';
                j--;
            }
            buffer[j] = temp_num % 2 + '0';
            temp_num /= 2;
        }
        cout << "[" << i << "]" << " " << buffer << endl;
    }
    cout << endl;
}

// Методы для работы со знаком числа Decimal

int Decimal::get_sign() const
{
    return (this->bits[PARTS_OF_DECIMAL - 1] >> 31) & 0x1;
}

void Decimal::set_sign(int sign)
{
    if(sign) // делаем число отрицательным
        this->bits[PARTS_OF_DECIMAL - 1] |= 0x80000000; 
    else // делаем число положительным
        this->bits[PARTS_OF_DECIMAL - 1] &= 0x7FFFFFFF;
}

// Методы для работы с экспонентой числа

int Decimal::get_exponent() const
{
    return (this->bits[PARTS_OF_DECIMAL - 1] >> 16) & 0xFF;
}

void Decimal::set_exponent(int exponent)
{
    this->bits[PARTS_OF_DECIMAL - 1] &= 0xFF00FFFF;       // очищаем предыдущее значение экспоненты
    this->bits[PARTS_OF_DECIMAL - 1] |= (exponent << 16); // устанавливаем новое значение экспоненты
}

/******************************************************************************************************/
/***************************************** Long_decimal ***********************************************/
/******************************************************************************************************/

// Конструкторы класса Long_decimal

Decimal::Long_decimal::Long_decimal()
{
    for(int i = 0; i < PARTS_OF_LONG_DECIMAL; i++)
        this->bits[i] = 0;
}

Decimal::Long_decimal::Long_decimal(initializer_list<unsigned> il)
{
    if(0 <= il.size() && il.size() <= PARTS_OF_LONG_DECIMAL)
    {
        int i = PARTS_OF_LONG_DECIMAL - 1;
        for(auto iter = il.begin(); iter < il.end(); ++iter, --i)
            this->bits[i] = *iter; 
        if((this->bits[PARTS_OF_LONG_DECIMAL - 1] & 0x7FE0FFFF) != 0)
        {
            cout << "Bad initializer_list<unsigned> in ctor (Error value in bits[PARTS_OF_LONG_DECIMAL - 1]).\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
        else if(((this->bits[PARTS_OF_LONG_DECIMAL - 1] & 0x001F0000) >> 16) > MAX_EXPONENT)
        {
            cout << "Bad initializer_list<unsigned> in ctor (Invalid exponent: exp > MAX_EXPONENT).\nProgram terminate.\n";
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        cout << "Bad initializer_list<unsigned> in ctor.\nProgram terminate.\n";
        exit(EXIT_FAILURE);  
    }
}

// Арифметические операторы (+ - * /)

Decimal::Long_decimal Decimal::Long_decimal::operator+(Long_decimal value_2) const
{ // Функция правильно складывает только если числа с одинаковыми знаками
    Long_decimal result;
    Long_decimal value_1 = *this;
    int transfer = 0;
    make_equal_exponents(value_1, value_2); // выравниваем десятичный разделитель
    result.clear();
    for(int current_bit = 0; current_bit < MAX_BITS_IN_MANTISSA_LONG_DECIMAL; current_bit++)
    {
        int sum = value_1.get_bit(current_bit) + value_2.get_bit(current_bit) + transfer;
        transfer = sum / BINARY_BASE; // вычисляем, есть ли перенос в следующий разряд
        sum %= BINARY_BASE;           // получаем результат значения текущего разряда
        result.set_bit(current_bit, sum);
    }
    result.set_exponent(value_1.get_exponent());
    result.set_sign(value_1.get_sign());
    return result;
}

Decimal::Long_decimal Decimal::Long_decimal::operator-(Long_decimal value_2) const
{ // Функция правильно вычитает только если числа имеют равные экспоненты
    Long_decimal result;
    Long_decimal value_1 = *this;
    Long_decimal * less_module = get_less_module(value_1, value_2); // определяем, какое число по модулю меньше   
    Long_decimal * greater_module = (less_module == &value_1) ? &value_2 : &value_1;
    int sign_greater = greater_module->get_sign(); // сохраняем знак для итогового результата
    less_module->set_sign(SIGN_PLUS);         // получили модуль меньшего числа
    greater_module->set_sign(SIGN_PLUS);      // получили модуль большего числа
    less_module->invert_mantissa();           // необходимо для вычитания
    less_module->increment_mantissa();        // необходимо для вычитания
    result = *greater_module + *less_module; // вычитание "через сложение"
    result.set_sign(sign_greater);           // устанавливаем знак того числа, модуль которого больше
    return result;
}

Decimal::Long_decimal Decimal::Long_decimal::operator*(Long_decimal value_2) const
{
    Long_decimal result;
    Long_decimal value_1 = *this;
    // определяем знак результата. Если сумма знаков чисел дает 1(т.е. SIGN_MINUS), то и у результата будет минус.
    int sign = (value_1.get_sign() + value_2.get_sign() == SIGN_MINUS) ? SIGN_MINUS : SIGN_PLUS;
    // определяем экспоненту результата путем сложения исходных экспонент
    int exponent = value_1.get_exponent() + value_2.get_exponent();
    // получаем модули чисел
    value_1.set_sign(SIGN_PLUS);
    value_2.set_sign(SIGN_PLUS);
    // обнуляем экспоненты для правильного вычисления результата
    value_1.set_exponent(MIN_EXPONENT);
    value_2.set_exponent(MIN_EXPONENT);
    result.clear(); // очищаем для будущего результата
    // проходим по битам множителя
    for(int bit = MAX_BITS_IN_MANTISSA_LONG_DECIMAL - 1; bit >= 0; --bit)
    {
        Long_decimal temp_value_1 = value_1; // необходимо для правильного вычисления результата
        if(value_2.get_bit(bit) == 1)
        {
            temp_value_1.shift_to_left(bit); // сдвигаем биты temp_value_1 на bit влево
            result = result + temp_value_1;
        }
    }
    result.set_exponent(exponent);
    result.set_sign(sign);
    return result;
}

Decimal::Long_decimal Decimal::Long_decimal::operator/(Long_decimal value_2) const
{
    Long_decimal result;
    Long_decimal value_1 = *this;
    Long_decimal temp_result; // хранит промежуточный результат
    int temp_exponent = value_1.get_exponent();
    int ctr_shifts = 0;
    int exponent = 0;
    // определяем знак результата. Если сумма знаков чисел дает 1(т.е. SIGN_MINUS), то и у результата будет минус.
    int sign = (value_1.get_sign() + value_2.get_sign() == SIGN_MINUS) ? SIGN_MINUS : SIGN_PLUS;
    value_1.set_sign(SIGN_PLUS); // необходимо для надлежащего вычитания
    value_2.set_sign(SIGN_PLUS); // необходимо для надлежащего вычитания
    make_equal_exponents(value_1, value_2);
    while(value_1.compare(value_2) == LESS)
    {
        value_1.mult_mantissa_on_10(); // умножаем мантиссу value_1 на 10, чтобы делимое стало больше делителя
        ++exponent;                        // поскольку изначально делимое было меньше делителя, то результат будет < 1 
    }
    ctr_shifts = align_dividend_and_divider(value_1, value_2);
    while(!value_1.is_zero() && exponent <= MAX_EXPONENT + 1)
    {
        value_1 = value_1 - value_2;
        temp_result.set_bit(0, 1); // устанавливаем бит в текущем частном
        if(value_1.is_zero())        // если больше нечего делить
        {
            while(ctr_shifts > 0)
            {
                temp_result.shift_to_left(1);
                --ctr_shifts;
            }
        }
        else // если деление еще не окончено
        {
            while(value_1.compare(value_2) == LESS && ctr_shifts > 0) // пытаемся сделать делимое больше делителя
            {
                temp_result.shift_to_left(1);   // "собираем" временный результат
                value_2.shift_to_right(1);      // "подгоняем" делитель под делимое
                --ctr_shifts;
            }
            while(value_1.compare(value_2) == LESS) // если не удалось сделать делимое больше делителя, значит
            {                                                     // настало время получения дробной части из этого остатка
                ++exponent;
                value_1.mult_mantissa_on_10();      // "дописываем ноль" к остатку от деления       
                temp_result.mult_mantissa_on_10();  // необходимо для итогового результата
                result.mult_mantissa_on_10();       // необходимо для итогового результата
                result = result + temp_result;      // *result "собирает" итоговый результат
                temp_result.clear();       // очищаем для хранения очередного частного от деления остатка
                ctr_shifts = align_dividend_and_divider(value_1, value_2);
            }
        }
    }
    if(result.is_zero()) 
    {
        temp_result.set_exponent(temp_exponent);
        while (temp_exponent-- > 0) 
            temp_result.mult_mantissa_on_10();
        result = result + temp_result;
    } 
    else 
    {
        result = result + temp_result;
        result.set_exponent(exponent);
    }
    result.set_sign(sign);
    return result;
}

// Вспомогательные методы

int Decimal::Long_decimal::align_dividend_and_divider(const Decimal::Long_decimal & dividend, Decimal::Long_decimal & divider)
{ // Функция "подгоняет" делитель под делимое и возвращает количество сдвигов делителя
    int ctr_shifts = 0;
    while(dividend.compare(divider) == GREATER) 
    {
        divider.shift_to_left(1);
        ++ctr_shifts;
    }
    if(dividend.compare(divider) == LESS && ctr_shifts > 0)
    {
        divider.shift_to_right(1);
        --ctr_shifts;
    }
    return ctr_shifts;
}

int Decimal::Long_decimal::convert_to_decimal(Decimal & dst)
{
    int ret_val = OK;
    Long_decimal temp_value = *this;
    Long_decimal fractional; // возможный остаток от деления при попытке сделать число валидным
    int exponent = temp_value.get_exponent();
    int exponent_fractional = 0;
    // если при умножении сумма экспонент больше MAX_EXPONENT либо после других операций в расширенной части установлены 
    // биты и экспонента != 0, то преобразуем число в число равное по значению, но с меньшей экспонентой, пока 
    // оно не станет валидным либо экспонента не станет == 0
    while((exponent > MAX_EXPONENT) || 
        ((temp_value.bits[6] || temp_value.bits[5] || temp_value.bits[4] || temp_value.bits[3]) && exponent != MIN_EXPONENT))
    {
        // часть преобразуемых значений не может быть преобразована с полной точностью, как следствие - после преобразования 
        // такого числа остается дробная часть, исходя из которой, в последствии будет произведено округление полученного числа
        Long_decimal temp_fractional = temp_value.to_down_exponent();
        make_equal_exponents(fractional, temp_fractional);
        fractional = fractional + temp_fractional; // в случае нескольких преобразований, получаемые остатки суммируются
                                        // т.е. округление не по послeдней цифре числа, а по всем отброшенным, т.е. например 
                                        // не по 0.3, а по 0.325
        exponent = temp_value.get_exponent();
        ++exponent_fractional;
    }
    if(!fractional.is_zero())       // если есть дробная часть
    {
        fractional.set_exponent(exponent_fractional); // получаем дробную часть в виде 0.123, а не 0.000123
        temp_value.bank_rounding(fractional); // то используем банковское округление
    }
    if(temp_value.bits[6] == 0 && temp_value.bits[5] == 0 && 
       temp_value.bits[4] == 0 && temp_value.bits[3] == 0) // если удалось сделать число валидным
    {
        for(int part = 0; part < PARTS_OF_DECIMAL; ++part) // сохраняем мантиссу
            dst.bits[part] = temp_value.bits[part];
        dst.bits[3] = 0; // подготавливаем часть со знаком и экспонентой путем очищения этой части
        dst.set_exponent(temp_value.get_exponent());
        dst.set_sign(this->get_sign());
    }
    else // безуспешное преобразование
    {
        ret_val = this->get_sign() == SIGN_PLUS ? 
            NUMBER_IS_TOO_LARGE_OR_EQUAL_INFINITY : NUMBER_IS_TOO_SMALL_OR_EQUAL_NEGATIVE_INFINITY;
    }
    return ret_val;
}

void Decimal::Long_decimal::bank_rounding(const Long_decimal & fractional)
{
    Long_decimal five = { 0, 0, 0, 0, 0, 0, 0, 5 }; // +5 в битовом представлении
    five.set_exponent(1);                             // получаем "+0.5"

    int res_cmp = fractional.compare(five);
    if(res_cmp == GREATER) // если остаток больше 0.5, то округляем вверх (12.6 -> 13)
        this->increment_mantissa();
    else if(res_cmp == EQUAL) // если остаток равен 0.5, то используем банковское округление (12.5 -> 12, но 13.5 -> 14)
    {
        if(this->get_bit(0) == 1) // если число нечетное
            this->increment_mantissa();
    }
    // else - если остаток меньше 0.5, то оставляем число как есть (12.4 -> 12)
}

bool Decimal::Long_decimal::is_zero() const
{ // Функция считает нулем то число, мантисса которого состоит только из нулевых битов, т.е. независимо от знака и экспоненты
    bool is_true = true;
    for(int part = 0; part < PARTS_OF_LONG_DECIMAL - 1; part++)
    {
        if(this->bits[part] != 0)
        {
            is_true = false;
            break;
        }
    }
    return is_true;
}

void Decimal::Long_decimal::clear()
{
    for(int i = 0; i < PARTS_OF_LONG_DECIMAL; i++)
        this->bits[i] = 0;
}

Decimal::Long_decimal * Decimal::Long_decimal::get_less_module(const Long_decimal & value_1, const Long_decimal & value_2)
{
    Long_decimal copy_val_1 = value_1;
    Long_decimal copy_val_2 = value_2;
    // Получаем модули
    copy_val_1.set_sign(SIGN_PLUS);
    copy_val_2.set_sign(SIGN_PLUS);
    const Long_decimal * ret_val = (copy_val_1.compare(copy_val_2) == LESS) ? &value_1 : &value_2;
    return (Long_decimal*)ret_val;
}

void Decimal::Long_decimal::invert_mantissa()
{
    for(int part = 0; part < PARTS_OF_LONG_DECIMAL - 1; ++part)
        this->bits[part] = ~this->bits[part]; // инвертирование битов в this->bits[part]
}

void Decimal::Long_decimal::increment_mantissa()
{
    Long_decimal one = {{ 0, 0, 0, 0, 0, 0, 0, 1 }};
    one.set_exponent(this->get_exponent());
    one.set_sign(this->get_sign());
    *this = *this + one;
}

int Decimal::Long_decimal::compare(Long_decimal value_2) const
{ // Функция возвращает: -1 если value_1 < value_2; 0 если value_1 == value_2; 1 если value_1 > value_2
    int ret_val = EQUAL;
    
    int sign_val_1 = this->get_sign();
    int sign_val_2 = value_2.get_sign();

    if(sign_val_1 == sign_val_2)
    {
        bool values_equal = true;
        Long_decimal value_1 = *this;
        make_equal_exponents(value_1, value_2);
        for(int cur_bit = 0; cur_bit < MAX_BITS_IN_MANTISSA_LONG_DECIMAL; ++cur_bit) // проверка на равенство чисел
        {
            int bit_val_1 = value_1.get_bit(cur_bit);
            int bit_val_2 = value_2.get_bit(cur_bit);
            if(bit_val_1 != bit_val_2)
            {
                values_equal = false;
                break;
            }
        }
        if(!values_equal)
        {
            for(int cur_bit = MAX_BITS_IN_MANTISSA_LONG_DECIMAL - 1; cur_bit >= 0; --cur_bit)
            {
                int bit_val_1 = value_1.get_bit(cur_bit);
                int bit_val_2 = value_2.get_bit(cur_bit);
                if(bit_val_1 != bit_val_2)
                {
                    if(sign_val_1 == SIGN_PLUS)
                        ret_val = (bit_val_1 < bit_val_2) ? LESS : GREATER;
                    else // if(sign_val_1 == SIGN_MINUS)
                        ret_val = (bit_val_1 > bit_val_2) ? LESS : GREATER;
                    break;
                }
            }
        }
    }
    else
        ret_val = (sign_val_1 == SIGN_MINUS) ? LESS : GREATER;
    return ret_val;
}

void Decimal::Long_decimal::make_equal_exponents(Long_decimal & val_1, Long_decimal & val_2)
{ // Функция, если это требуется, выравнивает в числах десятичный разделитель (точку) путем преобразования числа с 
  // меньшей экспонентой в равнозначное число с требуемой экспонентой, например: (0)3 = 3 -> (1)30 = 3.0 -> (2)300 = 3.00 
    int exp_val_1 = val_1.get_exponent();
    int exp_val_2 = val_2.get_exponent();
    if(exp_val_1 != exp_val_2)
    {   
        if(exp_val_1 < exp_val_2)
            val_1.to_up_exponent(exp_val_2);
        else
            val_2.to_up_exponent(exp_val_1);
    }
}

void Decimal::Long_decimal::to_up_exponent(int need_exponent)
{ // Функция преобразует число в число равное по значению, но с большей экспонентой
    for(int cur_exp = this->get_exponent(); cur_exp < need_exponent; ++cur_exp)
        this->mult_mantissa_on_10(); // умножаем мантиссу на 10
    this->set_exponent(need_exponent); // устанавливаем требуемую экспоненту
}

Decimal::Long_decimal Decimal::Long_decimal::to_down_exponent()
{ // Функция делит мантиссу на 10 и декрементирует экспоненту, причем *this будет с 
  // правильными экспонентой и знаком, а остаток будет положительным и с MIN_EXPONENT
    Long_decimal fractional;
    int sign = this->get_sign();                      // сохраняем знак для целой части
    this->set_sign(SIGN_PLUS);                        // получаем модуль
    int exponent = this->get_exponent();              // сохраняем экспоненту для целой части
    this->set_exponent(MIN_EXPONENT);                 // делаем целым для правильного деления на 10
    fractional = this->div_mantissa_on_10();
    fractional.set_exponent(exponent);
    this->set_sign(sign);               // восстанавливаем знак для целой части
    this->set_exponent(exponent - 1);   // уменьшаем исходную экспоненту на 1 в целой части
    return fractional;
}

void Decimal::Long_decimal::mult_mantissa_on_10()
{ // Функция умножает мантиссу и не изменяет экспоненту
    Long_decimal temp_1 = *this;
    Long_decimal temp_2 = *this;
    int exponent = this->get_exponent();
    this->set_exponent(MIN_EXPONENT);
    temp_1.shift_to_left(1);
    temp_2.shift_to_left(3);
    *this = temp_1 + temp_2;
    this->set_exponent(exponent);
}

Decimal::Long_decimal Decimal::Long_decimal::div_mantissa_on_10()
{ // Функция работает с целым числом и "возвращает" целую часть от деления через *this, а дробную - через return
    Long_decimal ten = { { 0, 0, 0, 0, 0, 0, 0, 10 } };
    Long_decimal temp_value_1 = *this;
    Long_decimal temp_value_2 = *this;
    Long_decimal integer, temp_integer, fractional;

    temp_value_1.shift_to_right(1);
    temp_value_2.shift_to_right(2);
    integer = temp_value_1 + temp_value_2;      // q - integer
    temp_integer = integer;
    // путем поочередных сдвигов получаем предцелую часть
    for(int shifts = 4; shifts <= 128; shifts *= 2)
    {
        temp_integer.shift_to_right(shifts);
        integer = temp_integer + integer;
        if(shifts != 128)
          temp_integer = integer;
    }
    // получаем финальную целую часть
    integer.shift_to_right( 3);          // q = q >> 3;
    // получаем дробную часть
    temp_value_1 = integer;
    temp_value_2 = integer;
    temp_value_1.shift_to_left(3);
    temp_value_2.shift_to_left(1);
    temp_integer = temp_value_1 + temp_value_2;    // ((q << 3) + (q << 1))
    fractional = *this - temp_integer;            // n - ((q << 3) + (q << 1))
    // получаем финальную дробную часть
    if(fractional.compare(ten) != LESS)
    {
        fractional = fractional - ten;            // n - ((q << 3) + (q << 1))
        integer.increment_mantissa();
    }
    *this = integer; // "возвращаем" целую часть
    return fractional;
}

void Decimal::Long_decimal::shift_to_left(int shift)
{ // Функция сдвигает биты *this влево на shift бит
    for(int ctr = 0; ctr < shift; ++ctr)
    {
        int prev_transfer = 0;
        for(int part = 0; part < PARTS_OF_LONG_DECIMAL - 1; ++part)
        {
            int transfer = (this->bits[part] >> 31) & 0x1;// сохранили старший бит в bits[part] для возможного переноса в bits[part + 1]
            this->bits[part] <<= 1;                    // сдвигаем биты bits[part] влево на 1 бит
            if(prev_transfer)                           // если от bits[part - 1] был перенос, то его учитываем в bits[part]
                this->bits[part] |= 0x1;
            prev_transfer = transfer;                   // transfer для bits[part] это prev_transfer для bits[part + 1]
        }
        //if(transfer) printf("OVERFLOW\n"); // - значит мы вышли даже за границы Long_decimal (но такого по сути не должно случиться)
    }
}

void Decimal::Long_decimal::shift_to_right(int shift)
{ // Функция сдвигает биты *this вправо на shift бит
    for(int ctr = 0; ctr < shift; ++ctr)
    {
        int prev_transfer = 0;
        for(int part = PARTS_OF_LONG_DECIMAL - 2; part >= 0; --part)
        {
            int transfer = this->bits[part] & 0x1; // сохранили младший бит в bits[part] для возможного переноса в bits[part - 1]
            if(((this->bits[part] >> 31) & 0x1) == 1) // если старший бит в bits[part] == 1,
            {                                         // то при сдвиге вправо предыдущий бит тоже станет == 1. Это такая 
                this->bits[part] >>= 1;               // логика побитовых операций, поэтому если у нас такой случай, то мы 
                this->bits[part] &= 0x7FFFFFFF;       // сдвигаем биты и самый старший бит обнуляем для последующих сдвигов 
            }
            else
                this->bits[part] >>= 1;               // сдвигаем биты bits[part] вправо на 1 бит
            if(prev_transfer)                         // если от bits[part + 1] был перенос, то его учитываем в bits[part]
                this->bits[part] |= 0x80000000;      
            prev_transfer = transfer;                 // transfer для bits[part] это prev_transfer для bits[part - 1]
        }
    }
}

void Decimal::Long_decimal::discard_zeroes()
{ // Функция удаляет незначащие завершающие нули
    int exponent = this->get_exponent();
    if(exponent != MIN_EXPONENT)
    {
        Long_decimal fractional;
        Long_decimal temp_value = *this;
        fractional = temp_value.to_down_exponent();
        while(fractional.is_zero() && exponent != MIN_EXPONENT)
        {
            *this = temp_value;
            fractional = temp_value.to_down_exponent();
            --exponent;
        }
    }
}

// Методы для работы со знаком числа Long_decimal

int Decimal::Long_decimal::get_sign() const
{
    return (this->bits[PARTS_OF_LONG_DECIMAL - 1] >> 31) & 0x1;
}

void Decimal::Long_decimal::set_sign(int sign)
{
    if(sign) // делаем число отрицательным
        this->bits[PARTS_OF_LONG_DECIMAL - 1] |= 0x80000000;
    else // делаем число положительным
        this->bits[PARTS_OF_LONG_DECIMAL - 1] &= 0x7FFFFFFF;
}

// Методы для работы с экспонентой числа Long_decimal

int Decimal::Long_decimal::get_exponent() const
{   
    return (this->bits[PARTS_OF_LONG_DECIMAL - 1] >> 16) & 0xFF;
}

void Decimal::Long_decimal::set_exponent(int exponent)
{
    this->bits[PARTS_OF_LONG_DECIMAL - 1] &= 0xFF00FFFF;       // очищаем предыдущее значение экспоненты
    this->bits[PARTS_OF_LONG_DECIMAL - 1] |= (exponent << 16); // устанавливаем новое значение экспоненты
}

// Методы для работы с мантиссой числа Long_decimal

int Decimal::Long_decimal::get_bit(int bit_number) const
{
    int index = identify_part_of_mantissa(bit_number);
    bit_number %= 32; // получаем порядковый номер бита в this->bits[index] (т.е в диапазоне от 0 до 31)
    return (this->bits[index] >> bit_number) & 0x1;
}

void Decimal::Long_decimal::set_bit(int bit_number, int bit)
{
    int index = identify_part_of_mantissa(bit_number);
    bit_number %= 32; // получаем порядковый номер бита в this->bits[index] (т.е в диапазоне от 0 до 31)
    if(bit) // bit == 1
        this->bits[index] |=  (0x1 << bit_number); // устанавливаем бит в значение 1
    else
        this->bits[index] &= ~(0x1 << bit_number); // устанавливаем бит в значение 0
}

int Decimal::Long_decimal::identify_part_of_mantissa(int bit_number) const
{
    int part_mantissa = 0; // if(0 <= bit_number && bit_number <= 31) // bit_number in bits[0]
    if(32 <= bit_number && bit_number <= 63)                          // bit_number in bits[1]
        part_mantissa = 1;
    else if(63 <= bit_number && bit_number <= 95)                     // bit_number in bits[2]
        part_mantissa = 2;
    // далее опеределяются индексы для частей Long_decimal
    else if(96 <= bit_number && bit_number <= 127)                    // bit_number in bits[3]
        part_mantissa = 3;
    else if(128 <= bit_number && bit_number <= 159)                   // bit_number in bits[4]
        part_mantissa = 4;
    else if(160 <= bit_number && bit_number <= 191)                   // bit_number in bits[5]
        part_mantissa = 5;
    else if(192 <= bit_number && bit_number <= 223)                   // bit_number in bits[6]
        part_mantissa = 6;
    return part_mantissa;
}

// Вывод значения Long_decimal в десятичной системе счисления

void Decimal::Long_decimal::output_in_dec() const
{
    Long_decimal val = *this;
    char str[150];
    int index = 0;
    int characters = 0;
    int exponent = val.get_exponent();
    val.set_exponent(MIN_EXPONENT);
    int sign = val.get_sign();
    val.set_sign(SIGN_PLUS);
    while(!val.is_zero())
    {
        ++characters;
        if(index == exponent)
            str[index++] = '.';
        Long_decimal temp_fractional = val.div_mantissa_on_10();
        str[index] = temp_fractional.bits[0] + '0';
        ++index;
    }
    if(exponent >= characters)
    {
        for(int ctr = exponent - characters; ctr > 0; --ctr)
            str[index++] = '0'; 
        str[index++] = '.';
        str[index++] = '0';
    }
    if(sign == SIGN_MINUS)
        str[index++] = '-';
    str[index--] = '\0';
    while(index >= 0)
    {
        if(index == 0 && str[index] == '.')
        {
            --index;
            continue;
        } 
        printf("%c", str[index--]);
    }
}

} // end namespace CPP_Decimal