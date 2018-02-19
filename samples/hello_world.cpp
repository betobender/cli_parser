/*
MIT License

Copyright (c) 2018 Roberto Bender

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <iostream>
#include "cli_parser.h"

int main(int argc, char* argv[])
{
    cli::Parser options("Sample Application", "9.9.9.9", "This is a sample application description. The string here will be there" \
                                                         "break into multiple lines if they overlap CLI_MAX_LINE_WIDTH");
    bool showVersion = false;

    options.addOptions({
        {{"-v", "--version"}, "Shows the application version.", false, {{}}, [&showVersion](cli::Parser::Option& opt){ showVersion = true; return true; }},
        {{"--mandatory"}, "This a mandatory argument and it expect two following args {arg1} and {arg2}.", true, {{"arg1", "The argument 1."}, {"arg2", "The argument 2."}}}
    });

    if(options.parse(argc, argv) == cli::Parser::PARSED_OK)
    {
        std::cout << "Parsing OK!" << std::endl;
        std::cout << "Argument 1: " << options("--mandatory").value("arg1") << std::endl;
        std::cout << "Argument 2: " << options("--mandatory").value("arg2") << std::endl;
    }

    if(showVersion)
        std::cout << "Showing application version: 9.9.9.9" << std::endl;
}