#ifndef EXPLICITMAIN_H
#define EXPLICITMAIN_H
#include <PetriEngine/options.h>
#include <utils/structures/shared_string.h>
#include <PetriEngine/PQL/PQL.h>
#include <PetriEngine/ExplicitColored/ExplicitErrors.h>
int explicitColored(options_t& options, shared_string_set& string_set, std::vector<PetriEngine::PQL::Condition_ptr>& queries, const std::vector<std::string>& queryNames);
int explicitColoredErrorHandler(const PetriEngine::ExplicitColored::explicit_error& error);

#endif