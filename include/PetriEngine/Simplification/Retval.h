#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        struct Retval {
            std::shared_ptr<PQL::Condition> formula = nullptr;
            AbstractProgramCollection_ptr lps;
            AbstractProgramCollection_ptr neglps;

            Retval(const std::shared_ptr<PQL::Condition> formula,
                   AbstractProgramCollection_ptr&& lps1,
                   AbstractProgramCollection_ptr&& lps2)
            : formula(formula), lps(std::move(lps1)),  neglps(std::move(lps2))
            {
                assert(lps);
                assert(neglps);
            }

            Retval(const std::shared_ptr<PQL::Condition> formula,
                   const AbstractProgramCollection_ptr& lps1,
                   const AbstractProgramCollection_ptr& lps2)
            : formula(formula), lps(lps1), neglps(lps2)
            {
                assert(lps);
                assert(neglps);
            }

            Retval(const std::shared_ptr<PQL::Condition> formula)
            : formula(formula)
            {
                lps = std::make_shared<SingleProgram>();
                neglps  = std::make_shared<SingleProgram>();
                assert(lps);
                assert(neglps);
            }
 
            Retval(const Retval&& other)
            : formula(std::move(other.formula)), lps(std::move(other.lps)), neglps(std::move(other.neglps))
            {
                assert(lps);
                assert(neglps);
            }
            
            Retval(Retval&& other)
            : formula(other.formula), lps(std::move(other.lps)), neglps(std::move(other.neglps))
            {
                assert(lps);
                assert(neglps);
            }

            Retval& operator=(Retval&& other)
            {
                lps = std::move(other.lps);
                neglps = std::move(other.neglps);
                formula = std::move(other.formula);
                assert(lps);
                assert(neglps);
                return *this;
            }

            Retval()
            {
                lps = std::make_shared<SingleProgram>();
                neglps  = std::make_shared<SingleProgram>();
                assert(lps);
                assert(neglps);
            }

            ~Retval() {}
        };

        struct RetvalPot {
            AbstractProgramCollection_ptr lps;

            RetvalPot(AbstractProgramCollection_ptr&& lps1)
            : lps(std::move(lps1))
            {
                assert(lps);
            }

            RetvalPot(const AbstractProgramCollection_ptr& lps1)
            : lps(lps1)
            {
                assert(lps);
            }

            RetvalPot(const std::shared_ptr<PQL::Condition> formula)
            {
                lps = std::make_shared<SingleProgram>();
                assert(lps);
            }

            RetvalPot(const RetvalPot&& other)
            : lps(std::move(other.lps))
            {
                assert(lps);
            }

            RetvalPot(RetvalPot&& other)
            : lps(std::move(other.lps))
            {
                assert(lps);
            }

            RetvalPot& operator=(RetvalPot&& other)
            {
                lps = std::move(other.lps);
                assert(lps);
                return *this;
            }

            RetvalPot()
            {
                lps = std::make_shared<SingleProgram>();
                assert(lps);
            }

            ~RetvalPot() {}
        };
    }
}

#endif /* RETVAL_H */
