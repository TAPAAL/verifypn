#include "Renamer.h"

#include <stdio.h>
#include <string>
#include <iostream>

using namespace std;

void Renamer::removeWildCharacters(std::string &text) {
	cout << text << endl;
	text.erase( std::remove_if( text.begin(), text.end(), ::isspace ), text.end() );
    int pos = 0;
    string replaceDash = "__dash__";
    string replaceSlash = "__slash__";

    while ((pos = text.find("-")) != -1) {
        text.replace(pos, 1, replaceDash);
    }

    while ((pos = text.find("/")) != -1) {
        text.replace(pos, 1, replaceSlash);
    }
}

void Renamer::addWildCharacters(std::string &text) {
    int pos = 0;
    string replaceDash = "__dash__";
    string replaceSlash = "__slash__";

    while ((pos = text.find(replaceDash)) != -1) {
        text.replace(pos, replaceDash.length(), "-");
    }

    while ((pos = text.find(replaceSlash)) != -1) {
        text.replace(pos, replaceSlash.length(), "/");
    }

}
