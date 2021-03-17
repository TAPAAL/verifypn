#ifndef PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_
#define PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_

#include "SuccessorGenerator.h"
#include "Structures/State.h"
#include "Structures/light_deque.h"
#include "PQL/PQL.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "LTL/Stubborn/LTLStubbornSet.h"
#include <memory>

namespace LTL {
    class GuardInfo;
}
namespace PetriEngine {

    class ReducingSuccessorGenerator : public SuccessorGenerator {

    public:
        ReducingSuccessorGenerator(const PetriNet &net, std::shared_ptr<StubbornSet> stubbornSet);

        void reset();

        void setQuery(PQL::Condition *ptr) { _stubSet->setQuery(ptr); }

        void prepare(const Structures::State *state);
        void prepare(const Structures::State *state, const successor_info &info) {
            assert(false);
            std::cerr << "ReducingSuccessorGenerator::prepare(State, successor_info) not implemented\n";
            exit(EXIT_FAILURE);
        }

        bool next(Structures::State &write);

        bool next(Structures::State &state, PetriEngine::successor_info &sucinfo) {
            assert(false);
            std::cerr << "ReducingSuccessorGenerator::next(State, successor_info) not implemented\n";
            exit(EXIT_FAILURE);
        }

        auto fired() const { return _current; }

        void generateAll() {
            if (auto ltlstub = std::dynamic_pointer_cast<LTL::LTLStubbornSet>(_stubSet)) {
                ltlstub->generateAll();
            }
        }

    protected:
        std::shared_ptr<StubbornSet> _stubSet;
        uint32_t _current;

        //std::vector<PQL::Condition *> _queries;

    };
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
