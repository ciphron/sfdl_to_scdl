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


#include "SHDLParser.h"
#include "common.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>

using boost::lexical_cast;

namespace scdl {
namespace shdl {

SHDLParser::SHDLParser(std::ifstream &in,
                       std::ofstream &out)
    : in(in), out(out), num_inputs(0), num_outputs(0), num_gates(0)
{
    init();

}

void SHDLParser::init()
{
    gate_names[0] = "zero";
    gate_names[1] = "and";
    gate_names[7] = "or";
    gate_names[6] = "xor";
    gate_names[14] = "nand";
    gate_names[8] = "nor";
    gate_names[2] = "andn";
    gate_names[9] = "eq";
    gate_names[4] = "nimplies";
    gate_names[11] = "implies";
}

void SHDLParser::parse()
{
    std::string line;
    unsigned int num = 0;

    out << "include \"base.scdl\"" << std::endl;

    try {
        while (!in.eof()) {
            getline(in, line);
            if (line.empty())
                continue;

            std::vector<std::string> components;
            boost::split(components, line, boost::is_any_of(":"));

            if (components[0] == "input") {
                parse_input(components);
            }
            else if (components[0] == "result") {
                outputs_to_process.push_back(components);
            }
            else if (components[0] == "gate") {
                parse_gate(components);
            }
        }
        std::vector<std::vector<std::string> >::iterator itr;
        for (itr = outputs_to_process.begin(); itr != outputs_to_process.end();
             itr++) {
            std::vector<std::string> components = *itr;
            parse_output(components);
        }
    }
    catch (const char *s) {
        throw s;
    }

}

void SHDLParser::parse_input(const std::vector<std::string> &components)
{
    Variable var;
    
    var.name = components[1];
    var.type = VAR_INT;
    int length = read_numeric<int>(components[2]);

    for (int i = 0; i < length; i++) {
        Declaration decl;
        decl.name = "in" + lexical_cast<std::string>(num_inputs);
        decl.type = DECL_INPUT;

        declarations.push_back(decl);
        num_inputs++;

        out << "input " << decl.name << std::endl;

        var.components.push_back(decl.name);
    }

    input_vars.push_back(var);
}

void SHDLParser::parse_output(const std::vector<std::string> &components)
{
    Variable var;
    
    var.name= components[1];
    var.type = VAR_INT;
    int length = read_numeric<int>(components[2]);

    if (components.size() < length + 3)
        throw "Invalid output declaration";

    for (int i = 0; i < length; i++) {
        int decl_num = read_numeric<int>(components[3 + i]);

        if (decl_num >= declarations.size())
          throw "Wire of output could not be found";

        std::string connected_to = declarations[decl_num].name;
        std::string func_name = "out" + lexical_cast<std::string>(num_outputs);
        out << "func " << func_name
            << " = " << connected_to << std::endl;

        var.components.push_back(func_name);
        num_outputs++;
    }

    output_vars.push_back(var);
}

void SHDLParser::parse_gate(const std::vector<std::string> &components)
{
    int num = read_numeric<int>(components[2]);

    if (num != declarations.size())
        throw "Invalid declaration number";

    Declaration decl;

    decl.name = "g" + lexical_cast<std::string>(num_gates);
    decl.type = DECL_GATE;

    declarations.push_back(decl);
    num_gates++;

    std::string left = read_gate_input(components[3]);
    std::string right = read_gate_input(components[4]);

    int gate_id = read_numeric<int>(components[5]);

    if (gate_names.find(gate_id) == gate_names.end()) {
        std::vector<std::string> minterms;
        
        for (int i = 0; i < 4; i++) {
            if (gate_id & 1 << (3 - i)) {
                std::string minterm = ((i & 0x02) ? "x" : "not(x)") +
                    std::string("*") +
                    ((i & 0x01) ? "y" : "not(y)");
                minterms.push_back(minterm);
            }
        }

        int depth = 0;
        std::string expr;

        if (minterms.empty())
            expr = "zero";
        else {
            std::vector<std::string>::iterator itr;
            for (itr = minterms.begin(); itr != minterms.end() - 1; itr++) {
                expr += "or(" + *itr + ", ";
                depth++;
            }
            expr += minterms[minterms.size() - 1];
            while (depth--)
                expr += ")";
        }

        gate_names[gate_id] = "gate" + lexical_cast<std::string>(gate_id);
        out << "func " << gate_names[gate_id] << "(x, y) = " << expr
            << std::endl;
    }

    out << "func " << decl.name << " = ";

    
    std::string gate_name = gate_names[gate_id];

    if (gate_name == "and") {
        out << left << " * " << right;
    }
    else if (gate_name == "xor") {
        out << left << " + " << right;
    }
    else {
        out << gate_name << '(' << left << ", " << right << ')';
    }

    out << std::endl;
}

size_t SHDLParser::get_num_input_variables() const
{
    return input_vars.size();
}

Variable SHDLParser::get_input_variable(int index) const
{
    if (index < 0 || index >= input_vars.size())
        throw "Bad input index";
    return input_vars[index];
}

size_t SHDLParser::get_num_output_variables() const
{
    return output_vars.size();
}

Variable SHDLParser::get_output_variable(int index) const
{
    if (index < 0 || index >= output_vars.size())
        throw "Bad input index";
    return output_vars[index];
}

std::string SHDLParser::read_gate_input(const std::string &str)
{
    std::string gate_input;
    
    if (str == "T")
        gate_input = "one";
    else if (str == "F")
        gate_input = "zero";
    else {
        int decl_num = read_numeric<int>(str);

        if (decl_num >= declarations.size())
            throw "Could not find inputs to gate";

        gate_input = declarations[decl_num].name;
    }

    return gate_input;
}

}
}
