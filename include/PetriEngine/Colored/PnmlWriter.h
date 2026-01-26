//
// Created by mathi on 09/03/2022.
//

#ifndef VERIFYPN_PNMLWRITER_H
#define VERIFYPN_PNMLWRITER_H

#include "utils/errors.h"
#include "ColoredPetriNetBuilder.h"

namespace PetriEngine::Colored {
    class PnmlWriter {
    public:
        PnmlWriter(PetriEngine::ColoredPetriNetBuilder &b, std::ostream &out) : _builder(b), _out(out), _tabsCount(0)
        {
            for (auto &namedSort: _builder._colors)
            {
                std::vector<const ColorType *> types;
                ColorType *colortype = const_cast<ColorType *>(namedSort.second);
                colortype->getColortypes(types);
                if (is_number(types[0]->operator[](size_t{0}).getColorName())) {
                    _namedSortTypes.emplace(colortype->getName(), "finite range");
                } else {
                    _namedSortTypes.emplace(colortype->getName(), "cyclic enumeration");
                }
            }
        }

        void toColPNML();
        void writeInitialTokens(const std::string& placeId);
    private:
        PetriEngine::ColoredPetriNetBuilder &_builder;
        std::ostream &_out;
        std::uint32_t _tabsCount;
        std::vector<Arc> _arcs;
        std::map<std::string, std::string> _namedSortTypes;

        uint32_t getTabsCount() {
            return _tabsCount;
        }

        std::string getTabs() {
            std::string tabsString;
            for (uint32_t i = 0; i < _tabsCount; i++) {
                tabsString += '\t';
            }
            return tabsString;
        }

        std::string increaseTabs() {
            _tabsCount += 1;
            return getTabs();
        }

        std::string decreaseTabs() {
            if (_tabsCount == 0) {
                throw base_error("Underflow in number of tabs when writing colored PNML");
            }
            _tabsCount -= 1;
            return getTabs();
        }

        bool is_number(const std::string &s);

        std::string guardStringToPnml(std::string guard);

        void metaInfo();

        void declarations();

        void transitions();

        void places();

        void arcs();

        void metaInfoClose();

        void page();

        void handleProductSort(std::vector<const ColorType *> types);

        void handleCyclicEnumeration(std::vector<const ColorType *> types);

        void handleFiniteRange(const std::vector<const ColorType *> &types);

        void handleVariables();

        void handleNamedSorts();

        void handlehlinitialMarking(Multiset marking);

        void handleTokenExpression(const Multiset& tokens);

        void handleType(const Place &place);

        void add_arcs_from_transition(Transition &transition);

        void handleCondition(Colored::Transition &transition);

        void handleProducts(std::vector<std::string> productSorts);

        void handleTuple(const PetriEngine::Colored::Color *const c);

        void handleOtherColor(const Color *const c);

        void handleNumberOf(std::pair<const PetriEngine::Colored::Color *const, uint32_t> numberOff);

        void inhibitorArcs();

        void writeInhibitorExpressionToPnml(Colored::Arc inhibitor);
    };
}


#endif //VERIFYPN_PNMLWRITER_H
