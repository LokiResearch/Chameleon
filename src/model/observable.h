/* Copyright 2020 Damien Masson, Sylvain Malacria, Edward Lank, Géry Casiez
               (University of Waterloo, Université de Lille, Inria, France)

This file is part of Chameleon.

Chameleon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Chameleon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Chameleon.  If not, see <https://www.gnu.org/licenses/>. */

/**
    Observable<int> test(0);

    test.addCallbackOnChange([=](int& val) {
        std::cout << val << std::endl;
    });

    test.setValue(42);
**/


#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include <vector>
#include <functional>

template <typename Type>
class Observable
{
public:
    Observable(Type initValue) {
        value = initValue;
    }

    Observable() {}

    Type& getValue() {
        return value;
    }

    Type& setValue(Type newValue) {
        this->value = newValue;
        markAsChanged();
        return this->value;
    }

    void markAsChanged() {
        for (auto callback : callbacks) {
            callback(value);
        }
    }

    // void (*callback)(Type&)
    void addCallbackOnChange(std::function<void (Type&)> callback) {
        callbacks.push_back(callback);
        callback(value);
    }


private:
    Type value;
    std::vector<std::function<void (Type&)>> callbacks;
};

#endif // OBSERVABLE_H
