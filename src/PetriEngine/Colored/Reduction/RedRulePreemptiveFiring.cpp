//
// Created by mathi on 21/04/2022.
//

#include "PetriEngine/Colored/Reduction/RedRulePreemptiveFiring.h"
#include <PetriEngine/Colored/ArcVarMultisetVisitor.h>
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRulePreemptiveFiring::apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                       QueryType queryType, bool preserveLoops, bool preserveStutter) {
        // Fire initially enabled transitions if they are the single consumer of their preset

        return false;
        /*bool continueReductions = false;

        for (uint32_t t = 0; t < parent->numberOfTransitions(); ++t)
        {
            Transition& tran = parent->_transitions[t];

            if (tran.skip || tran.inhib || tran.pre.empty()) continue;

            // We take advantage of pre and post being sorted as well as the overloaded < operator to check:
            // - Preset and postset must be disjoint (to avoid infinite use)
            // - Preset and postset cannot inhibit or be in query
            // - Preset can only have this transition in postset
            // - How many times can we fire the transition
            uint32_t k = 0;
            bool ok = true;
            uint32_t i = 0, j = 0;
            while (i < tran.pre.size() || j < tran.post.size())
            {
                if (i < tran.pre.size() && (j == tran.post.size() || tran.pre[i] < tran.post[j]))
                {
                    const Arc& prearc = tran.pre[i];
                    uint32_t n = parent->initialMarking[prearc.place] / prearc.weight;
                    if (n == 0 ||
                        parent->_places[prearc.place].inhib ||
                        placeInQuery[prearc.place] > 0 ||
                        parent->_places[prearc.place].consumers.size() != 1)
                    {
                        ok = false;
                        break;
                    }
                    else
                    {
                        if (k == 0) k = n;
                        else k = std::min(k, n);
                    }
                    i++;
                }
                else if (j < tran.post.size() && (i == tran.pre.size() || tran.post[j] < tran.pre[i]))
                {
                    const Arc& postarc = tran.post[j];
                    if (parent->_places[postarc.place].inhib || placeInQuery[postarc.place] > 0)
                    {
                        ok = false;
                        break;
                    }
                    j++;
                }
                else
                {
                    ok = false;
                    break;
                }
            }

            if (!ok || k == 0) continue;

            // Update initial marking
            for (const Arc& prearc : tran.pre)
            {
                parent->initialMarking[prearc.place] -= prearc.weight * k;
            }
            for (const Arc& postarc : tran.post)
            {
                parent->initialMarking[postarc.place] += postarc.weight * k;
            }

            _ruleQ++;
            continueReductions = true;
        }

        return continueReductions;*/
    }
}