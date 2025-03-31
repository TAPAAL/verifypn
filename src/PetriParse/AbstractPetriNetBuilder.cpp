
#include "utils/errors.h"
#include "PetriParse/PNMLParser.h"

#include <fstream>
#include <iomanip>
#include <PetriEngine/AbstractPetriNetBuilder.h>


namespace PetriEngine {
    void AbstractPetriNetBuilder::parse_model(const std::string& model)
    {
        std::ifstream mfile(model, std::ifstream::in);
        if (!mfile) {
            throw base_error("Model file ", std::quoted(model), " could not be opened");
        }
        try {
            parse_model(mfile);
        } catch(const base_error& err) {
            throw base_error("Model file ", std::quoted(model), "\n\t", err.what());
        }
        mfile.close();
    }

    void AbstractPetriNetBuilder::parse_model(std::istream& model)
    {
        //Parse and build the petri net
        PNMLParser parser;
        parser.parse(model, this);
    }
}