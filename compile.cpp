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
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <json/json.h>

using namespace scdl;

std::string extract_base_name(const std::string &name)
{
    size_t dot_pos = name.find_last_of(".");
    size_t base_len = (dot_pos == std::string::npos)
                      ? name.length()
                      : dot_pos;

    return name.substr(0, base_len);
}

struct json_object *var_to_json_object(const Variable &var)
{
    std::string type;
        
    if (var.type == VAR_INT) {
        type = "int<" +
               boost::lexical_cast<std::string>(var.components.size()) +
               ">";
    }
    else if (var.type == VAR_UINT) {
        type = "uint<" +
                boost::lexical_cast<std::string>(var.components.size()) +
                ">";
    }
    else if (var.type == VAR_BOOL) {
        type = "bool";
    }
    else {
        type = "bitstring";
    }

    struct json_object *comps = json_object_new_array();
    std::vector<std::string>::const_iterator itr;
    for (itr = var.components.begin(); itr != var.components.end(); itr++) {
        json_object_array_add(comps, json_object_new_string(itr->c_str()));
    }                                  

    struct json_object *var_obj = json_object_new_object();
    json_object_object_add(var_obj, "name",
                           json_object_new_string(var.name.c_str()));
    json_object_object_add(var_obj, "type",
                           json_object_new_string(type.c_str()));
    json_object_object_add(var_obj, "components", comps);

    return var_obj;
}



void write_vars_file(const shdl::SHDLParser &parser,
                     std::ofstream &out)
{
    struct json_object *obj = json_object_new_object();
    struct json_object *inputs = json_object_new_array();
    struct json_object *outputs = json_object_new_array();
                                                             
    size_t n_inputs = parser.get_num_input_variables();
    for (int i = 0; i < n_inputs; i++) {
        Variable var = parser.get_input_variable(i);
        json_object_array_add(inputs, var_to_json_object(var));
    }

    size_t n_outputs = parser.get_num_output_variables();
    for (int i = 0; i < n_outputs; i++) {
        Variable var = parser.get_output_variable(i);
        json_object_array_add(outputs, var_to_json_object(var));
    }

    json_object_object_add(obj, "inputs", inputs);
    json_object_object_add(obj, "outputs", outputs);

    const char *str = json_object_to_json_string_ext(obj,
                                                     JSON_C_TO_STRING_PRETTY);
    out << str << std::endl;

    free(obj);
}


void compile(const std::string &source_file)
{
    std::string cmd = "java -cp sfdl2_compiler sfdl.Compiler " + source_file;
    system(cmd.c_str());


    std::ifstream shdl_in((source_file + ".shdl").c_str());
    if (!shdl_in.good()) {
        shdl_in.close();
        std::cerr << "shdl file not found" << std::endl;
        exit(1);
    }
        

    std::string base_name = extract_base_name(source_file);
    std::ofstream scdl_out((base_name + ".scdl").c_str());
    std::ofstream vars_out((base_name + ".scdl.vars").c_str());

    shdl::SHDLParser parser(shdl_in, scdl_out);
    parser.parse();

    write_vars_file(parser, vars_out);

    shdl_in.close();
    scdl_out.close();
    vars_out.close();
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << "<filename>" << std::endl;
        exit(1);
    }
    
    try {
        compile(argv[1]);
    }
    catch (const char *e) {
        std::cout << e << std::endl;
    }

    return 0;
}
