#ifndef PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_
#define PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_

#include "SuccessorGenerator.h"
#include "Structures/State.h"
#include "Structures/light_deque.h"
#include "PQL/PQL.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "LTL/SuccessorGeneration/VisibleLTLStubbornSet.h"
#include <memory>

namespace LTL {
    class GuardInfo;
}
namespace PetriEngine {

    class ReducingSuccessorGenerator : public SuccessorGenerator {

    public:
        struct sucinfo {
            uint32_t buchi_state;
            uint32_t tid = no_value;
            size_t enabled; // id from ptrie storage at call-site, see EnabledTransitionSet
            size_t stubborn;
            size_t last_state;

            [[nodiscard]] bool hasEnabled() const {
                return enabled != std::numeric_limits<size_t>::max() && stubborn != std::numeric_limits<size_t>::max();
            }

            [[nodiscard]] inline bool fresh() const {
                return tid == no_value;
            }
            inline bool has_prev_state() const {
                return last_state != std::numeric_limits<size_t>::max();
            }

            static constexpr auto no_value = std::numeric_limits<uint32_t>::max();
        };

        static constexpr sucinfo initial_suc_info{
            std::numeric_limits<uint32_t>::max(),
            sucinfo::no_value,
            std::numeric_limits<size_t>::max(),
            std::numeric_limits<size_t>::max(),
            std::numeric_limits<size_t>::max()};

        void getSuccInfo(sucinfo &sucinfo) const {}

        ReducingSuccessorGenerator(const PetriNet &net, std::shared_ptr<StubbornSet> stubbornSet);

        ReducingSuccessorGenerator(const PetriNet &net,
                                   std::shared_ptr<StubbornSet> stubbornSet,
                                   const bool* enabled, const bool* stubborn);

        void reset();

        void setQuery(PQL::Condition *ptr) { _stubSet->setQuery(ptr); }

        bool prepare(const Structures::State *state) override;

        void prepare(const Structures::State *state, const sucinfo &sucinfo)
        {
            if (sucinfo.hasEnabled()) {
                SuccessorGenerator::prepare(state);
            }
            // indirection to stubborn set prepare, thus calculating stubborn and enabled arrays.
            else prepare(state);
        }

        bool next(Structures::State &write) override;

        bool next(Structures::State &state, sucinfo &sucinfo);

        auto fired() const { return _current; }

        void generateAll()
        {
            if (auto ltlstub = std::dynamic_pointer_cast<LTL::VisibleLTLStubbornSet>(_stubSet)) {
                ltlstub->generateAll();
            }
        }

        bool *enabled() const { return _stubSet->enabled(); };

        bool *stubborn() const { return _stubSet->stubborn(); };

        size_t nenabled() { return _stubSet->nenabled(); }

    protected:
        std::shared_ptr<StubbornSet> _stubSet;
        uint32_t _current;

        //std::vector<PQL::Condition *> _queries;

        const bool* _enabled = enabled();
        const bool* _stubborn = stubborn();
    };
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
