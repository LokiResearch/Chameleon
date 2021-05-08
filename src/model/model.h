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
#ifndef MODEL_H
#define MODEL_H

// Do not forget to place this somewhere in your .cpp :
// Model* Model::instance = 0;


#include "model/observable.h"

class Model {
private:
    static Model* instance;

    Model() :
      timeBetweenUpdates(1000),
      distanceThreshold(0.098),
      nbAssociationsMax(1000),
      showInfoButton(true),
      redirectAugmentedView(false),
      useAccessibility(true),
      onlyAnalyzeActiveWindow(true)
    {}

public:
    static Model* getInstance() {
        if (instance == NULL) {
            instance = new Model();
        }

        return instance;
    }


    Observable<int> timeBetweenUpdates;
    Observable<double> distanceThreshold;
    Observable<int> nbAssociationsMax;
    Observable<bool> showInfoButton;
    Observable<bool> redirectAugmentedView;
    Observable<bool> useAccessibility;
    Observable<bool> onlyAnalyzeActiveWindow;
};

#endif // MODEL_H

