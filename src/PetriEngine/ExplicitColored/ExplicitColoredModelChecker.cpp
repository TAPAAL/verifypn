#include "PetriEngine/ExplicitColored/ExplicitColoredModelChecker.h"

#include <VerifyPN.h>
#include <PetriEngine/ExplicitColored/ColorIgnorantPetriNetBuilder.h>
#include <PetriEngine/PQL/Evaluation.h>

namespace PetriEngine::ExplicitColored {
    ExplicitColoredModelChecker::Result ExplicitColoredModelChecker::checkQuery(
        options_t& options,
        shared_string_set& string_set,
        PetriEngine::PQL::Condition_ptr query,
        std::string queryName
    ) {

    }

    ExplicitColoredModelChecker::Result ExplicitColoredModelChecker::checkColorIgnorantLP(shared_string_set& string_set, Condition_ptr query, options_t& options) {
        if (const auto efGammaQuery = dynamic_cast<PQL::EFCondition*>(query.get())) {

        }
        ColorIgnorantPetriNetBuilder ignorantBuilder(string_set);
        auto status = ignorantBuilder.build();
        if (status == ColoredIgnorantPetriNetBuilderStatus::CONTAINS_NEGATIVE) {
            return Result::UNKNOWN;
        }
        auto builder = ignorantBuilder.getUnderlying();
        auto qnet = std::unique_ptr<PetriNet>(builder.makePetriNet(false));
        std::unique_ptr<MarkVal[]> qm0(qnet->makeInitialMarking());
        size_t initial_size = 0;
        std::vector queries { std::move(query) };
        std::vector queryNames = { queryNames };
        contextAnalysis(false, {}, {}, builder, qnet.get(), queries);


        for(size_t i = 0; i < qnet->numberOfPlaces(); ++i)
            initial_size += qm0[i];
        {
            EvaluationContext context(qm0.get(), qnet.get());
            ContainsFireabilityVisitor has_fireability;
            Visitor::visit(has_fireability, query);

            auto r = PQL::evaluate(query.get(), context);
            if(r == Condition::RFALSE)
            {
                query = BooleanCondition::FALSE_CONSTANT;
            }
            else if(r == Condition::RTRUE)
            {
                query = BooleanCondition::TRUE_CONSTANT;
            }
        }


        // simplification. We always want to do negation-push and initial marking check.
        simplify_queries(qm0.get(), qnet.get(), queries, options, std::cout);

        outputQueries(builder, queries, queryNames, options.query_out_file, options.binary_query_io, options.keep_solved);
        if (query == BooleanCondition::FALSE_CONSTANT) {

        }
    }
}
