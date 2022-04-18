/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_COLOREDUSEVISITOR_H
#define VERIFYPN_COLOREDUSEVISITOR_H

#include "PetriEngine/PQL/Visitor.h"

#include <vector>

namespace PetriEngine::PQL {
    class ColoredUseVisitor : public Visitor {
    public:
        ColoredUseVisitor(const shared_name_index_map &placeNameToIndexMap, size_t places,
                          const shared_name_index_map &transitionNameToIndexMap, size_t transitions)
                : _placeNameToIndexMap(placeNameToIndexMap), _transitionNameToIndexMap(transitionNameToIndexMap),
                _placeInUse(places), _transitionInUse(transitions) {

        }

        bool isPlaceUsed(uint32_t p) const {
            return p < _placeInUse.size() && _placeInUse[p];
        }

        bool isTransitionUsed(uint32_t t) const {
            return t < _transitionInUse.size() && _transitionInUse[t];
        }

    protected:
        void _accept(const NotCondition *element) override;

        void _accept(const DeadlockCondition *element) override;

        void _accept(const CompareConjunction *element) override;

        void _accept(const SimpleQuantifierCondition *element) override;

        void _accept(const LogicalCondition *element) override;

        void _accept(const CompareCondition *element) override;

        void _accept(const UntilCondition *element) override;

        void _accept(const FireableCondition *element) override;

        void _accept(const ShallowCondition *element) override;

        void _accept(const BooleanCondition *element) override;

        void _accept(const IdentifierExpr *element) override;

        void _accept(const LiteralExpr *element) override;

        void _accept(const MinusExpr *element) override;

        void _accept(const NaryExpr *element) override;

    private:
        const shared_name_index_map &_placeNameToIndexMap;
        const shared_name_index_map &_transitionNameToIndexMap;
        std::vector<bool> _placeInUse;
        std::vector<bool> _transitionInUse;
    };
}

#endif //VERIFYPN_COLOREDUSEVISITOR_H
