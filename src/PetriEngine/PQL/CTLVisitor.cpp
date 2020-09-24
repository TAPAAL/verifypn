#include "PetriEngine/PQL/CTLVisitor.h"


namespace PetriEngine::PQL {

    void IsCTLVisitor::_accept(const NotCondition *element) {
        (*element)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const LogicalCondition *element) {
        for (size_t i = 0; i < element->operands(); i++){
            (*element)[i]->visit(*this);
            if (curType != CTLSyntaxType::BOOLEAN){
                isCTL = false;
                break;
            }
        }
    }

    void IsCTLVisitor::_accept(const AndCondition *element) {
        _accept((const LogicalCondition *) element);
    }

    void IsCTLVisitor::_accept(const OrCondition *element) {
        _accept((const LogicalCondition *) element);
    }

    void IsCTLVisitor::_accept(const CompareCondition *element) {
        //We are an atom. No need to check children as they are the same as CTL*
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const LessThanCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const LessThanOrEqualCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const GreaterThanCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const GreaterThanOrEqualCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const EqualCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const NotEqualCondition *element) {
        _accept((const CompareCondition *) element);
    }

    void IsCTLVisitor::_accept(const DeadlockCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const CompareConjunction *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const UnfoldedUpperBoundsCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const EFCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const EGCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const AGCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const AFCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const EXCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const AXCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const EUCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const AUCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
    }

    void IsCTLVisitor::_accept(const ACondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::PATH)
            isCTL = false;
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const ECondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::PATH)
            isCTL = false;
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const GCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        curType = CTLSyntaxType::PATH;
    }

    void IsCTLVisitor::_accept(const FCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        curType = CTLSyntaxType::PATH;
    }

    void IsCTLVisitor::_accept(const XCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        curType = CTLSyntaxType::PATH;
    }

    void IsCTLVisitor::_accept(const UntilCondition *condition) {
        (*condition)[0]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        (*condition)[1]->visit(*this);
        if (curType != CTLSyntaxType::BOOLEAN)
            isCTL = false;
        curType = CTLSyntaxType::PATH;
    }

    void IsCTLVisitor::_accept(const UnfoldedFireableCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const FireableCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const UpperBoundsCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const LivenessCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const KSafeCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const QuasiLivenessCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const StableMarkingCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const BooleanCondition *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const UnfoldedIdentifierExpr *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const LiteralExpr *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const PlusExpr *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const MultiplyExpr *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const MinusExpr *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const SubtractExpr *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }

    void IsCTLVisitor::_accept(const IdentifierExpr *element) {
        curType = CTLSyntaxType::BOOLEAN;
    }


    void AsCTL::_accept(const NotCondition *element) {
        (*element)[0]->visit(*this);
        ctlQuery = std::make_shared<NotCondition>(ctlQuery);
    }

    void AsCTL::_accept(const AndCondition *element) {
        (*element)[0]->visit(*this);
        Condition_ptr first = ctlQuery;
        (*element)[1]->visit(*this);
        ctlQuery = std::make_shared<AndCondition>(first, ctlQuery);
    }

    void AsCTL::_accept(const OrCondition *element) {
        (*element)[0]->visit(*this);
        Condition_ptr first = ctlQuery;
        (*element)[1]->visit(*this);
        ctlQuery = std::make_shared<OrCondition>(first, ctlQuery);
    }

    std::pair<Expr_ptr, Expr_ptr> AsCTL::compareCondition(const CompareCondition *element){
        return std::make_pair(element->getExpr1(), element->getExpr2());
        /*element->getExpr1()->visit(*this);
        Expr_ptr expr1 = curExpr;
        element->getExpr2()->visit(*this);
        return std::make_pair(expr1, curExpr);*/
    }

    void AsCTL::_accept(const LessThanCondition *element) {
        auto exprs = compareCondition(element);
        ctlQuery = std::make_shared<LessThanCondition>(exprs.first, exprs.second);
    }

    void AsCTL::_accept(const LessThanOrEqualCondition *element) {
        auto exprs = compareCondition(element);
        ctlQuery = std::make_shared<LessThanOrEqualCondition>(exprs.first, exprs.second);
    }

    void AsCTL::_accept(const GreaterThanCondition *element) {
        auto exprs = compareCondition(element);
        ctlQuery = std::make_shared<GreaterThanCondition>(exprs.first, exprs.second);
    }

    void AsCTL::_accept(const GreaterThanOrEqualCondition *element) {
        auto exprs = compareCondition(element);
        ctlQuery = std::make_shared<GreaterThanOrEqualCondition>(exprs.first, exprs.second);
    }

    void AsCTL::_accept(const EqualCondition *element) {
        auto exprs = compareCondition(element);
        ctlQuery = std::make_shared<EqualCondition>(exprs.first, exprs.second);
    }

    void AsCTL::_accept(const NotEqualCondition *element) {
        auto exprs = compareCondition(element);
        ctlQuery = std::make_shared<NotEqualCondition>(exprs.first, exprs.second);
    }

    void AsCTL::_accept(const DeadlockCondition *element) {
        ctlQuery = std::make_shared<DeadlockCondition>();
    }

    void AsCTL::_accept(const CompareConjunction *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const CompareConjunction *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const UnfoldedUpperBoundsCondition *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const UnfoldedUpperBoundsCondition *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const EFCondition *condition) {
        (*condition)[0]->visit(*this);
        ctlQuery = std::make_shared<EFCondition>(ctlQuery);
    }

    void AsCTL::_accept(const EGCondition *condition) {
        (*condition)[0]->visit(*this);
        ctlQuery = std::make_shared<EGCondition>(ctlQuery);
    }

    void AsCTL::_accept(const AGCondition *condition) {
        (*condition)[0]->visit(*this);
        ctlQuery = std::make_shared<AGCondition>(ctlQuery);
    }

    void AsCTL::_accept(const AFCondition *condition) {
        (*condition)[0]->visit(*this);
        ctlQuery = std::make_shared<AFCondition>(ctlQuery);
    }

    void AsCTL::_accept(const EXCondition *condition) {
        (*condition)[0]->visit(*this);
        ctlQuery = std::make_shared<EXCondition>(ctlQuery);
    }

    void AsCTL::_accept(const AXCondition *condition) {
        (*condition)[0]->visit(*this);
        ctlQuery = std::make_shared<AXCondition>(ctlQuery);
    }

    void AsCTL::_accept(const EUCondition *condition) {
        (*condition)[0]->visit(*this);
        auto first = ctlQuery;
        (*condition)[1]->visit(*this);
        ctlQuery = std::make_shared<EUCondition>(first, ctlQuery);
    }

    void AsCTL::_accept(const AUCondition *condition) {
        (*condition)[0]->visit(*this);
        auto first = ctlQuery;
        (*condition)[1]->visit(*this);
        ctlQuery = std::make_shared<AUCondition>(first, ctlQuery);
    }

    void AsCTL::_accept(const ACondition *condition) {
        auto child = dynamic_cast<SimpleQuantifierCondition*>((*condition)[0].get());
        switch (child->getPath()) {
            case Path::G:
                (*child)[0]->visit(*this);
                ctlQuery = std::make_shared<AGCondition>(ctlQuery);
                break;
            case Path::X:
                (*child)[0]->visit(*this);
                ctlQuery = std::make_shared<AXCondition>(ctlQuery);
                break;
            case Path::F:
                (*child)[0]->visit(*this);
                ctlQuery = std::make_shared<AFCondition>(ctlQuery);
                break;
            case Path::U: {
                (*child)[0]->visit(*this);
                auto first = ctlQuery;
                (*child)[1]->visit(*this);
                ctlQuery = std::make_shared<AUCondition>(first, ctlQuery);
                break;
            }
            case Path::pError:
                assert(false);
                ctlQuery = nullptr;
                break;
        }
    }

    void AsCTL::_accept(const ECondition *condition) {
        auto child = dynamic_cast<SimpleQuantifierCondition*>((*condition)[0].get());
        switch (child->getPath()) {
            case Path::G:
                (*child)[0]->visit(*this);
                ctlQuery = std::make_shared<EGCondition>(ctlQuery);
                break;
            case Path::X:
                (*child)[0]->visit(*this);
                ctlQuery = std::make_shared<EXCondition>(ctlQuery);
                break;
            case Path::F:
                (*child)[0]->visit(*this);
                ctlQuery = std::make_shared<EFCondition>(ctlQuery);
                break;
            case Path::U: {
                (*child)[0]->visit(*this);
                auto first = ctlQuery;
                (*child)[1]->visit(*this);
                ctlQuery = std::make_shared<EUCondition>(first, ctlQuery);
                break;
            }
            case Path::pError:
                assert(false);
                ctlQuery = nullptr;
                break;
        }
    }

    void AsCTL::_accept(const GCondition *condition) {
        std::cerr << "Direct call to path quantifier in AsCTL GCondition" << std::endl;
        assert(false);
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const FCondition *condition) {
        std::cerr << "Direct call to path quantifier in AsCTL FCondition" << std::endl;
        assert(false);
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const XCondition *condition) {
        std::cerr << "Direct call to path quantifier in AsCTL XCondition" << std::endl;
        assert(false);
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const UntilCondition *condition) {
        std::cerr << "Direct call to path quantifier in AsCTL UntilCondition" << std::endl;
        assert(false);
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const UnfoldedFireableCondition *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const UnfoldedFireableCondition *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const FireableCondition *element) {
        ctlQuery = std::make_shared<FireableCondition>(element->getName());
    }

    void AsCTL::_accept(const UpperBoundsCondition *element) {
        ctlQuery = std::make_shared<UpperBoundsCondition>(element->getPlaces());
    }

    void AsCTL::_accept(const LivenessCondition *element) {
        ctlQuery = std::make_shared<LivenessCondition>();
    }

    void AsCTL::_accept(const KSafeCondition *element) {
        ctlQuery = std::make_shared<KSafeCondition>(element->getBound());
    }

    void AsCTL::_accept(const QuasiLivenessCondition *element) {
        ctlQuery = std::make_shared<QuasiLivenessCondition>();
    }

    void AsCTL::_accept(const StableMarkingCondition *element) {
        ctlQuery = std::make_shared<StableMarkingCondition>();
    }

    void AsCTL::_accept(const BooleanCondition *element) {
        ctlQuery = element->value ? BooleanCondition::TRUE_CONSTANT : BooleanCondition::FALSE_CONSTANT;
    }

    void AsCTL::_accept(const UnfoldedIdentifierExpr *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const UnfoldedIdentifierExpr *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
    }

    //TODO Clean expressions if it is possible to reuse shared pointers.

    void AsCTL::_accept(const LiteralExpr *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const LiteralExpr *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
        //curExpr = std::make_shared<LiteralExpr>(element->value());
    }

    void AsCTL::_accept(const PlusExpr *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const PlusExpr *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
        //curExpr = std::make_shared<PlusExpr>(element->expressions(), element->tk);
    }

    void AsCTL::_accept(const MultiplyExpr *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const MultiplyExpr *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
        //curExpr = std::make_shared<MultiplyExpr>(std::move(element->expressions()));
    }

    void AsCTL::_accept(const MinusExpr *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const MinusExpr *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const SubtractExpr *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const SubtractExpr *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
    }

    void AsCTL::_accept(const IdentifierExpr *element) {
        assert(false);
        std::cerr << "void AsCTL::_accept(const IdentifierExpr *element) Not implemented" << std::endl;
        ctlQuery = nullptr;
    }


}