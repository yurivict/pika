// Copyright Vladimir Prus 2002-2004.
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

/** This example shows how to handle response file.

    For a test, build and run:
       response_file -I foo @response_file.rsp

    The expected output is:
      Include paths: foo bar biz

    Thanks to Hartmut Kaiser who raised the issue of response files
    and discussed the possible approach.
*/

#include <pika/modules/program_options.hpp>

#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

using namespace boost;
using namespace pika::program_options;
using namespace std;

// Additional command line parser which interprets '@something' as a
// option "config-file" with the value "something"
pair<string, string> at_option_parser(string const& s)
{
    if ('@' == s[0])
        return std::make_pair(string("response-file"), s.substr(1));
    else
        return pair<string, string>();
}

int main(int ac, char* av[])
{
    try
    {
        options_description desc("Allowed options");
        // clang-format off
        desc.add_options()
            ("help", "produce a help message")
            ("include-path,I", value< vector<string> >()->composing(),
                 "include path")
            ("magic", value<int>(), "magic value")
            ("response-file", value<string>(),
                 "can be specified with '@name', too")
        ;
        // clang-format on

        variables_map vm;
        store(command_line_parser(ac, av).options(desc).extra_parser(at_option_parser).run(), vm);

        if (vm.count("help"))
        {
            cout << desc;
        }
        if (vm.count("response-file"))
        {
            // Load the file and tokenize it
            ifstream ifs(vm["response-file"].as<string>().c_str());
            if (!ifs)
            {
                cout << "Could not open the response file: " << vm["response-file"].as<string>()
                     << "\n";
                return 1;
            }

            // Read the whole file into a string
            stringstream ss;
            ss << ifs.rdbuf();

            // Split the file content
            string sstr = ss.str();
            char_separator<char> sep(" \n\r");
            tokenizer<char_separator<char>> tok(sstr, sep);

            vector<string> args;
            copy(tok.begin(), tok.end(), back_inserter(args));

            // Parse the file and store the options
            store(command_line_parser(args).options(desc).run(), vm);
        }

        if (vm.count("include-path"))
        {
            const vector<string>& s = vm["include-path"].as<vector<string>>();
            cout << "Include paths: ";
            copy(s.begin(), s.end(), ostream_iterator<string>(cout, " "));
            cout << "\n";
        }
        if (vm.count("magic"))
        {
            cout << "Magic value: " << vm["magic"].as<int>() << "\n";
        }
    }
    catch (std::exception& e)
    {
        cout << e.what() << "\n";
    }
    return 0;
}
