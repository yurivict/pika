// Copyright Vladimir Prus 2002-2004.
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

/* The simplest usage of the library.
 */

#include <pika/modules/program_options.hpp>

#include <iostream>

namespace po = pika::program_options;
using namespace std;

int main(int ac, char* av[])
{
    try
    {
        po::options_description desc("Allowed options");
        // clang-format off
        desc.add_options()
            ("help", "produce help message")
            ("compression", po::value<double>(), "set compression level")
        ;
        // clang-format on

        po::variables_map vm;
        po::store(po::command_line_parser(ac, av).allow_unregistered().options(desc).run(), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            cout << desc << "\n";
            return 0;
        }

        if (vm.count("compression"))
        {
            cout << "Compression level was set to " << vm["compression"].as<double>() << ".\n";
        }
        else
        {
            cout << "Compression level was not set.\n";
        }
    }
    catch (exception const& e)
    {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        cerr << "Exception of unknown type!\n";
    }
    return 0;
}
