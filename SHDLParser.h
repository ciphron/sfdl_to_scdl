/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef SHDL_PARSER_H
#define SHDL_PARSER_H

#include "Variable.h"

#include <set>
#include <vector>
#include <map>
#include <cstdlib>
#include <string>


#include <fstream>

namespace scdl {
namespace shdl {
enum DeclType {
    DECL_INPUT,
    DECL_OUTPUT,
    DECL_GATE
};


struct Declaration {
    DeclType type;
    std::string name;
    int num;
};


class SHDLParser {
public:
    SHDLParser(std::ifstream &in,
               std::ofstream &out);

    void parse();

    size_t get_num_input_variables() const;
    Variable get_input_variable(int index) const;

    size_t get_num_output_variables() const;
    Variable get_output_variable(int index) const;

private:
    std::ifstream &in;
    std::ofstream &out;
    std::vector<Declaration> declarations;
    std::map<int,std::string> gate_names;
    int num_inputs;
    int num_outputs;
    int num_gates;
    std::vector<Variable> input_vars;
    std::vector<Variable> output_vars;
    std::vector<std::vector<std::string> > outputs_to_process;

    void init();

    void parse_input(const std::vector<std::string> &components);
    void parse_output(const std::vector<std::string> &components);
    void parse_gate(const std::vector<std::string> &components);
    std::string read_gate_input(const std::string &str);
};



#endif // SHDL_PARSER_H
}
}
