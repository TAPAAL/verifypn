#ifndef PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_
#define PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_

#include "SuccessorGenerator.h"
#include "Structures/State.h"
#include "Structures/light_deque.h"
#include "PQL/PQL.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "LTL/Stubborn/LTLStubbornSet.h"
#include <memory>

namespace PetriEngine {

    class ReducingSuccessorGenerator : public SuccessorGenerator {

    public:
        struct sucinfo {
            uint32_t buchi_state;
            uint32_t tid = no_value;

            [[nodiscard]] inline bool fresh() const {
                return tid == no_value;
            }

            static constexpr auto no_value = std::numeric_limits<uint32_t>::max();
        };

        static constexpr sucinfo initial_suc_info{std::numeric_limits<uint32_t>::max(), sucinfo::no_value};

        void getSuccInfo(sucinfo &sucinfo) const {}

        ReducingSuccessorGenerator(const PetriNet &net, std::shared_ptr<StubbornSet> stubbornSet);

        void reset();

        void setQuery(PQL::Condition *ptr) { _stubSet->setQuery(ptr); }

        void prepare(const Structures::State *state);

        void prepare(const Structures::State *state, const sucinfo &)
        {
            SuccessorGenerator::prepare(state);
        }

        bool next(Structures::State &write);

        bool next(Structures::State &state, const sucinfo &sucinfo);

        auto fired() const { return _current; }

        void generateAll()
        {
            if (auto ltlstub = std::dynamic_pointer_cast<LTL::LTLStubbornSet>(_stubSet)) {
                ltlstub->generateAll();
            }
        }

        const bool *enabled() const { return _stubSet->enabled(); };

        const bool *stubborn() const { return _stubSet->stubborn(); };

        size_t nenabled() { return _stubSet->nenabled(); }

    private:
        std::shared_ptr<StubbornSet> _stubSet;
        uint32_t _current;

        //std::vector<PQL::Condition *> _queries;

    };
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
